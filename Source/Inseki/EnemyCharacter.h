#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Animation/AnimMontage.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnemyCharacter.generated.h"

UENUM(BlueprintType)
enum class EEnemyState : uint8
{
	Idle		UMETA(DisplayName = "Idle"),
	Damage		UMETA(DisplayName = "Damage"),
	Blowaway	UMETA(DisplayName = "Blowaway")
};

UCLASS()
class INSEKI_API AEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override; // ★ 追加

protected:

	// =========================
	// AI
	// =========================

	UPROPERTY()
	AActor* TargetPlayer = nullptr;

	UPROPERTY(EditAnywhere, Category = "Enemy|AI")
	float ChaseDistance = 600.0f;

	UPROPERTY(EditAnywhere, Category = "Enemy|AI")
	float AttackDistance = 120.0f;

	UPROPERTY(EditAnywhere, Category = "Enemy|AI")
	float MoveSpeed = 200.0f;

	UPROPERTY(EditAnywhere, Category = "Enemy|AI")
	float AttackInterval = 1.5f;

	UPROPERTY(VisibleAnywhere, Category = "Enemy|AI")
	bool bCanAttack = true;

	FTimerHandle AttackCooldownTimer;

	// 攻撃モンタージュ
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Attack")
	TObjectPtr<UAnimMontage> AttackMontage = nullptr;

	// =========================
	// Status
	// =========================

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Status")
	int32 MaxHP = 3;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Status")
	int32 CurrentHP = 3;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|State")
	EEnemyState CurrentState = EEnemyState::Idle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|State")
	bool bIsDead = false;

	// =========================
	// Animation
	// =========================

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Animation")
	TObjectPtr<UAnimMontage> DamageMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Animation")
	TObjectPtr<UAnimMontage> BlowawayMontage = nullptr;

	// =========================
	// Damage制御
	// =========================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Damage")
	bool bCanTakeDamage = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Damage")
	float DamageInvincibleTime = 0.15f;

	FTimerHandle DamageInvincibleTimer;

	// =========================
	// Respawn
	// =========================

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Respawn")
	float TotalRespawnTime = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Respawn")
	float DeathBlinkDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Respawn")
	float RespawnBlinkDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Respawn")
	float BlinkInterval = 0.1f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Respawn")
	bool bIsRespawning = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Respawn")
	bool bIsRespawnBlinking = false;

	FVector InitialSpawnLocation;
	FRotator InitialSpawnRotation;

	FTimerHandle StartDeathBlinkTimerHandle;
	FTimerHandle DeathBlinkTimerHandle;
	FTimerHandle HideEnemyTimerHandle;
	FTimerHandle RespawnTimerHandle;
	FTimerHandle RespawnBlinkTimerHandle;
	FTimerHandle FinishRespawnTimerHandle;

	void StartDeathBlink();
	void ToggleDeathBlinkVisibility();
	void HideEnemy();

	void StartRespawnBlink();
	void ToggleRespawnBlinkVisibility();
	void FinishRespawn();

	void RespawnEnemy();

	// =========================
	// AI処理関数（★追加）
	// =========================

	void MoveToPlayer(float DeltaTime);
	void TryAttack();
	void PlayAttackMontage();

public:

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void ReceiveAttackDamage(float DamageAmount);

protected:

	void EnterIdleState();
	void EnterDamageState();
	void EnterBlowawayState();

	UFUNCTION()
	void OnDamageMontageEnded(UAnimMontage* Montage, bool bInterrupted);
};