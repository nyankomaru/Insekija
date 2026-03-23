// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "InsekiCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UAnimMontage;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UENUM(BlueprintType)
enum class EAttackType : uint8
{
	None	UMETA(DisplayName = "None"),
	Punch	UMETA(DisplayName = "Punch"),
	Kick	UMETA(DisplayName = "Kick")
};

USTRUCT(BlueprintType)
struct FAttackData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	EAttackType AttackType = EAttackType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	TObjectPtr<UAnimMontage> Montage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	FName SectionName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float Damage = 0.0f;
};

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class AInsekiCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MouseLookAction;

	/** Punch Input Action (RB) */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* PunchAction;

	/** Kick Input Action (LB) */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* KickAction;

	/** Punch montage. Sections: Punch1, Punch2, Punch3 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	TObjectPtr<UAnimMontage> PunchMontage;

	/** Kick montage. Section: Kick1 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	TObjectPtr<UAnimMontage> KickMontage;

	/** 現在攻撃中か */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack")
	bool bIsAttacking = false;

	/** 現在の攻撃タイプ */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack")
	EAttackType CurrentAttackType = EAttackType::None;

	/** 次に出すパンチ番号（1～3をループ） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack")
	int32 NextPunchIndex = 1;

public:

	/** Constructor */
	AInsekiCharacter();

	UFUNCTION(BlueprintPure, Category = "Attack")
	float GetCurrentAttackDamage() const { return CurrentAttackDamage; }

protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Punch input */
	void OnPunchPressed();

	/** Kick input */
	void OnKickPressed();

	/** 汎用攻撃再生 */
	virtual bool TryStartAttack(const FAttackData& AttackData);

	/** パンチ実行 */
	virtual void StartPunchAttack();

	/** キック実行 */
	virtual void StartKickAttack();

	/** モンタージュ終了時 */
	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	virtual void Landed(const FHitResult& Hit) override;

	UFUNCTION(BlueprintCallable, Category = "Attack")
	void OpenPunchQueueWindow();

	UFUNCTION(BlueprintCallable, Category = "Attack")
	void ClosePunchQueueWindow();

	UFUNCTION()
	void OnMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& Payload);

	virtual void BeginPlay() override;

	// =============================
	// Jump System
	// =============================

	// 足場落下時の空中横制御
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Fall")
	float LedgeFallAirControlScale = 0.05f;

	// ジャンプ垂直速度
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump")
	float JumpVerticalVelocity = 540.0f;

	// 通常時重力
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump")
	float NormalGravityScale = 1.6f;

	// 上昇中重力
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump")
	float JumpRiseGravityScale = 1.2f;

	// 落下中重力
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump")
	float JumpFallGravityScale = 2.8f;

	// 空中横移動の強さ
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump")
	float JumpAirMoveScale = 1.f;

	// StartMove解除後の入力立ち上がり補間
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float MoveStartReleaseBlend = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float MoveStartReleaseInterpSpeed = 10.0f;

	// 着地硬直
	bool bLandingLock = false;
	FTimerHandle LandingTimer;

	// コヨーテタイム
	float CoyoteTime = 0.12f;
	float CoyoteTimer = 0.0f;

	// ジャンプ入力バッファ
	float JumpBufferTime = 0.12f;
	float JumpBufferTimer = 0.0f;

	// ジャンプ由来の空中状態か
	bool bIsJumpFalling = false;

	// ジャンプボタン押しっぱなし管理（任意）
	bool bJumpHeld = false;

	// Tick
	virtual void Tick(float DeltaTime) override;

	// パンチの次段予約
	bool bPunchQueued = false;

	// 今のパンチ番号
	int32 CurrentPunchIndex = 0;

	// コンボ受付中か
	bool bCanQueueNextPunch = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack")
	float CurrentAttackDamage = 0.0f;

	// =============================
	// Movement Start / Stop System
	// =============================

	// 歩き出しロック中か
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bMoveStartLocked = false;

	// 歩き出し中か
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bIsMoveStarting = false;

	// 入力があるか
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bHasMoveInput = false;

	// 最後の移動入力方向
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float LastMoveInput = 0.0f;

	// 移動の立ち上がり補間
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float CurrentMoveInputScale = 0.0f;

	// 補間速度
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float MoveInputInterpSpeed = 8.0f;

	// 立ち上がり判定を行う速度閾値
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float MoveStartSpeedThreshold = 5.0f;

	// 移動停止判定の入力閾値
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float MoveInputDeadZone = 0.05f;

	// 移動開始ロックの安全解除時間
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float MoveStartSafetyUnlockTime = 0.10f;

	FTimerHandle MoveStartSafetyUnlockTimer;

	// 歩き出し開始
	void BeginMoveStart();

	// 歩き出し状態リセット
	void ResetMoveStartState();

	bool bMoveStartConsumed = false;

	// 前フレームで落下中だったか
	bool bWasFallingLastFrame = false;

	// 足場落下開始時に横速度をどれだけ残すか
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Fall")
	float LedgeFallHorizontalVelocityKeepRatio = 0.15f;

	// 向き制御
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Facing")
	float FacingTurnSpeed = 360.0f;

	// 右向きのYaw
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Facing")
	float RightFacingYaw = 90.0f;

	// 左向きのYaw
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Facing")
	float LeftFacingYaw = -90.0f;

	// 目標Yaw
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Facing")
	float TargetFacingYaw = 90.0f;

	// 背中側経由で回すか
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Facing")
	bool bRotateThroughBack = true;

	// =============================
	// Smooth Turn System
	// =============================

	// 見た目上の現在Yaw（360度をまたいで連続回転できるようにアンラップ値で持つ）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float VisualYaw = 90.0f;

	// 現在向いている入力方向（-1: 左, +1: 右）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float FacingSign = 1.0f;

	// ターン中か
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bIsTurning = false;

	// ターン終了時に向く方向
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float PendingFacingSign = 1.0f;

	// ターン先のYaw（アンラップ値）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float TurnTargetYaw = 90.0f;

	// ターン速度（度/秒）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float TurnSpeedDegreesPerSecond = 720.0f;

	// 入力方向に応じてターン開始
	void UpdateTurnFromInput(float RightInput);

	// ターン開始
	void StartTurnToSign(float NewFacingSign);

	// 空中上昇中に入力された攻撃を頂点で出すための予約
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack|Air")
	bool bAirAttackQueued = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack|Air")
	FAttackData QueuedAirAttackData;

	// 攻撃開始時の空中/地上状態
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack")
	bool bAttackStartedInAir = false;

	// 頂点判定用
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Air")
	float AirAttackApexThresholdZ = 20.0f;

	bool IsRisingInAir() const;
	bool IsFallingInAir() const;
	void QueueAirAttack(const FAttackData& AttackData);
	void TryExecuteQueuedAirAttackAtApex();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Hit")
	float PunchHitRadius = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Hit")
	FVector PunchHitOffset = FVector(0.0f, 50.0f, 45.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Hit")
	float KickHitRadius = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Hit")
	FVector KickHitOffset = FVector(0.0f, 100.0f, 45.0f);

	UPROPERTY()
	TArray<TObjectPtr<AActor>> HitActorsThisSwing;

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpEnd();

	/** 将来の当たり判定開始用フック */
	UFUNCTION(BlueprintCallable, Category = "Attack")
	virtual void BeginAttackHitCheck();

	/** 将来の当たり判定終了用フック */
	UFUNCTION(BlueprintCallable, Category = "Attack")
	virtual void EndAttackHitCheck();

	/** StartMoveアニメの踏み込み開始で呼ぶ */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void UnlockMoveStart();

	/** AnimBPから入力状態参照用 */
	UFUNCTION(BlueprintPure, Category = "Movement")
	bool HasMoveInput() const { return bHasMoveInput; }

	/** AnimBPから歩き出し状態参照用 */
	UFUNCTION(BlueprintPure, Category = "Movement")
	bool IsMoveStarting() const { return bIsMoveStarting; }

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};