// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "CourseProjectGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class COURSEPROJECT_API UCourseProjectGameInstance : public UGameInstance
{
	GENERATED_BODY()
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> LoadingScreenWidgetClass;

	UPROPERTY()
	UUserWidget* LoadingScreenWidget;
public:
	UFUNCTION(BlueprintCallable, Category="CourseProject")
	void StartGame();

	UFUNCTION(BlueprintCallable, Category = "CourseProject")
	void LeaveGame();

	UFUNCTION(BlueprintCallable, Category = "CourseProject")
	void ExitToDesktop();

	void ShowLoadingScreen();
	void HideLoadingScreen();

	FTimerHandle LoadingDelayTimerHandle;

	UCourseProjectGameInstance(const FObjectInitializer& ObjectInitializer);

};
