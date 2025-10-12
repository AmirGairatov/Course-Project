// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "PlayerWeapon.h"
#include "UStatsWidget.h"
#include "BasicEnemy.h"
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

	void OnRollPressed();

	void OnAttackPressed();

	void OnHealPressed();

	bool bIsDodging = false;

	bool bIsHealing = false;


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

	bool bIsAttacking = false;

	/** Returns CameraBoom subobject **/
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return _springArmComponent; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return _cameraComponent; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UClass* _WeaponClass;
	APlayerWeapon* _Weapon;

	int _HealthPoints;
	float _StaminaPoints;
	float _AttackCountingDown;
	float _HealCountingDown;
	float _RollCountingDown = 0;

	void DieProcess();

	UPROPERTY()
	ABasicEnemy* LockedTarget;

	UPROPERTY(EditAnywhere, Category = "Target Lock")
	float TargetLockRadius = 1500.f;

	UPROPERTY(EditAnywhere, Category = "Target Lock")
	float TargetLockMaxAngle = 45.f;

	void OnTargetLockPressed();

	ABasicEnemy* FindBestTarget();
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;


	UPROPERTY(EditAnywhere, Category="PlayerCharacter params")
	int HealthPoints = 500;


	UPROPERTY(EditAnywhere, Category = "PlayerCharacter params")
	int StaminaPoints = 200;

	UPROPERTY(EditAnywhere, Category = "PlayerCharacter params")
	float AttackRange = 6.0f;

	UPROPERTY(EditAnywhere, Category = "PlayerCharacter params")
	float AttackInterval = 1.0f;

	UPROPERTY(EditAnywhere, Category = "PlayerCharacter params")
	float RollInterval = 1.15f;

	UPROPERTY(EditAnywhere, Category = "PlayerCharacter params")
	float HealInterval = 1.41f;

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

	UFUNCTION()
	void HandleOnMontageNotifyIvents(FName a_MotifyName, const FBranchingPointNotifyPayload& a_BranchingPayload);

	UFUNCTION()
	void OnStatsChanged();

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> StatsBarWidgetClass;


	// óêàçàòåëü íà ñîçäàííûé ýêçåìïëÿð
	UPROPERTY()
	UUStatsWidget* StatsBarWidgetInstance;

	void Hit(int damage);
};
