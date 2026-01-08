// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LlamaComponent.h"
#include "TimerManager.h" 
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Crystal.h"
#include "CourseProjectGameMode.generated.h"


UCLASS(minimalapi)
class ACourseProjectGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACourseProjectGameMode();

	UFUNCTION()
	void SaveSessionStatsToLog();

	bool bIsHintExists = false;

	UFUNCTION()
	void GenerateWorldEvent();

	UFUNCTION()
	void SpawnHint();

	UFUNCTION()
	void RetargetAllEnemies();

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void SpawnEnemies(FString EnemyType, int32 Count, int HP, int Damage, FVector Center, float Radius, float attackInterval);

	void OnEnemyEliminated(AActor* DestroyedActor);

	UFUNCTION(BlueprintCallable)
	void ShowWidget(bool isWin);

	UFUNCTION(BlueprintCallable)
	void SpawnWidget(TSubclassOf<UUserWidget> WidgetClass, UUserWidget* WidgetInstance);

	void DecreaseArtefactAmoint();

	void Buff(bool bIsPlayer);

	UPROPERTY()
	TArray<AActor*> AliveEnemies;

	UPROPERTY()
	TArray<ACrystal*> CrystalArray;
protected:
	virtual void BeginPlay() override;

	void PingInternetOnce();

private:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> LoseWidgetClass;

	UPROPERTY()
	FString LastLLMPrompt;

	void LogLLMConversation(const FString& Prompt, const FString& Response);

	FString GameModeState;

	UPROPERTY()
	UUserWidget* LoseWidgetInstance;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> WinWidgetClass;

	UPROPERTY()
	UUserWidget* WinWidgetInstance;

	int32 ArtefactsRemains = 3;
	FTimerHandle WorldEventTimerHandle; 
	void OnLLMHttpResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	UPROPERTY(EditDefaultsOnly, Category = "Enemy")
	TSubclassOf<AActor> EnemyBPClass;
	TSubclassOf<AActor> CastleEnemyBPClass;
	TSubclassOf<AActor> CrystalBPClass;
	void TryStartWorldEventTimer(float timer);

	void HandleWorldEvent(const FString& Content);

	UFUNCTION(BlueprintCallable, Category = "Crystals")
	void SpawnCrystals(const TArray<FVector>& SpawnLocations);

	void GenerateWorldEvent_Deterministic();

};



