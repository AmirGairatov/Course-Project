// Fill out your copyright notice in the Description page of Project Settings.


#include "CastleEnemyController.h"
#include "Math/UnrealMathUtility.h"
#include "NavigationSystem.h"
#include "CastleEnemy.h"

void ACastleEnemyController::MakeAttackDecision(APawn* targetPawn)
{
    if (targetPawn)
    {
        auto controlledCharacter = Cast<ACastleEnemy>(GetPawn());
        auto dist = FVector::Dist2D(targetPawn->GetActorLocation(), GetPawn()->GetTargetLocation());
        FVector ToTarget = targetPawn->GetActorLocation() - controlledCharacter->GetActorLocation();
        ToTarget.Z = 0.0f;
        FRotator TargetRotation = ToTarget.Rotation();
        float AngleToTarget = FMath::Abs(FRotator::NormalizeAxis(controlledCharacter->GetOwner()->GetActorRotation().Yaw - TargetRotation.Yaw));
        constexpr float AttackAngleThreshold = 15.0f;
       
        if (dist <= controlledCharacter->AttackRange && controlledCharacter && controlledCharacter->AttackIndex == 0 && AngleToTarget < AttackAngleThreshold && !controlledCharacter->bAreHit && !controlledCharacter->bIsInAttackRecover)
        {
            UE_LOG(LogTemp, Error, TEXT("Attack"));
            controlledCharacter->Attack();
            /*controlledCharacter->AttackIndex += 1;*/
            
        }
        else if (controlledCharacter->AttackIndex > 0)
        {
            controlledCharacter->RecoverFromAttack();
        }
        else if (!controlledCharacter->bIsInHitRecover && dist > controlledCharacter->AttackRange && !controlledCharacter->bIsInAttackRecover)
        {
            controlledCharacter->AttackIndex = 0;
        }
        
    }
}
