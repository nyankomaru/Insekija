#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Animation/AnimMontage.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnemyCharacter.generated.h"

/**
 * 敵キャラクターの状態定義
 *
 * Idle     : 通常状態
 * Damage   : 被ダメージ状態
 * Blowaway : 吹き飛び状態
 */
UENUM(BlueprintType)
enum class EEnemyState : uint8
{
	Idle		UMETA(DisplayName = "Idle"),
	Damage		UMETA(DisplayName = "Damage"),
	Blowaway	UMETA(DisplayName = "Blowaway")
};

/**
 * 敵キャラクタークラス
 *
 * 主な役割:
 * ・プレイヤーの検知、追跡、攻撃
 * ・被ダメージ、吹き飛び、死亡状態の管理
 * ・点滅演出を含むリスポーン処理
 * ・段差や高低差を考慮したジャンプ制御
 *
 * 想定ゲーム:
 * ・固定カメラ式の3D風2Dアクションゲーム
 * ・1画面内で戦闘を行う横移動型アクション
 */
UCLASS()
class INSEKI_API AEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyCharacter();

protected:
	// ==================================================
	// Engine Override
	// ==================================================

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void Landed(const FHitResult& Hit) override;

public:
	// ==================================================
	// Damage Interface
	// ==================================================

	/**
	 * プレイヤーなど外部から攻撃を受けた際に呼ばれる
	 *
	 * @param DamageAmount 与えるダメージ量
	 */
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void ReceiveAttackDamage(float DamageAmount);

protected:
	// ==================================================
	// AI Target / Basic AI Parameter
	// ==================================================

	/** 追跡対象となるプレイヤー */
	UPROPERTY()
	AActor* TargetPlayer = nullptr;

	/** プレイヤーを追跡開始する距離 */
	UPROPERTY(EditAnywhere, Category = "Enemy|AI")
	float ChaseDistance = 600.0f;

	/** 攻撃を行う距離 */
	UPROPERTY(EditAnywhere, Category = "Enemy|AI")
	float AttackDistance = 90.0f;

	/** 移動速度 */
	UPROPERTY(EditAnywhere, Category = "Enemy|AI")
	float MoveSpeed = 200.0f;

	/** 攻撃のクールタイム */
	UPROPERTY(EditAnywhere, Category = "Enemy|AI")
	float AttackInterval = 1.5f;

	/** 現在攻撃可能かどうか */
	UPROPERTY(VisibleAnywhere, Category = "Enemy|AI")
	bool bCanAttack = true;

	/** 攻撃クールタイム管理用タイマー */
	FTimerHandle AttackCooldownTimer;

protected:
	// ==================================================
	// AI Jump Control
	// ==================================================

	/** プレイヤーとの高低差がこの値を超えた場合にジャンプを検討する */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|AI|Jump")
	float JumpHeightThreshold = 25.0f;

	/** ジャンプ後の再ジャンプ禁止時間 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|AI|Jump")
	float JumpCooldown = 0.25f;

	/** 段差落下後、着地してから再ジャンプ可能になるまでの時間 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|AI|Jump")
	float LedgeFallJumpLockTime = 1.0f;

	/** 現在ターゲットに向かってジャンプ可能かどうか */
	bool bCanJumpToTarget = true;

	/** 自発的なジャンプを開始したかどうか */
	bool bEnemyJumpStarted = false;

	/** 段差からの落下を検出したかどうか */
	bool bLedgeFallDetected = false;

	/** 段差落下後、ジャンプが一時的に禁止されているか */
	bool bJumpLockedAfterLedgeFall = false;

	/** 前フレームで落下状態だったかどうか */
	bool bWasFallingLastFrame = false;

	/** ジャンプクールタイム管理用タイマー */
	FTimerHandle JumpCooldownTimer;

	/** 段差落下後のジャンプロック管理用タイマー */
	FTimerHandle LedgeFallJumpLockTimer;

protected:
	// ==================================================
	// Status
	// ==================================================

	/** 最大HP */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Status")
	int32 MaxHP = 3;

	/** 現在HP */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Status")
	int32 CurrentHP = 3;

	/** 現在の行動状態 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|State")
	EEnemyState CurrentState = EEnemyState::Idle;

	/** 死亡済みかどうか */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|State")
	bool bIsDead = false;

protected:
	// ==================================================
	// Damage Control
	// ==================================================

	/** ダメージを受け付けるかどうか */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Damage")
	bool bCanTakeDamage = true;

	/** 被弾後の無敵時間 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Damage")
	float DamageInvincibleTime = 0.15f;

	/** 被弾無敵時間管理用タイマー */
	FTimerHandle DamageInvincibleTimer;

protected:
	// ==================================================
	// Animation
	// ==================================================

	/** 攻撃アニメーションモンタージュ */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Animation")
	TObjectPtr<UAnimMontage> AttackMontage = nullptr;

	/** 被ダメージアニメーションモンタージュ */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Animation")
	TObjectPtr<UAnimMontage> DamageMontage = nullptr;

	/** 吹き飛びアニメーションモンタージュ */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Animation")
	TObjectPtr<UAnimMontage> BlowawayMontage = nullptr;

protected:
	// ==================================================
	// Respawn Control
	// ==================================================

	/** 死亡から復活までの総時間 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Respawn")
	float TotalRespawnTime = 5.0f;

	/** 死亡時点滅演出の継続時間 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Respawn")
	float DeathBlinkDuration = 1.0f;

	/** 復活時点滅演出の継続時間 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Respawn")
	float RespawnBlinkDuration = 1.0f;

	/** 点滅間隔 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Respawn")
	float BlinkInterval = 0.1f;

	/** 現在リスポーン処理中かどうか */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Respawn")
	bool bIsRespawning = false;

	/** 現在復活点滅中かどうか */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Respawn")
	bool bIsRespawnBlinking = false;

	/** 初期スポーン位置 */
	FVector InitialSpawnLocation;

	/** 初期スポーン回転 */
	FRotator InitialSpawnRotation;

	FTimerHandle StartDeathBlinkTimerHandle;
	FTimerHandle DeathBlinkTimerHandle;
	FTimerHandle HideEnemyTimerHandle;
	FTimerHandle RespawnTimerHandle;
	FTimerHandle RespawnBlinkTimerHandle;
	FTimerHandle FinishRespawnTimerHandle;

protected:
	// ==================================================
	// AI Behavior Function
	// ==================================================

	/** プレイヤーに向かって移動する */
	void MoveToPlayer(float DeltaTime);

	/** 攻撃可能条件を満たす場合に攻撃を試行する */
	void TryAttack();

	/** 攻撃モンタージュを再生する */
	void PlayAttackMontage();

	/** 高低差や状況に応じてジャンプを試行する */
	void TryJumpToTarget();

protected:
	// ==================================================
	// State Transition
	// ==================================================

	/** 通常状態へ遷移する */
	void EnterIdleState();

	/** 被ダメージ状態へ遷移する */
	void EnterDamageState();

	/** 吹き飛び状態へ遷移する */
	void EnterBlowawayState();

protected:
	// ==================================================
	// Respawn Sequence Function
	// ==================================================

	/** 死亡時の点滅演出を開始する */
	void StartDeathBlink();

	/** 死亡時点滅の表示切り替え */
	void ToggleDeathBlinkVisibility();

	/** 敵を非表示にする */
	void HideEnemy();

	/** 復活時の点滅演出を開始する */
	void StartRespawnBlink();

	/** 復活時点滅の表示切り替え */
	void ToggleRespawnBlinkVisibility();

	/** 復活処理を完了する */
	void FinishRespawn();

	/** 敵を初期位置に再出現させる */
	void RespawnEnemy();

protected:
	// ==================================================
	// Animation Callback
	// ==================================================

	/**
	 * 被ダメージモンタージュ終了時に呼ばれる
	 *
	 * @param Montage      終了したモンタージュ
	 * @param bInterrupted 中断終了かどうか
	 */
	UFUNCTION()
	void OnDamageMontageEnded(UAnimMontage* Montage, bool bInterrupted);
};