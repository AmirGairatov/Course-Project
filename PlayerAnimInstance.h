// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PlayerAnimInstance.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class EPlayerState : uint8
{
	Locomotion = 0,
	Attack,
	Hit,
	Die,
	Roll,
	SecondAttack,
	ThirdAttack,
	Heal
};

UCLASS()
class COURSEPROJECT_API UPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Speed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Direction;


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPlayerState State;

	UFUNCTION(BlueprintCallable)
	void OnStateAnimationEnds();
	
};
