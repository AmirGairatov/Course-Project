// Fill out your copyright notice in the Description page of Project Settings.


#include "CourseProjectGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "CourseProjectGameMode.h"

UCourseProjectGameInstance::UCourseProjectGameInstance(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    static ConstructorHelpers::FClassFinder<UUserWidget> LoadingScreenBPClass(TEXT("/Game/ThirdPerson/Blueprints/WBP_LoadingScreen"));
    if (LoadingScreenBPClass.Succeeded())
    {
        LoadingScreenWidgetClass = LoadingScreenBPClass.Class;
    }
    else
    {
        LoadingScreenWidgetClass = nullptr;
        UE_LOG(LogTemp, Warning, TEXT("Failed to find LoadingScreenWidgetClass!"));
    }
}
void UCourseProjectGameInstance::StartGame()
{
    ShowLoadingScreen();

    UWorld* World = GEngine ? GEngine->GetCurrentPlayWorld() : nullptr;
    if (World)
    {
        World->GetTimerManager().SetTimer(LoadingDelayTimerHandle, [this, World]()
            {
                UGameplayStatics::OpenLevel(World, "MainWorld");
            }, 0.2f, false);
    }
}

void UCourseProjectGameInstance::LeaveGame()
{
    UWorld* World = GEngine->GetCurrentPlayWorld();
    if (!World)
    {
        return;
    }

    ACourseProjectGameMode* GameMode = Cast<ACourseProjectGameMode>(World->GetAuthGameMode());
    if (GameMode)
    {
        GameMode->SaveSessionStatsToLog();  
    }

    UGameplayStatics::OpenLevel(World, FName("LobbyMap"));  
}

void UCourseProjectGameInstance::ShowLoadingScreen()
{
    if (LoadingScreenWidgetClass && !LoadingScreenWidget)
    {
        LoadingScreenWidget = CreateWidget<UUserWidget>(this, LoadingScreenWidgetClass);
        if (LoadingScreenWidget)
        {
            LoadingScreenWidget->AddToViewport(100);
        }
    }
}

void UCourseProjectGameInstance::HideLoadingScreen()
{
    if (LoadingScreenWidget)
    {
        LoadingScreenWidget->RemoveFromParent();
        LoadingScreenWidget = nullptr;
    }
}

void UCourseProjectGameInstance::ExitToDesktop()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    APlayerController* PC = World->GetFirstPlayerController();

    UKismetSystemLibrary::QuitGame(World, PC, EQuitPreference::Quit, false);
}