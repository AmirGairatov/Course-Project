// Fill out your copyright notice in the Description page of Project Settings.


#include "BasicEnemyController.h"
#include "Math/UnrealMathUtility.h"
#include "BasicEnemy.h"

void ABasicEnemyController::MakeAttackDecision(APawn* targetPawn)
{
	auto controlledCharacter = Cast<ABasicEnemy>(GetPawn());
	auto dist = FVector::Dist2D(targetPawn->GetActorLocation(), GetPawn()->GetTargetLocation());
	if (dist <= controlledCharacter->AttackRange && controlledCharacter->CanAttack() && controlledCharacter)
	{

		controlledCharacter->AttackUP();

		int32 AttackType = FMath::RandRange(0, 2);

        switch (AttackType)
        {
        case 0:
            controlledCharacter->Attack();
            break;
        case 1:
            controlledCharacter->AttackUP();
            break;
        case 2:
            controlledCharacter->HeavyAttack();
            break;
        default:
            controlledCharacter->Attack();
            break;
        }
	}
}