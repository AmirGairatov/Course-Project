// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NiagaraSystem.h"
#include "PlayerWeapon.h"
#include "EnemyHealthBar.h"
#include "CastleEnemy.generated.h"

UCLASS()
class COURSEPROJECT_API ACastleEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ACastleEnemy();
	UPROPERTY(EditAnywhere, Category = "Enemy Params")
	int HealthPoints = 500;

	UPROPERTY(EditAnywhere, Category = "Enemy Params")
	int BasicDamage = 100;

	UPROPERTY(EditAnywhere, Category = "Enemy Params")
	float AttackInterval = 3.0f;

	UPROPERTY(EditAnywhere, Category = "Enemy Params")
	float AttackRange = 140.0f;

	UPROPERTY(EditAnywhere, Category = "Enemy Params")
	float HitRecoverInterval =1.3f;
	UPROPERTY(EditAnywhere, Category = "Enemy Params")
	float AttackRecoverInterval = 1.5f;
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float LoseSightDelay = 2.0f;

	UFUNCTION()
	bool CanAttack();

	float LostSightTime = 2.0f;
	FTimerHandle LoseSightTimerHandle;

	UFUNCTION()
	void OnLosePlayer();
	bool bCanSeePlayer = false;

	float _HitRecoverCountingDown;
	float _AttackRecoverCountingDown;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	float _AttackCountongDown;
	int _HealthPoints;
	bool bIsHeavyAttacking = false;
	bool bIsDamageDealed = false;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<APlayerWeapon> _WeaponClass;

	bool bShouldBuff = false;
	bool bIsBuffed = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	UNiagaraSystem* BuffEffectNiagara;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	UNiagaraSystem* SwordBuffEffectNiagara;

public:	
	APawn* _ChasedTarget = nullptr;
	APlayerWeapon* _Weapon;
	bool bIsComboPossible = false;
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	bool bIsAttacking = true;
	bool bAreHit = false;
	bool bIsInHitRecover = true;
	bool bIsInAttackRecover = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UUserWidget* HealthBarWidget;
	void DieProcess();
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void Chase(APawn* targetPawn);

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void Retreat(APawn* TargetPawn, float RetreatDistance);

	UFUNCTION()
	void OnSeePlayer(APawn* SensedPawn);

	UFUNCTION()
	void Attack();

	UFUNCTION()
	void RecoverFromAttack();

	UFUNCTION()
	void ChangeWeaponSocket();

	UFUNCTION()
	void SpawnProcessEnds();

	UFUNCTION()
	void ActivateBuff();

	void SetAllParams(int health, int damage, float attackInterval);

	UFUNCTION()
	void OnStatsChanged();

	int AttackIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	UNiagaraSystem* HitEffectNiagara;

	void Hit(int damage);

	int GetEnemyDamage();
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UPawnSensingComponent* PawnSensingComponent;

	int HitRecived = 0;

};
