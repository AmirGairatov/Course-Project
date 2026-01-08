// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CourseProjectStateBase.generated.h"

/**
 * 
 */
UCLASS()
class COURSEPROJECT_API ACourseProjectStateBase : public AGameStateBase
{
	GENERATED_BODY()
public:
	ACourseProjectStateBase();

	UPROPERTY(BlueprintReadOnly, Category = "Timer")
	int32 RemainingSeconds;

	FTimerHandle CountdownTimerHandle;

	virtual void BeginPlay() override;
	void StartCountdown();
	void AdvanceTimer();


	UPROPERTY(BlueprintReadWrite)
	int32 EnemiesEliminated=0;

	UPROPERTY(BlueprintReadWrite)
	int32 ArtefactsRemaining=3;

	UPROPERTY(BlueprintReadWrite)
	int32 NumberOfLLMErrors = 0;

};
