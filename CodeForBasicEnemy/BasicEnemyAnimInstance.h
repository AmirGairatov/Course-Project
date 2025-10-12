// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "BasicEnemyAnimInstance.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class EBasicEnemyState : uint8
{
	Spawn,
	Locomotion,
	Attack,
	AttackUP,
	HeavyAttack,
	Hit,
	Die
};

UCLASS()
class COURSEPROJECT_API UBasicEnemyAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Params")
	float Speed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Params")
	float Direction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Params")
	EBasicEnemyState State;

	UFUNCTION(BlueprintCallable)
	void OnStateAnimationEnds();
};
