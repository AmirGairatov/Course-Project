// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyHealthBar.h"
#include "NiagaraSystem.h"
#include "BasicEnemy.generated.h"

UCLASS()
class COURSEPROJECT_API ABasicEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABasicEnemy();

	UPROPERTY(EditAnywhere, Category="Enemy Params")
	int HealthPoints = 5000;

	UPROPERTY(EditAnywhere, Category = "Enemy Params")
	float AttackInterval = 2.7f;
	UPROPERTY(EditAnywhere, Category = "Enemy Params")
	float AttackUPInterval = 2.5f;

	UPROPERTY(EditAnywhere, Category = "Enemy Params")
	float HeavyAttackInterval = 3.1f;

	UPROPERTY(EditAnywhere, Category = "Enemy Params")
	float HitAnimInterval = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Enemy Params")
	int AttackRange = 200.0f;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	int _HealthPoints;
	float _AttackCountingDown;
	float _HitAnimCountingDown;
	bool bIsAttacking = false;
	bool bAreHit = false;
	APawn* _ChasedTarget = nullptr;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

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

	UFUNCTION()
	void OnSeePlayer(APawn* SensedPawn);

	UFUNCTION()
	bool IsTargetInFront(float AngleThresholdDegrees) const;
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	class UPawnSensingComponent* PawnSensingComponent;

};
