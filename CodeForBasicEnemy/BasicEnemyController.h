// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BasicEnemyController.generated.h"

/**
 * 
 */
UCLASS()
class COURSEPROJECT_API ABasicEnemyController : public AAIController
{
	GENERATED_BODY()
public:
	void MakeAttackDecision(APawn* targetPawn);
};
