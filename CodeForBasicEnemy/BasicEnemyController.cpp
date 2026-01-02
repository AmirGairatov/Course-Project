// Fill out your copyright notice in the Description page of Project Settings.


#include "BasicEnemyController.h"
#include "Math/UnrealMathUtility.h"
#include "NavigationSystem.h"
#include "BasicEnemy.h"

void ABasicEnemyController::MakeAttackDecision(APawn* targetPawn)
{
	auto controlledCharacter = Cast<ABasicEnemy>(GetPawn());
	auto dist = FVector::Dist2D(targetPawn->GetActorLocation(), GetPawn()->GetTargetLocation());
	if (dist <= controlledCharacter->AttackRange && controlledCharacter->CanAttack())
	{

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
void ABasicEnemyController::StrafeAroundTarget(APawn* TargetPawn, float StrafeRadius, float StrafeSpeed)
{
    APawn* ControlledPawn = GetPawn();
    if (!TargetPawn || !ControlledPawn)
        return;

    constexpr float MinAngle = -60.f;
    constexpr float MaxAngle = 60.f;

    if (!bStrafeDirectionRightInitialized)
    {
        bStrafeDirectionRight = true;
        bStrafeDirectionRightInitialized = true;
    }

    float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.016f;
    float AngleStep = StrafeSpeed * DeltaSeconds * (bStrafeDirectionRight ? 1.f : -1.f);
    CurrentStrafeAngle += AngleStep;

    if (CurrentStrafeAngle > MaxAngle)
    {
        CurrentStrafeAngle = MaxAngle;
        bStrafeDirectionRight = false;
    }
    else if (CurrentStrafeAngle < MinAngle)
    {
        CurrentStrafeAngle = MinAngle;
        bStrafeDirectionRight = true;
    }

    FVector ToTarget = TargetPawn->GetActorLocation() - ControlledPawn->GetActorLocation();
    ToTarget.Z = 0.f;
    ToTarget.Normalize();

    FVector RightVector = FVector::CrossProduct(FVector::UpVector, ToTarget);

    float RadAngle = FMath::DegreesToRadians(CurrentStrafeAngle);
    FVector Offset = ToTarget * StrafeRadius * FMath::Cos(RadAngle) + RightVector * StrafeRadius * FMath::Sin(RadAngle);

    FVector DesiredLocation = TargetPawn->GetActorLocation() + Offset;

    FNavLocation NavLocation;
    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
    if (NavSys && NavSys->ProjectPointToNavigation(DesiredLocation, NavLocation, FVector(50, 50, 50)))
    {
        DesiredLocation = NavLocation.Location;
    }

    MoveToLocation(DesiredLocation, 10.f);

#if !UE_BUILD_SHIPPING
    DrawDebugSphere(GetWorld(), DesiredLocation, 30.f, 12, FColor::Green, false, 0.1f);
#endif
}
