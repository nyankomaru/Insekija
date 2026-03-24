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

/**
 * 攻撃種別
 */
UENUM(BlueprintType)
enum class EAttackType : uint8
{
	None	UMETA(DisplayName = "None"),
	Punch	UMETA(DisplayName = "Punch"),
	Kick	UMETA(DisplayName = "Kick")
};

/**
 * 攻撃再生用データ
 *
 * AttackType  : 攻撃種別
 * Montage     : 再生するモンタージュ
 * SectionName : 再生するセクション名
 * Damage      : 攻撃ダメージ
 */
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
 * プレイヤーキャラクタークラス
 *
 * 主な役割:
 * ・移動、ジャンプ、視点操作などのプレイヤー入力処理
 * ・パンチ / キック / コンボを含む攻撃制御
 * ・空中攻撃予約や着地処理を含むジャンプ挙動管理
 * ・歩き出しロックや入力補間による移動開始制御
 * ・2.5Dアクション向けの左右向き制御とスムーズターン
 *
 * 想定ゲーム:
 * ・固定カメラ式の3D風2Dアクションゲーム
 * ・画面固定の横移動アクション
 */
UCLASS(Abstract)
class AInsekiCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	/** Constructor */
	AInsekiCharacter();

protected:
	// ==================================================
	// Engine Override
	// ==================================================

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	// ==================================================
	// Public Interface
	// ==================================================

	/** 現在攻撃中のダメージ値を取得する */
	UFUNCTION(BlueprintPure, Category = "Attack")
	float GetCurrentAttackDamage() const { return CurrentAttackDamage; }

	/** UIや外部入力から移動を受け付ける */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoMove(float Right, float Forward);

	/** UIや外部入力から視点入力を受け付ける */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** UIや外部入力からジャンプ開始を受け付ける */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpStart();

	/** UIや外部入力からジャンプ終了を受け付ける */
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

	/** CameraBoom取得 */
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** FollowCamera取得 */
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

protected:
	// ==================================================
	// Camera Component
	// ==================================================

	/** プレイヤー背後に配置するカメラブーム */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** 実際の追従カメラ */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

protected:
	// ==================================================
	// Input Action
	// ==================================================

	/** ジャンプ入力 */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* JumpAction;

	/** 移動入力 */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MoveAction;

	/** 視点入力 */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* LookAction;

	/** マウス視点入力 */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MouseLookAction;

	/** パンチ入力 */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* PunchAction;

	/** キック入力 */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* KickAction;

protected:
	// ==================================================
	// Attack Animation Data
	// ==================================================

	/** パンチモンタージュ（Punch1 / Punch2 / Punch3 を想定） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	TObjectPtr<UAnimMontage> PunchMontage;

	/** キックモンタージュ（Kick1 を想定） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	TObjectPtr<UAnimMontage> KickMontage;

protected:
	// ==================================================
	// Attack State
	// ==================================================

	/** 現在攻撃中かどうか */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack")
	bool bIsAttacking = false;

	/** 現在の攻撃種別 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack")
	EAttackType CurrentAttackType = EAttackType::None;

	/** 現在攻撃中のダメージ値 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack")
	float CurrentAttackDamage = 0.0f;

	/** 次に出すパンチ番号（1～3ループ） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack")
	int32 NextPunchIndex = 1;

	/** 現在実行中のパンチ番号 */
	int32 CurrentPunchIndex = 0;

	/** 次段パンチが予約されているか */
	bool bPunchQueued = false;

	/** 次段パンチ受付中か */
	bool bCanQueueNextPunch = false;

	/** 攻撃開始時の状態が空中だったか */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack")
	bool bAttackStartedInAir = false;

protected:
	// ==================================================
	// Air Attack Control
	// ==================================================

	/** 空中上昇中に入力された攻撃を頂点で出すための予約フラグ */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack|Air")
	bool bAirAttackQueued = false;

	/** 頂点到達時に実行する予約攻撃データ */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack|Air")
	FAttackData QueuedAirAttackData;

	/** 頂点判定用のZ速度閾値 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Air")
	float AirAttackApexThresholdZ = 20.0f;

protected:
	// ==================================================
	// Attack Hit Parameter
	// ==================================================

	/** パンチ当たり判定半径 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Hit")
	float PunchHitRadius = 45.0f;

	/** パンチ当たり判定オフセット */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Hit")
	FVector PunchHitOffset = FVector(0.0f, 50.0f, 45.0f);

	/** キック当たり判定半径 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Hit")
	float KickHitRadius = 100.0f;

	/** キック当たり判定オフセット */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Hit")
	FVector KickHitOffset = FVector(0.0f, 100.0f, 45.0f);

	/** 1回の攻撃中に既にヒットしたActor一覧 */
	UPROPERTY()
	TArray<TObjectPtr<AActor>> HitActorsThisSwing;

protected:
	// ==================================================
	// Attack Function
	// ==================================================

	/** 通常移動入力処理 */
	void Move(const FInputActionValue& Value);

	/** パンチ入力時処理 */
	void OnPunchPressed();

	/** キック入力時処理 */
	void OnKickPressed();

	/** 汎用攻撃開始処理 */
	virtual bool TryStartAttack(const FAttackData& AttackData);

	/** パンチ攻撃開始 */
	virtual void StartPunchAttack();

	/** キック攻撃開始 */
	virtual void StartKickAttack();

	/** 攻撃モンタージュ終了時 */
	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	/** パンチ次段受付開始 */
	UFUNCTION(BlueprintCallable, Category = "Attack")
	void OpenPunchQueueWindow();

	/** パンチ次段受付終了 */
	UFUNCTION(BlueprintCallable, Category = "Attack")
	void ClosePunchQueueWindow();

	/** Notify開始時処理 */
	UFUNCTION()
	void OnMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& Payload);

	/** 上昇中かどうか */
	bool IsRisingInAir() const;

	/** 落下中かどうか */
	bool IsFallingInAir() const;

	/** 空中攻撃を予約する */
	void QueueAirAttack(const FAttackData& AttackData);

	/** 頂点到達時に予約攻撃を実行する */
	void TryExecuteQueuedAirAttackAtApex();

protected:
	// ==================================================
	// Jump System
	// ==================================================

	/** ジャンプ中の垂直初速 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump")
	float JumpVerticalVelocity = 540.0f;

	/** 通常時の重力スケール */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump")
	float NormalGravityScale = 1.6f;

	/** 上昇中の重力スケール */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump")
	float JumpRiseGravityScale = 1.2f;

	/** 落下中の重力スケール */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump")
	float JumpFallGravityScale = 2.8f;

	/** 空中横移動の強さ */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump")
	float JumpAirMoveScale = 1.0f;

	/** 段差落下時の空中横制御倍率 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Fall")
	float LedgeFallAirControlScale = 0.05f;

	/** 段差落下開始時に残す横速度割合 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Fall")
	float LedgeFallHorizontalVelocityKeepRatio = 0.15f;

	/** 着地硬直中かどうか */
	bool bLandingLock = false;

	/** 着地硬直タイマー */
	FTimerHandle LandingTimer;

	/** コヨーテタイム長さ */
	float CoyoteTime = 0.12f;

	/** コヨーテタイマー */
	float CoyoteTimer = 0.0f;

	/** ジャンプ入力バッファ時間 */
	float JumpBufferTime = 0.12f;

	/** ジャンプ入力バッファタイマー */
	float JumpBufferTimer = 0.0f;

	/** ジャンプ由来の落下中か */
	bool bIsJumpFalling = false;

	/** ジャンプボタン保持中か */
	bool bJumpHeld = false;

	/** 前フレームで落下中だったか */
	bool bWasFallingLastFrame = false;

protected:
	// ==================================================
	// Movement Start / Stop System
	// ==================================================

	/** 歩き出しロック中か */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bMoveStartLocked = false;

	/** 歩き出し中か */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bIsMoveStarting = false;

	/** 移動入力が存在するか */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bHasMoveInput = false;

	/** 最後の移動入力方向 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float LastMoveInput = 0.0f;

	/** 現在の移動入力スケール */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float CurrentMoveInputScale = 0.0f;

	/** StartMove解除後の入力立ち上がり補間値 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float MoveStartReleaseBlend = 1.0f;

	/** StartMove解除後の補間速度 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float MoveStartReleaseInterpSpeed = 10.0f;

	/** 通常移動補間速度 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float MoveInputInterpSpeed = 8.0f;

	/** 歩き出し判定速度閾値 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float MoveStartSpeedThreshold = 5.0f;

	/** 入力無効扱いにするデッドゾーン */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float MoveInputDeadZone = 0.05f;

	/** 歩き出しロックの安全解除時間 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float MoveStartSafetyUnlockTime = 0.10f;

	/** 歩き出し開始済みかどうか */
	bool bMoveStartConsumed = false;

	/** 安全解除タイマー */
	FTimerHandle MoveStartSafetyUnlockTimer;

	/** 歩き出し開始 */
	void BeginMoveStart();

	/** 歩き出し状態リセット */
	void ResetMoveStartState();

protected:
	// ==================================================
	// Facing / Turn System
	// ==================================================

	/** 左右反転の回転速度 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Facing")
	float FacingTurnSpeed = 360.0f;

	/** 右向きYaw */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Facing")
	float RightFacingYaw = 90.0f;

	/** 左向きYaw */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Facing")
	float LeftFacingYaw = -90.0f;

	/** 目標Yaw */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Facing")
	float TargetFacingYaw = 90.0f;

	/** 背中側経由で回転するか */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Facing")
	bool bRotateThroughBack = true;

	/** 見た目上の現在Yaw（アンラップ値） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float VisualYaw = 90.0f;

	/** 現在向いている入力方向（-1: 左, +1: 右） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float FacingSign = 1.0f;

	/** ターン中かどうか */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bIsTurning = false;

	/** ターン終了後に向く方向 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float PendingFacingSign = 1.0f;

	/** ターン先Yaw（アンラップ値） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float TurnTargetYaw = 90.0f;

	/** ターン速度（度/秒） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float TurnSpeedDegreesPerSecond = 720.0f;

	/** 入力方向に応じてターン状態を更新する */
	void UpdateTurnFromInput(float RightInput);

	/** 指定方向へのターンを開始する */
	void StartTurnToSign(float NewFacingSign);
};