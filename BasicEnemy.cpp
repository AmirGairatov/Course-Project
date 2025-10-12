// Fill out your copyright notice in the Description page of Project Settings.


#include "BasicEnemy.h"
#include "Perception/PawnSensingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BasicEnemyController.h"
#include "BasicEnemyAnimInstance.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

// Sets default values
ABasicEnemy::ABasicEnemy()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PawnSensingComponent = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("PawnSensor"));
}

// Called when the game starts or when spawned
void ABasicEnemy::BeginPlay()
{
	Super::BeginPlay();
	_HealthPoints = HealthPoints;
	if (PawnSensingComponent) {
		PawnSensingComponent->OnSeePawn.AddDynamic(this, &ABasicEnemy::OnSeePlayer);
	}
	// Включить плавное ориентирование по направлению движения
	GetCharacterMovement()->bOrientRotationToMovement = true;
	// Отключить мгновенный поворот контроллера
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	bUseControllerRotationYaw = false;

	// Настройка скорости вращения (чем меньше, тем плавнее)
	GetCharacterMovement()->RotationRate = FRotator(0.f, 240.f, 0.f);

    if (HealthBarWidget)
    {
        OnStatsChanged();
    }

}

// Called every frame
void ABasicEnemy::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    auto animInst = Cast<UBasicEnemyAnimInstance>(GetMesh()->GetAnimInstance());
    if (!animInst)
        return;

    FVector Velocity = GetCharacterMovement()->Velocity;
    float TargetSpeed = Velocity.Size();
    animInst->Speed = FMath::FInterpTo(animInst->Speed, TargetSpeed, DeltaTime, 8.0f);

    if (Velocity.SizeSquared() > KINDA_SMALL_NUMBER)
    {
        const FRotator ControlRotation = Controller ? Controller->GetControlRotation() : FRotator::ZeroRotator;
        const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
        const FVector ForwardVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        FVector MovementDirection = Velocity.GetSafeNormal();

        float DotProduct = FVector::DotProduct(ForwardVector, MovementDirection);
        float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(DotProduct, -1.0f, 1.0f)));

        FVector CrossProduct = FVector::CrossProduct(ForwardVector, MovementDirection);
        if (CrossProduct.Z < 0.0f)
        {
            Angle = -Angle;
        }

        animInst->Direction = Angle;
    }
    else
    {
        animInst->Direction = 0.0f;
    }

    if (_HitAnimCountingDown > 0.0f)
    {
        _HitAnimCountingDown -= DeltaTime;
    }
    else
    {
        bAreHit = false;
    }
    if (_AttackCountingDown > 0.0f)
    {
        _AttackCountingDown -= DeltaTime;
    }
    else
    {
        bIsAttacking = false;
    }

    if (_ChasedTarget && !bIsAttacking)
    {
        FVector ToTarget = _ChasedTarget->GetActorLocation() - GetActorLocation();
        ToTarget.Z = 0.0f;

        if (ToTarget.SizeSquared() > KINDA_SMALL_NUMBER)
        {
            FRotator CurrentRotation = GetActorRotation();
            FRotator TargetRotation = ToTarget.Rotation();

            FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, 5.0f);
            SetActorRotation(NewRotation);
        }
    }

    if (_ChasedTarget && animInst->State == EBasicEnemyState::Locomotion && !bAreHit)
    {
        FVector ToTarget = _ChasedTarget->GetActorLocation() - GetActorLocation();
        ToTarget.Z = 0.0f;

        FRotator TargetRotation = ToTarget.Rotation();
        float AngleToTarget = FMath::Abs(FRotator::NormalizeAxis(GetActorRotation().Yaw - TargetRotation.Yaw));
        constexpr float AttackAngleThreshold = 15.0f;

        if (AngleToTarget < AttackAngleThreshold)
        {
            auto enemyController = Cast<ABasicEnemyController>(GetController());
            if (enemyController)
            {
                enemyController->MakeAttackDecision(_ChasedTarget);
            }
        }
    }
}
bool ABasicEnemy::IsTargetInFront(float AngleThresholdDegrees) const
{
	if (!_ChasedTarget) return false;

	FVector Forward = GetActorForwardVector();
	FVector ToTarget = (_ChasedTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();

	float Dot = FVector::DotProduct(Forward, ToTarget);
	float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(Dot));
	return AngleDegrees < AngleThresholdDegrees;
}
int ABasicEnemy::GetHealthPoints()
{
	return _HealthPoints;
}
bool ABasicEnemy::IsKilled()
{
	return (_HealthPoints <= 0.0f);
}

bool ABasicEnemy::CanAttack()
{
	auto animInst = GetMesh()->GetAnimInstance();
	auto enemyAnimInst = Cast<UBasicEnemyAnimInstance>(animInst);
	return(_AttackCountingDown <= 0.0f && enemyAnimInst->State == EBasicEnemyState::Locomotion && !bIsAttacking);
}

void ABasicEnemy::Chase(APawn* targetPawn)
{
	USkeletalMeshComponent* meshComp = GetMesh();

	UAnimInstance* animInst = meshComp->GetAnimInstance();
	UBasicEnemyAnimInstance* enemyAnimInst = Cast<UBasicEnemyAnimInstance>(animInst);

	if (targetPawn && enemyAnimInst->State == EBasicEnemyState::Locomotion && !bAreHit)
	{
		AController* controller = GetController();
		ABasicEnemyController* enemyController = Cast<ABasicEnemyController>(controller);
		//UE_LOG(LogTemp, Error, TEXT("Chasing"));
            
		enemyController->MoveToActor(targetPawn, 90.0f);
	}
	
	_ChasedTarget = targetPawn;
}

void ABasicEnemy::Attack()
{
    StartAttack(AttackInterval);
}

void ABasicEnemy::AttackUP()
{
    StartAttack(AttackUPInterval);
}

void ABasicEnemy::HeavyAttack()
{
    StartAttack(HeavyAttackInterval);
}
void ABasicEnemy::StartAttack(float attackInterval)
{
    GetController()->StopMovement();
    bIsAttacking = true;
    auto animInst = Cast<UBasicEnemyAnimInstance>(GetMesh()->GetAnimInstance());
    if (animInst)
    {
        if (attackInterval == AttackInterval)
        {
            animInst->State = EBasicEnemyState::Attack;
            _AttackCountingDown = AttackInterval;
        }
        else if (attackInterval == AttackUPInterval)
        {
            animInst->State = EBasicEnemyState::AttackUP;
            _AttackCountingDown = AttackUPInterval;
        }
        else
        {
            animInst->State = EBasicEnemyState::HeavyAttack;
            _AttackCountingDown = HeavyAttackInterval;
        }

    }
}
void ABasicEnemy::Hit(int damage)
{
	_HealthPoints -= damage;
    auto animInst = GetMesh()->GetAnimInstance();
    auto enemyAnimInst = Cast<UBasicEnemyAnimInstance>(animInst);
    OnStatsChanged();
    _HitAnimCountingDown = HitAnimInterval;

    if (HitEffectNiagara)  
    {
        FName HitSocketName(TEXT("FX_ChestFlame"));
        UNiagaraFunctionLibrary::SpawnSystemAttached(
            HitEffectNiagara,
            GetMesh(),
            HitSocketName, 
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget,
            true                         
        );
    }

    if (_HealthPoints <=0)
    {

        enemyAnimInst->State = EBasicEnemyState::Die;
    }
    else
    {
        if (enemyAnimInst->State == EBasicEnemyState::Locomotion && !bAreHit)
        {
            UE_LOG(LogTemp, Log, TEXT("Hit animation plays"));
            bAreHit = true;
            enemyAnimInst->State = EBasicEnemyState::Hit;
        }
    }
}

void ABasicEnemy::DieProcess()
{
	PrimaryActorTick.bCanEverTick = false;
	K2_DestroyActor();
	GEngine->ForceGarbageCollection(true);
}

void ABasicEnemy::OnSeePlayer(APawn* SensedPawn) {
	if (SensedPawn && !IsKilled()) {
		Chase(SensedPawn);
	}
}

void ABasicEnemy::OnStatsChanged()
{
    if (HealthBarWidget!=nullptr)
    {
        float HealthPercent = FMath::Clamp((float)_HealthPoints / (float)HealthPoints, 0.0f, 1.0f);
        auto healthBar = Cast<UEnemyHealthBar>(HealthBarWidget);
        healthBar->HealthProgressBar->SetPercent(HealthPercent);
    }
}