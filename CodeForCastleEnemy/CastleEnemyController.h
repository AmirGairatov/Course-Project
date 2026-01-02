// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CastleEnemyController.generated.h"

/**
 * 
 */
UCLASS()
class COURSEPROJECT_API ACastleEnemyController : public AAIController
{
	GENERATED_BODY()
	

public:
	void MakeAttackDecision(APawn* targetPawn);
};
