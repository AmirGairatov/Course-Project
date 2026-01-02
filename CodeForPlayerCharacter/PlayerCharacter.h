
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "PlayerWeapon.h"
#include "UStatsWidget.h"
#include "BasicEnemy.h"
#include "YouDiedWidget.h" 
#include "Explosion.h"
#include "PlayerCharacter.generated.h"


class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UCLASS(Blueprintable)
class COURSEPROJECT_API APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* _springArmComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* _cameraComponent;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;


	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	FVector2D LastMovementInput = FVector2D::ZeroVector;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* m_pDodgeMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* m_pHealMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	UNiagaraSystem* BuffEffectNiagara;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	UNiagaraSystem* SwordBuffEffectNiagara;

	void OnRollPressed();

	void OnAttackPressed();

	void OnSpecialAttackPressed();

	void OnHealPressed();


	bool bIsDodging = false;


	bool bIsGamePaused = false;

	int ComboAttackIndex = 0;
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);


protected:

	virtual void NotifyControllerChanged() override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	// Sets default values for this character's properties
	APlayerCharacter();

	UFUNCTION()
	void ActivateBuff();

	bool bIsBuffed = false;
	bool bIsAttacking = true;

	bool bIsInvulnerable = false;

	bool bIsHit = false;	

	bool bIsHealing = false;

	bool bIsFalling = false;

	bool bIsInFallRecover = false;

	bool bIsInRespawnProcess = false;

	bool bHasRespawnBuff = false;

	/** Returns CameraBoom subobject **/
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return _springArmComponent; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return _cameraComponent; }

	UFUNCTION(BlueprintCallable)
	void OnPausePressed();

	UFUNCTION()
	void RespawnBuff();

	UPROPERTY(EditDefaultsOnly, Category = "Buffs")
	TSubclassOf<AExplosion> ExplosionBlueprintClass;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UYouDiedWidget> YouDiedWidgetClass;

	UPROPERTY()
	UYouDiedWidget* YouDiedWidget = nullptr;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> PauseWidgetClass;

	UPROPERTY()
	UUserWidget* PauseWidget = nullptr;

	FTimerHandle TimerHandle_FadeOut;
	FTimerHandle TimerHandle_Respawn;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<APlayerWeapon> _WeaponClass;
	APlayerWeapon* _Weapon;

	int _HealthPoints;
	float _StaminaPoints;
	float _AttackCountingDown;
	float _HealCountingDown;
	float _RollCountingDown = 0;
	float _HitCountingDown = 0;

	void DieProcess();

	UPROPERTY()
	ACharacter* LockedTarget;

	UPROPERTY(EditAnywhere, Category = "Target Lock")
	float TargetLockRadius = 1500.f;

	UPROPERTY(EditAnywhere, Category = "Target Lock")
	float TargetLockMaxAngle = 45.f;

	void OnTargetLockPressed();

	ACharacter* FindBestTarget();
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	int _HealItems;
	UPROPERTY(EditAnywhere, Category="PlayerCharacter params")
	int HealthPoints = 500;

	bool bIsInHitAnimation = false;
	UPROPERTY(EditAnywhere, Category = "PlayerCharacter params")
	int Damage = 100;

	UPROPERTY(EditAnywhere, Category = "PlayerCharacter params")
	int MaxHealItems = 5;

	UPROPERTY(EditAnywhere, Category = "PlayerCharacter params")
	int StaminaPoints = 150;

	UPROPERTY(EditAnywhere, Category = "PlayerCharacter params")
	float AttackRange = 6.0f;

	UPROPERTY(EditAnywhere, Category = "PlayerCharacter params")
	float AttackInterval = 1.0f;

	UPROPERTY(EditAnywhere, Category = "PlayerCharacter params")
	float SpecialAttackInterval = 3.3f;

	UPROPERTY(EditAnywhere, Category = "PlayerCharacter params")
	float RollInterval = 1.0f;

	UPROPERTY(EditAnywhere, Category = "PlayerCharacter params")
	float HealInterval = 1.8f;


	UPROPERTY(EditAnywhere, Category = "PlayerCharacter params")
	float HitInterval = 0.78f;

	UFUNCTION(BlueprintCallable, Category = "PlayerCharacter", meta = (DisplayName="Get HP"))
	int GetHealthPoints();
	UFUNCTION(BlueprintCallable, Category = "PlayerCharacter")
	bool IsKilled();
	UFUNCTION(BlueprintCallable, Category = "PlayerCharacter")
	bool CanAttack();
	UFUNCTION(BlueprintCallable, Category = "PlayerCharacter")
	bool CanRoll();
	UFUNCTION(BlueprintCallable, Category = "PlayerCharacter")
	bool CanHeal();
	UFUNCTION(BlueprintCallable, Category = "PlayerCharacter")
	int GetComboIndex();

	int GetPlayerDamage();

	int GetCurrentHealItems();

	UFUNCTION()
	void HandleOnMontageNotifyIvents(FName a_MotifyName, const FBranchingPointNotifyPayload& a_BranchingPayload);

	UFUNCTION()
	void OnStatsChanged();

	UFUNCTION()
	void OnHealItemsChanged();

	UFUNCTION()
	void OnTimerChanged(int32 minutes, int32 seconds);

	UFUNCTION()
	int InWhichLocation();

	void GiveHeal(int amount);

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> StatsBarWidgetClass;

	bool bIsDead = false;

	UFUNCTION()
	void ChangeWeaponSocket();

	UPROPERTY()
	UUStatsWidget* StatsBarWidgetInstance;

	void Hit(int damage, bool IsHeavyHit, const FVector& HitOrigin);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	UNiagaraSystem* HitEffectNiagara;


	void ShowYouDiedWidget();

	void HideYouDiedWidget();

	void Die();

	void PerformRespawn();
};
