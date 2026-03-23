#include "EnemyCharacter.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"

AEnemyCharacter::AEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AAIController::StaticClass();

	GetCharacterMovement()->DefaultLandMovementMode = MOVE_Walking;
	GetCharacterMovement()->bOrientRotationToMovement = false;
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHP = MaxHP;
	InitialSpawnLocation = GetActorLocation();
	InitialSpawnRotation = GetActorRotation();

	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	TargetPlayer = UGameplayStatics::GetPlayerPawn(this, 0);
	UE_LOG(LogTemp, Warning, TEXT("BeginPlay Target: %s"), *GetNameSafe(TargetPlayer));

	EnterIdleState();
}

void AEnemyCharacter::ReceiveAttackDamage(float DamageAmount)
{
	UE_LOG(LogTemp, Warning, TEXT("ReceiveAttackDamage called. DamageAmount = %f, CurrentHP(before) = %d"), DamageAmount, CurrentHP);

	if (bIsDead || bIsRespawning)
	{
		UE_LOG(LogTemp, Warning, TEXT("Enemy already dead or respawning"));
		return;
	}

	if (bIsDead)
	{
		UE_LOG(LogTemp, Warning, TEXT("Enemy already dead"));
		return;
	}

	if (!bCanTakeDamage)
	{
		UE_LOG(LogTemp, Warning, TEXT("Enemy cannot take damage now"));
		return;
	}

	if (DamageAmount <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("DamageAmount <= 0"));
		return;
	}

	CurrentHP -= FMath::RoundToInt(DamageAmount);
	CurrentHP = FMath::Max(CurrentHP, 0);

	UE_LOG(LogTemp, Warning, TEXT("Enemy HP(after) = %d"), CurrentHP);

	bCanTakeDamage = false;
	GetWorldTimerManager().SetTimer(
		DamageInvincibleTimer,
		[this]()
		{
			bCanTakeDamage = true;
		},
		DamageInvincibleTime,
		false
	);

	if (CurrentHP <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Enter BlowawayState"));
		EnterBlowawayState();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Enter DamageState"));
		EnterDamageState();
	}
}

void AEnemyCharacter::EnterIdleState()
{
	if (bIsDead)
	{
		return;
	}

	CurrentState = EEnemyState::Idle;
}

void AEnemyCharacter::EnterDamageState()
{
	if (bIsDead)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnterDamageState blocked: already dead"));
		return;
	}

	CurrentState = EEnemyState::Damage;
	UE_LOG(LogTemp, Warning, TEXT("EnterDamageState called"));

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("Damage: AnimInstance is null"));
		return;
	}

	if (!DamageMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("Damage: DamageMontage is null"));
		return;
	}

	const float Played = AnimInstance->Montage_Play(DamageMontage);
	UE_LOG(LogTemp, Warning, TEXT("Damage Montage_Play result = %f"), Played);

	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AEnemyCharacter::OnDamageMontageEnded);
	AnimInstance->OnMontageEnded.AddDynamic(this, &AEnemyCharacter::OnDamageMontageEnded);
}

void AEnemyCharacter::EnterBlowawayState()
{
	if (bIsDead)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnterBlowawayState blocked: already dead"));
		return;
	}

	bIsDead = true;
	CurrentState = EEnemyState::Blowaway;

	UE_LOG(LogTemp, Warning, TEXT("EnterBlowawayState called"));

	GetCharacterMovement()->DisableMovement();

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	float DelayBeforeDeathBlink = 0.5f;

	if (!AnimInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("Blowaway: AnimInstance is null"));
	}
	else if (!BlowawayMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("Blowaway: BlowawayMontage is null"));
	}
	else
	{
		const float Played = AnimInstance->Montage_Play(BlowawayMontage);
		UE_LOG(LogTemp, Warning, TEXT("Blowaway Montage_Play result = %f"), Played);

		if (Played > 0.0f)
		{
			DelayBeforeDeathBlink = Played;
		}
	}

	GetWorldTimerManager().ClearTimer(StartDeathBlinkTimerHandle);
	GetWorldTimerManager().SetTimer(
		StartDeathBlinkTimerHandle,
		this,
		&AEnemyCharacter::StartDeathBlink,
		DelayBeforeDeathBlink,
		false
	);
}

void AEnemyCharacter::OnDamageMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (bIsDead)
	{
		return;
	}

	if (Montage == DamageMontage)
	{
		EnterIdleState();
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AEnemyCharacter::OnDamageMontageEnded);
	}
}

void AEnemyCharacter::StartDeathBlink()
{
	bIsRespawning = true;
	bIsRespawnBlinking = false;

	GetWorldTimerManager().ClearTimer(DeathBlinkTimerHandle);
	GetWorldTimerManager().SetTimer(
		DeathBlinkTimerHandle,
		this,
		&AEnemyCharacter::ToggleDeathBlinkVisibility,
		BlinkInterval,
		true
	);

	GetWorldTimerManager().ClearTimer(HideEnemyTimerHandle);
	GetWorldTimerManager().SetTimer(
		HideEnemyTimerHandle,
		this,
		&AEnemyCharacter::HideEnemy,
		DeathBlinkDuration,
		false
	);
}

void AEnemyCharacter::ToggleDeathBlinkVisibility()
{
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetVisibility(!MeshComp->IsVisible(), true);
	}
}

void AEnemyCharacter::HideEnemy()
{
	GetWorldTimerManager().ClearTimer(DeathBlinkTimerHandle);

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetVisibility(false, true);
	}

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorHiddenInGame(true);

	const float HiddenDelay = FMath::Max(0.0f, TotalRespawnTime - DeathBlinkDuration - RespawnBlinkDuration);

	GetWorldTimerManager().ClearTimer(RespawnTimerHandle);
	GetWorldTimerManager().SetTimer(
		RespawnTimerHandle,
		this,
		&AEnemyCharacter::RespawnEnemy,
		HiddenDelay,
		false
	);
}

void AEnemyCharacter::RespawnEnemy()
{
	SetActorLocation(InitialSpawnLocation);
	SetActorRotation(InitialSpawnRotation);

	SetActorHiddenInGame(false);

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetVisibility(false, true);
	}

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	CurrentHP = MaxHP;
	bIsDead = false;
	CurrentState = EEnemyState::Idle;

	StartRespawnBlink();
}

void AEnemyCharacter::StartRespawnBlink()
{
	bIsRespawnBlinking = true;

	GetWorldTimerManager().ClearTimer(RespawnBlinkTimerHandle);
	GetWorldTimerManager().SetTimer(
		RespawnBlinkTimerHandle,
		this,
		&AEnemyCharacter::ToggleRespawnBlinkVisibility,
		BlinkInterval,
		true
	);

	GetWorldTimerManager().ClearTimer(FinishRespawnTimerHandle);
	GetWorldTimerManager().SetTimer(
		FinishRespawnTimerHandle,
		this,
		&AEnemyCharacter::FinishRespawn,
		RespawnBlinkDuration,
		false
	);
}

void AEnemyCharacter::ToggleRespawnBlinkVisibility()
{
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetVisibility(!MeshComp->IsVisible(), true);
	}
}

void AEnemyCharacter::FinishRespawn()
{
	GetWorldTimerManager().ClearTimer(RespawnBlinkTimerHandle);

	SetActorHiddenInGame(false);

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetVisibility(true, true);
	}

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	bCanTakeDamage = true;
	bIsRespawning = false;
	bIsRespawnBlinking = false;

	EnterIdleState();
}

void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UE_LOG(LogTemp, Warning, TEXT("Enemy Tick"));

	if (!TargetPlayer || bIsDead || bIsRespawning)
	{
		return;
	}

	const float Distance = FVector::Dist(GetActorLocation(), TargetPlayer->GetActorLocation());
	UE_LOG(LogTemp, Warning, TEXT("Distance = %f"), Distance);

	if (Distance <= AttackDistance)
	{
		UE_LOG(LogTemp, Warning, TEXT("TryAttack"));
		TryAttack();
		return;
	}

	if (Distance <= ChaseDistance)
	{
		UE_LOG(LogTemp, Warning, TEXT("MoveToPlayer"));
		MoveToPlayer(DeltaTime);
		return;
	}
}

void AEnemyCharacter::MoveToPlayer(float DeltaTime)
{
	if (!TargetPlayer)
	{
		return;
	}

	FVector Direction = TargetPlayer->GetActorLocation() - GetActorLocation();
	Direction.Z = 0.0f;

	if (!Direction.Normalize())
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Direction = %s"), *Direction.ToString());

	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;

	AddMovementInput(Direction, 1.0f, true);

	const FRotator LookAtRotation = Direction.Rotation();
	SetActorRotation(FRotator(0.0f, LookAtRotation.Yaw, 0.0f));
}

void AEnemyCharacter::TryAttack()
{
	if (!bCanAttack) return;

	bCanAttack = false;

	UE_LOG(LogTemp, Warning, TEXT("Enemy Attack"));

	PlayAttackMontage();

	GetWorldTimerManager().SetTimer(
		AttackCooldownTimer,
		[this]()
		{
			bCanAttack = true;
		},
		AttackInterval,
		false
	);
}

void AEnemyCharacter::PlayAttackMontage()
{
	UAnimInstance* Anim = GetMesh()->GetAnimInstance();

	if (!Anim || !AttackMontage)
	{
		return;
	}

	Anim->Montage_Play(AttackMontage);
}