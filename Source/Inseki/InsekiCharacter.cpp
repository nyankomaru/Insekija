// Copyright Epic Games, Inc. All Rights Reserved.

#include "InsekiCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Inseki.h"
#include "EnemyCharacter.h"
#include "Kismet/KismetSystemLibrary.h"

AInsekiCharacter::AInsekiCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator::ZeroRotator;

	GetCharacterMovement()->JumpZVelocity = JumpVerticalVelocity;
	GetCharacterMovement()->GravityScale = NormalGravityScale;

	GetCharacterMovement()->MaxWalkSpeed = 380.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->AirControl = 1.0f;
	GetCharacterMovement()->MaxWalkSpeed = 380.f;
	GetCharacterMovement()->MaxAcceleration = 1400.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 1200.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 0.0f;
	GetCharacterMovement()->FallingLateralFriction = 0.0f;
	GetCharacterMovement()->GroundFriction = 6.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void AInsekiCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// -----------------------------
	// コヨーテタイム
	// -----------------------------
	if (GetCharacterMovement()->IsMovingOnGround())
	{
		CoyoteTimer = CoyoteTime;
	}
	else
	{
		CoyoteTimer -= DeltaTime;
	}

	// -----------------------------
	// ジャンプバッファ
	// -----------------------------
	if (JumpBufferTimer > 0.0f)
	{
		JumpBufferTimer -= DeltaTime;

		if (CoyoteTimer > 0.0f && !GetCharacterMovement()->IsFalling())
		{
			DoJumpStart();
			JumpBufferTimer = 0.0f;
		}
	}

	// -----------------------------
	// ジャンプ重力制御
	// -----------------------------
	const bool bIsCurrentlyFalling = GetCharacterMovement()->IsFalling();

	if (bIsJumpFalling && bIsCurrentlyFalling)
	{
		const float Z = GetVelocity().Z;

		// 上昇
		if (Z > 20.0f)
		{
			GetCharacterMovement()->GravityScale = JumpRiseGravityScale;
		}
		// 頂点付近～落下開始
		else
		{
			GetCharacterMovement()->GravityScale = JumpFallGravityScale;
		}
	}

	// 足場落下検出
	if (bIsCurrentlyFalling && !bWasFallingLastFrame)
	{
		if (!bIsJumpFalling)
		{
			FVector Velocity = GetCharacterMovement()->Velocity;
			Velocity.Y *= LedgeFallHorizontalVelocityKeepRatio;
			GetCharacterMovement()->Velocity = Velocity;
		}
	}

	bWasFallingLastFrame = bIsCurrentlyFalling;

	// -----------------------------
	// StartMove解除後の入力立ち上がり
	// -----------------------------
	const float TargetReleaseBlend = bMoveStartLocked ? 0.0f : 1.0f;
	MoveStartReleaseBlend = FMath::FInterpTo(
		MoveStartReleaseBlend,
		TargetReleaseBlend,
		DeltaTime,
		MoveStartReleaseInterpSpeed
	);

	// -----------------------------
	// 回転補間
	// -----------------------------
	if (bIsTurning)
	{
		VisualYaw = FMath::FixedTurn(
			VisualYaw,
			TurnTargetYaw,
			TurnSpeedDegreesPerSecond * DeltaTime
		);

		SetActorRotation(FRotator(0.0f, VisualYaw, 0.0f));

		if (FMath::IsNearlyEqual(VisualYaw, TurnTargetYaw, 1.0f))
		{
			VisualYaw = TurnTargetYaw;
			FacingSign = PendingFacingSign;
			bIsTurning = false;

			SetActorRotation(FRotator(0.0f, VisualYaw, 0.0f));
		}
	}

	// -----------------------------
	// 空中攻撃予約：頂点で発動
	// -----------------------------
	TryExecuteQueuedAirAttackAtApex();
}

void AInsekiCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// ジャンプ
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AInsekiCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AInsekiCharacter::DoJumpEnd);

		// 移動
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AInsekiCharacter::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AInsekiCharacter::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Canceled, this, &AInsekiCharacter::Move);

		// パンチ
		if (PunchAction)
		{
			EnhancedInputComponent->BindAction(PunchAction, ETriggerEvent::Started, this, &AInsekiCharacter::OnPunchPressed);
		}

		// キック
		if (KickAction)
		{
			EnhancedInputComponent->BindAction(KickAction, ETriggerEvent::Started, this, &AInsekiCharacter::OnKickPressed);
		}
	}
	else
	{
		UE_LOG(LogInseki, Error, TEXT("'%s' Enhanced Inputコンポーネントの取得に失敗しました。"), *GetNameSafe(this));
	}
}

void AInsekiCharacter::StartTurnToSign(float NewFacingSign)
{
	if (FMath::IsNearlyZero(NewFacingSign))
	{
		return;
	}

	// すでに同じ向きへ回転中なら何もしない
	if (bIsTurning && PendingFacingSign == NewFacingSign)
	{
		return;
	}

	PendingFacingSign = NewFacingSign;
	bIsTurning = true;

	if (NewFacingSign > 0.0f)
	{
		TurnTargetYaw = RightFacingYaw;
	}
	else
	{
		TurnTargetYaw = LeftFacingYaw;
	}
}

void AInsekiCharacter::UpdateTurnFromInput(float RightInput)
{
	if (FMath::IsNearlyZero(RightInput, MoveInputDeadZone))
	{
		return;
	}

	const float InputSign = FMath::Sign(RightInput);

	// ターン中なら最終的に向かっている方向を見る
	const float CurrentOrPendingSign = bIsTurning ? PendingFacingSign : FacingSign;

	if (InputSign != CurrentOrPendingSign)
	{
		StartTurnToSign(InputSign);
	}
}

void AInsekiCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();
	DoMove(MovementVector.X, MovementVector.Y);
}

void AInsekiCharacter::BeginMoveStart()
{
	if (bIsMoveStarting || bMoveStartLocked || bMoveStartConsumed)
	{
		return;
	}

	bIsMoveStarting = true;
	bMoveStartLocked = true;
	bMoveStartConsumed = true;
	MoveStartReleaseBlend = 0.0f;

	FVector Velocity = GetCharacterMovement()->Velocity;
	Velocity.Y = 0.0f;
	GetCharacterMovement()->Velocity = Velocity;

	GetWorldTimerManager().ClearTimer(MoveStartSafetyUnlockTimer);
	GetWorldTimerManager().SetTimer(
		MoveStartSafetyUnlockTimer,
		this,
		&AInsekiCharacter::UnlockMoveStart,
		0.16f,
		false
	);
}

void AInsekiCharacter::ResetMoveStartState()
{
	bIsMoveStarting = false;
	bMoveStartLocked = false;
	bMoveStartConsumed = false;
	MoveStartReleaseBlend = 1.0f;
}

void AInsekiCharacter::UnlockMoveStart()
{
	bMoveStartLocked = false;
	bIsMoveStarting = false;
}
void AInsekiCharacter::DoMove(float Right, float Forward)
{
	const bool bIsAirborne = GetCharacterMovement()->IsFalling();
	const bool bShouldBlockMoveByAttack = bIsAttacking && !bIsAirborne;

	if (bShouldBlockMoveByAttack || bLandingLock)
	{
		bHasMoveInput = false;
		return;
	}

	if (GetController() == nullptr)
	{
		bHasMoveInput = false;
		return;
	}

	const FVector WorldRightDirection = FVector(0.0f, 1.0f, 0.0f);

	bHasMoveInput = !FMath::IsNearlyZero(Right, MoveInputDeadZone);

	if (bHasMoveInput)
	{
		LastMoveInput = FMath::Sign(Right);
		UpdateTurnFromInput(Right);
	}

	// 地上
	if (!GetCharacterMovement()->IsFalling())
	{
		const float CurrentSpeed2D = FVector(GetVelocity().X, GetVelocity().Y, 0.0f).Size();

		if (bHasMoveInput &&
			CurrentSpeed2D < MoveStartSpeedThreshold &&
			!bIsMoveStarting &&
			!bMoveStartLocked &&
			!bMoveStartConsumed)
		{
			BeginMoveStart();
		}

		if (!bHasMoveInput)
		{
			ResetMoveStartState();
		}

		if (bMoveStartLocked)
		{
			FVector Velocity = GetCharacterMovement()->Velocity;
			Velocity.Y = 0.0f;
			GetCharacterMovement()->Velocity = Velocity;
			return;
		}

		AddMovementInput(WorldRightDirection, Right * MoveStartReleaseBlend);
		return;
	}

	// 空中
	FVector Velocity = GetCharacterMovement()->Velocity;
	Velocity.Y = Right * GetCharacterMovement()->MaxWalkSpeed * JumpAirMoveScale;
	GetCharacterMovement()->Velocity = Velocity;
}

void AInsekiCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AInsekiCharacter::DoJumpStart()
{
	if (bIsAttacking || bLandingLock)
	{
		return;
	}

	if (GetCharacterMovement()->IsFalling() && CoyoteTimer <= 0.0f)
	{
		JumpBufferTimer = JumpBufferTime;
		return;
	}

	GetCharacterMovement()->bOrientRotationToMovement = false;

	ResetMoveStartState();

	// ジャンプ由来の空中状態
	bIsJumpFalling = true;
	bJumpHeld = true;

	// 地上の横速度を引き継ぎたくないなら消す
	FVector Velocity = GetCharacterMovement()->Velocity;
	Velocity.Y = 0.0f;
	GetCharacterMovement()->Velocity = Velocity;

	// 上昇開始時の重力
	GetCharacterMovement()->GravityScale = JumpRiseGravityScale;

	// 真上ジャンプのみ
	const FVector LaunchVelocity(0.0f, 0.0f, JumpVerticalVelocity);
	LaunchCharacter(LaunchVelocity, false, true);
}

void AInsekiCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	GetCharacterMovement()->GravityScale = NormalGravityScale;

	bIsJumpFalling = false;
	bJumpHeld = false;
	bWasFallingLastFrame = false;

	ResetMoveStartState();

	// パンチ予約をクリア
	bPunchQueued = false;
	bCanQueueNextPunch = false;

	bLandingLock = true;

	GetWorldTimerManager().SetTimer(
		LandingTimer,
		[this]()
		{
			bLandingLock = false;
		},
		0.15f,
		false
	);
}
void AInsekiCharacter::DoJumpEnd()
{
	bJumpHeld = false;
	StopJumping();

	// 上昇中に離したら少し早めに落とす
	if (GetCharacterMovement()->IsFalling() && GetVelocity().Z > 0.0f)
	{
		GetCharacterMovement()->GravityScale = JumpFallGravityScale;
	}
}

void AInsekiCharacter::OnPunchPressed()
{
	// 空中ではパンチ不可
	if (GetCharacterMovement()->IsFalling())
	{
		return;
	}

	// すでにパンチ中なら、次段を予約
	if (bIsAttacking && CurrentAttackType == EAttackType::Punch)
	{
		if (bCanQueueNextPunch)
		{
			bPunchQueued = true;
		}
		return;
	}

	StartPunchAttack();
}

void AInsekiCharacter::OnKickPressed()
{
	FAttackData AttackData;
	AttackData.AttackType = EAttackType::Kick;
	AttackData.Montage = KickMontage;
	AttackData.SectionName = TEXT("Kick1");
	AttackData.Damage = 3.0f;

	const bool bIsAirborne = GetCharacterMovement()->IsFalling();

	if (bIsAttacking)
	{
		return;
	}

	// 地上なら即発動
	if (!bIsAirborne)
	{
		TryStartAttack(AttackData);
		return;
	}

	// 空中上昇中なら頂点予約
	if (IsRisingInAir())
	{
		QueueAirAttack(AttackData);
		return;
	}

	// 落下中は無効
	if (IsFallingInAir())
	{
		return;
	}
}

void AInsekiCharacter::StartPunchAttack()
{
	if (!PunchMontage)
	{
		UE_LOG(LogInseki, Warning, TEXT("PunchMontage が設定されていません。"));
		return;
	}

	FAttackData AttackData;
	AttackData.AttackType = EAttackType::Punch;
	AttackData.Montage = PunchMontage;
	AttackData.Damage = 1.0f;

	switch (NextPunchIndex)
	{
	case 1:
		AttackData.SectionName = TEXT("Punch1");
		CurrentPunchIndex = 1;
		break;
	case 2:
		AttackData.SectionName = TEXT("Punch2");
		CurrentPunchIndex = 2;
		break;
	case 3:
		AttackData.SectionName = TEXT("Punch3");
		CurrentPunchIndex = 3;
		break;
	default:
		AttackData.SectionName = TEXT("Punch1");
		CurrentPunchIndex = 1;
		NextPunchIndex = 1;
		break;
	}

	bPunchQueued = false;
	bCanQueueNextPunch = false;

	if (TryStartAttack(AttackData))
	{
		NextPunchIndex++;
		if (NextPunchIndex > 3)
		{
			NextPunchIndex = 1;
		}
	}
}

void AInsekiCharacter::StartKickAttack()
{
	if (!KickMontage)
	{
		UE_LOG(LogInseki, Warning, TEXT("KickMontage が設定されていません。"));
		return;
	}

	FAttackData AttackData;
	AttackData.AttackType = EAttackType::Kick;
	AttackData.Montage = KickMontage;
	AttackData.SectionName = TEXT("Kick1");

	if (TryStartAttack(AttackData))
	{
		UE_LOG(LogInseki, Log, TEXT("Kick started"));
	}
}

bool AInsekiCharacter::TryStartAttack(const FAttackData& AttackData)
{
	if (bIsAttacking)
	{
		return false;
	}

	if (!AttackData.Montage)
	{
		return false;
	}

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		UE_LOG(LogInseki, Warning, TEXT("AnimInstance が取得できません。"));
		return false;
	}

	const float PlayedLength = AnimInstance->Montage_Play(AttackData.Montage);
	if (PlayedLength <= 0.0f)
	{
		UE_LOG(LogInseki, Warning, TEXT("Montage_Play に失敗しました: %s"), *GetNameSafe(AttackData.Montage));
		return false;
	}

	if (AttackData.SectionName != NAME_None)
	{
		AnimInstance->Montage_JumpToSection(AttackData.SectionName, AttackData.Montage);
	}

	bAttackStartedInAir = GetCharacterMovement()->IsFalling();

	if (!bAttackStartedInAir)
	{
		GetCharacterMovement()->StopMovementImmediately();
		ResetMoveStartState();
	}

	bIsAttacking = true;
	CurrentAttackType = AttackData.AttackType;
	CurrentAttackDamage = AttackData.Damage;

	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AInsekiCharacter::OnAttackMontageEnded);
	AnimInstance->OnMontageEnded.AddDynamic(this, &AInsekiCharacter::OnAttackMontageEnded);

	return true;
}

void AInsekiCharacter::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	const EAttackType EndedAttackType = CurrentAttackType;

	bIsAttacking = false;
	bAttackStartedInAir = false;
	CurrentAttackType = EAttackType::None;
	CurrentAttackDamage = 0.0f;
	bCanQueueNextPunch = false;

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (AnimInstance)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &AInsekiCharacter::OnAttackMontageEnded);
	}

	if (!bInterrupted &&
		EndedAttackType == EAttackType::Punch &&
		bPunchQueued &&
		GetCharacterMovement() &&
		GetCharacterMovement()->IsMovingOnGround())
	{
		bPunchQueued = false;
		StartPunchAttack();
	}
}

void AInsekiCharacter::OpenPunchQueueWindow()
{
	if (bIsAttacking && CurrentAttackType == EAttackType::Punch)
	{
		bCanQueueNextPunch = true;
	}
}

void AInsekiCharacter::ClosePunchQueueWindow()
{
	bCanQueueNextPunch = false;
}

bool AInsekiCharacter::IsRisingInAir() const
{
	return GetCharacterMovement()->IsFalling() && GetVelocity().Z > AirAttackApexThresholdZ;
}

bool AInsekiCharacter::IsFallingInAir() const
{
	return GetCharacterMovement()->IsFalling() && GetVelocity().Z <= AirAttackApexThresholdZ;
}

void AInsekiCharacter::QueueAirAttack(const FAttackData& AttackData)
{
	bAirAttackQueued = true;
	QueuedAirAttackData = AttackData;
}

void AInsekiCharacter::TryExecuteQueuedAirAttackAtApex()
{
	if (!bAirAttackQueued)
	{
		return;
	}

	// まだ上昇中なら出さない
	if (!GetCharacterMovement()->IsFalling())
	{
		return;
	}

	// 頂点付近～落下開始に入ったら出す
	if (GetVelocity().Z <= AirAttackApexThresholdZ)
	{
		const FAttackData AttackData = QueuedAirAttackData;
		bAirAttackQueued = false;
		QueuedAirAttackData = FAttackData();

		TryStartAttack(AttackData);
	}
}

void AInsekiCharacter::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->GravityScale = NormalGravityScale;
	GetCharacterMovement()->JumpZVelocity = JumpVerticalVelocity;

	VisualYaw = RightFacingYaw;
	FacingSign = 1.0f;
	TurnTargetYaw = RightFacingYaw;
	PendingFacingSign = 1.0f;

	SetActorRotation(FRotator(0.0f, VisualYaw, 0.0f));

	if (UAnimInstance* Anim = GetMesh()->GetAnimInstance())
	{
		Anim->OnPlayMontageNotifyBegin.AddDynamic(this, &AInsekiCharacter::OnMontageNotifyBegin);
	}
}

void AInsekiCharacter::OnMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& Payload)
{
	UE_LOG(LogTemp, Warning, TEXT("Notify Begin: %s"), *NotifyName.ToString());

	if (NotifyName == "OpenPunchQueue")
	{
		OpenPunchQueueWindow();
	}
	else if (NotifyName == "ClosePunchQueue")
	{
		ClosePunchQueueWindow();
	}
	else if (NotifyName == "StartMoveUnlock")
	{
		UnlockMoveStart();
	}
	else if (NotifyName == "BeginAttackHitCheck")
	{
		BeginAttackHitCheck();
	}
	else if (NotifyName == "EndAttackHitCheck")
	{
		EndAttackHitCheck();
	}
}

void AInsekiCharacter::BeginAttackHitCheck()
{
	UE_LOG(LogTemp, Warning, TEXT("BeginAttackHitCheck called"));

	HitActorsThisSwing.Empty();

	float HitRadius = PunchHitRadius;
	FVector HitOffset = PunchHitOffset;

	if (CurrentAttackType == EAttackType::Kick)
	{
		HitRadius = KickHitRadius;
		HitOffset = KickHitOffset;
	}

	const FVector Center = GetActorLocation()
		+ GetActorForwardVector() * HitOffset.X
		+ GetActorRightVector() * HitOffset.Y
		+ GetActorUpVector() * HitOffset.Z;

	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(this);

	TArray<FHitResult> HitResults;

	const bool bHit = UKismetSystemLibrary::SphereTraceMulti(
		this,
		Center,
		Center,
		HitRadius,
		UEngineTypes::ConvertToTraceType(ECC_Pawn),
		false,
		IgnoreActors,
		EDrawDebugTrace::None,   // ←ここ変更
		HitResults,
		true
	);

	if (!bHit)
	{
		UE_LOG(LogTemp, Warning, TEXT("SphereTrace hit nothing"));
		return;
	}

	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor)
		{
			continue;
		}

		UE_LOG(LogTemp, Warning, TEXT("Hit Actor: %s"), *GetNameSafe(HitActor));

		if (HitActorsThisSwing.Contains(HitActor))
		{
			continue;
		}

		if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(HitActor))
		{
			UE_LOG(LogTemp, Warning, TEXT("Enemy hit. Damage = %f"), CurrentAttackDamage);
			Enemy->ReceiveAttackDamage(CurrentAttackDamage);
			HitActorsThisSwing.Add(HitActor);
		}
	}
}

void AInsekiCharacter::EndAttackHitCheck()
{
	HitActorsThisSwing.Empty();
}