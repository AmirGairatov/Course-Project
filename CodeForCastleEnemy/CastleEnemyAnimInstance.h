// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "CastleEnemyAnimInstance.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class ECastleEnemyState : uint8
{
	Spawn,
	Locomotion,
	Die,
	Attack1,
	Attack1Recovery,
	Attack2,
	Attack2Recovery,
	Attack3,
	Attack3Recovery,
	Attack4,
	Buff,
	Hit,
	Dodge
};

UCLASS()
class COURSEPROJECT_API UCastleEnemyAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Params")
	float Speed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Params")
	float Direction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Params")
	ECastleEnemyState State;

	UFUNCTION(BlueprintCallable)
	void OnStateAnimationEnds();

	UFUNCTION(BlueprintCallable)
	void OnWeaponSocketChanged();

	UFUNCTION(BlueprintCallable)
	void Buff();
};
