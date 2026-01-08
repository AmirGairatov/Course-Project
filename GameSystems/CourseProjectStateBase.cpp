// Fill out your copyright notice in the Description page of Project Settings.


#include "CourseProjectStateBase.h"
#include "PlayerCharacter.h"
#include "CourseProjectGameMode.h"
ACourseProjectStateBase::ACourseProjectStateBase()
{
    RemainingSeconds = 1200; 
}

void ACourseProjectStateBase::BeginPlay()
{
    Super::BeginPlay();
    StartCountdown();
    UE_LOG(LogTemp, Error, TEXT("Begin Play launched"));
}

void ACourseProjectStateBase::StartCountdown()
{
    GetWorldTimerManager().SetTimer(CountdownTimerHandle, this, &ACourseProjectStateBase::AdvanceTimer, 1.0f, true);
}

void ACourseProjectStateBase::AdvanceTimer()
{
    if (RemainingSeconds > 0)
    {
        --RemainingSeconds;
        int32 Minutes = RemainingSeconds / 60;
        int32 Seconds = RemainingSeconds % 60;
        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        if (PC)
        {
            APawn* Pawn = PC->GetPawn();
            if (Pawn)
            {
                APlayerCharacter* PlayerChar = Cast<APlayerCharacter>(Pawn);
                if (PlayerChar)
                {
                    PlayerChar->OnTimerChanged(Minutes, Seconds);
                }
            }
        }
    }
    else
    {
        GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
        ACourseProjectGameMode* GameMode = Cast<ACourseProjectGameMode>(GetWorld()->GetAuthGameMode());
        if (GameMode)
        {
            GameMode->ShowWidget(false);
        }
    }
}
