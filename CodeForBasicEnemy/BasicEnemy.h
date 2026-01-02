// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyHealthBar.h"
#include "Components/CapsuleComponent.h"
#include "NiagaraSystem.h"
#include "Sound/SoundCue.h"
#include "BasicEnemy.generated.h"

UCLASS()
class COURSEPROJECT_API ABasicEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABasicEnemy();

	UPROPERTY(EditAnywhere, Category="Enemy Params")
	int HealthPoints = 100;

	UPROPERTY(EditAnywhere, Category = "Enemy Params")
	int BasicDamage = 100;

	UPROPERTY(EditAnywhere, Category = "Enemy Params")
	float AttackInterval = 2.15f;
	UPROPERTY(EditAnywhere, Category = "Enemy Params")
	float AttackUPInterval = 2.16f;

	UPROPERTY(EditAnywhere, Category = "Enemy Params")
	float HeavyAttackInterval = 2.23f;

	UPROPERTY(EditAnywhere, Category = "Enemy Params")
	float HitAnimInterval = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Enemy Params")
	int AttackRange = 200.0f;

	UPROPERTY(EditAnywhere, Category = "Enemy Params")
	float AttackRecoverInterval = 1.5f;

	float _AttackRecoverCountingDown;

	bool bIsInAttackRecover = false;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float LoseSightDelay = 2.0f;

	UFUNCTION()
	void OnLosePlayer();

	float LostSightTime = 0.0f;
	FTimerHandle LoseSightTimerHandle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	UCapsuleComponent* LeftFistCapsule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	UCapsuleComponent* RightFistCapsule;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	UNiagaraSystem* BuffEffectNiagara;

	int _HealthPoints;
	float _AttackCountingDown;
	float _HitAnimCountingDown;
	bool bIsHeavyAttacking = false;
	bool bIsDamageDealed = false;

	UFUNCTION()
	void OnFistBeginOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UPROPERTY(EditAnywhere, Category = "Audio")
	USoundCue* MySoundCue;

	UPROPERTY(EditAnywhere, Category = "Audio")
	USoundCue* PunchCue;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	bool bAreHit = true;
	bool bIsAttacking = false;
	bool bIsBuffed = false;
	APawn* _ChasedTarget = nullptr;
	UFUNCTION(BlueprintCallable, Category = "Enemy", meta = (DisplayName="Get HP"))
	int GetHealthPoints();
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	bool IsKilled();
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	bool CanAttack();
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void Chase(APawn* targetPawn);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UUserWidget* HealthBarWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	UNiagaraSystem* HitEffectNiagara;

	UFUNCTION()
	void OnStatsChanged();
	void StartAttack(float attackInterval);
	void Attack();
	void AttackUP();
	void HeavyAttack();
	void Hit(int damage);
	void DieProcess();

	void ActivateBuff();

	UFUNCTION()
	void OnSeePlayer(APawn* SensedPawn);

	void SetAllParams(int health, int damage, float attackInterval);

	UFUNCTION()
	bool IsTargetInFront(float AngleThresholdDegrees) const;

	UFUNCTION(BlueprintCallable, Category = "Combat|Fists")
	void EnableFistCollision(bool bEnable);
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	class UPawnSensingComponent* PawnSensingComponent;

};
