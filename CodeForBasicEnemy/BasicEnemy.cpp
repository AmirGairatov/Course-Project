// Fill out your copyright notice in the Description page of Project Settings.


#include "BasicEnemy.h"
#include "Perception/PawnSensingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BasicEnemyController.h"
#include "BasicEnemyAnimInstance.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "CourseProjectGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerCharacter.h"

// Sets default values
ABasicEnemy::ABasicEnemy()
{
    PrimaryActorTick.bCanEverTick = true;
    PawnSensingComponent = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("PawnSensor"));

    LeftFistCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("LeftFistCapsule"));
    LeftFistCapsule->SetupAttachment(GetMesh(), TEXT("hand_l"));
    LeftFistCapsule->SetCapsuleSize(10.f, 20.f);
    LeftFistCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    LeftFistCapsule->SetCollisionObjectType(ECC_WorldDynamic);
    LeftFistCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);

    RightFistCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("RightFistCapsule"));
    RightFistCapsule->SetupAttachment(GetMesh(), TEXT("hand_r"));
    RightFistCapsule->SetCapsuleSize(10.f, 20.f);
    RightFistCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RightFistCapsule->SetCollisionObjectType(ECC_WorldDynamic);
    RightFistCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);

    GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));

    GetCharacterMovement()->bUseRVOAvoidance = true;
    GetCharacterMovement()->AvoidanceWeight = 0.5f;
    static int32 NextUID = 1;
    GetCharacterMovement()->SetRVOAvoidanceUID(NextUID++);
}

// Called when the game starts or when spawned
void ABasicEnemy::BeginPlay()
{
	Super::BeginPlay();
	_HealthPoints = HealthPoints;
    UE_LOG(LogTemp, Error, TEXT("Enemy HP Set"));
	if (PawnSensingComponent) {
		PawnSensingComponent->OnSeePawn.AddDynamic(this, &ABasicEnemy::OnSeePlayer);
        UE_LOG(LogTemp, Error, TEXT("PawnSensing Set"));
	}
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	bUseControllerRotationYaw = false;

	GetCharacterMovement()->RotationRate = FRotator(0.f, 240.f, 0.f);

    if (HealthBarWidget)
    {
        OnStatsChanged();
    }
    

    if (LeftFistCapsule)
    {
        LeftFistCapsule->OnComponentBeginOverlap.AddDynamic(this, &ABasicEnemy::OnFistBeginOverlap);
    }
    if (RightFistCapsule)
    {
        RightFistCapsule->OnComponentBeginOverlap.AddDynamic(this, &ABasicEnemy::OnFistBeginOverlap);
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
        bIsHeavyAttacking = false;
        //EnableFistCollision(false);
        bIsDamageDealed = false;
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

        if (AngleToTarget < AttackAngleThreshold && !bIsInAttackRecover)
        {
            auto enemyController = Cast<ABasicEnemyController>(GetController());
            if (enemyController)
            {
                if (CanAttack())
                {
                    enemyController->MakeAttackDecision(_ChasedTarget);
                }
            }
        }
    }

    if (_ChasedTarget)
    {
        LostSightTime = 0.0f;
    }
    else
    {
        LostSightTime += DeltaTime;
        if (LostSightTime >= 15.0f)
        {
            DieProcess();
        }
    }
    if (_AttackRecoverCountingDown > 0.0f)
    {
        _AttackRecoverCountingDown -= DeltaTime;
    }
    else
    {
        bIsInAttackRecover = false;

    }
}

void ABasicEnemy::ActivateBuff()
{
    if (BuffEffectNiagara && GetMesh())
    {
        FName SocketRName(TEXT("FX_Trail_R"));
        UNiagaraFunctionLibrary::SpawnSystemAttached(
            BuffEffectNiagara,
            GetMesh(),
            SocketRName,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget,
            true
        );

        FName SocketLName(TEXT("FX_Trail_L"));
        UNiagaraFunctionLibrary::SpawnSystemAttached(
            BuffEffectNiagara,
            GetMesh(),
            SocketLName,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget,
            true
        );
    }
    BasicDamage = BasicDamage + 50;
}

void ABasicEnemy::SetAllParams(int health, int damage, float attackInterval)
{
    HealthPoints = health;
    BasicDamage = damage;
    AttackInterval = attackInterval;
    _HealthPoints = HealthPoints;
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

    _ChasedTarget = targetPawn;
    UAnimInstance* animInst = meshComp->GetAnimInstance();
    UBasicEnemyAnimInstance* enemyAnimInst = Cast<UBasicEnemyAnimInstance>(animInst);

    if (targetPawn && enemyAnimInst->State == EBasicEnemyState::Locomotion && !bAreHit && !bIsAttacking)
    {
        AController* controller = GetController();
        ABasicEnemyController* enemyController = Cast<ABasicEnemyController>(controller);
        //UE_LOG(LogTemp, Error, TEXT("Chasing"));

        enemyController->MoveToActor(targetPawn, 70.0f, true, true, true);
    }
    else if (targetPawn && enemyAnimInst->State == EBasicEnemyState::Locomotion && !bAreHit)
    {
        AController* controller = GetController();
        ABasicEnemyController* enemyController = Cast<ABasicEnemyController>(controller);
        //UE_LOG(LogTemp, Error, TEXT("Chasing"));

        enemyController->MoveToActor(targetPawn, 210.0f, true, true, true);
    }
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
    bIsHeavyAttacking = true;
}
void ABasicEnemy::StartAttack(float attackInterval)
{
    GetController()->StopMovement();
    auto animInst = Cast<UBasicEnemyAnimInstance>(GetMesh()->GetAnimInstance());
    if (animInst && !bAreHit)
    {
        bIsAttacking = true;
        //EnableFistCollision(true);
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
void ABasicEnemy::OnFistBeginOverlap(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult
)
{
    UE_LOG(LogTemp, Log, TEXT("Player Overlapped"));
    auto actor = Cast<APlayerCharacter>(OtherActor);
    if (actor != nullptr && bIsAttacking && !bIsDamageDealed)
    {
        bIsDamageDealed = true;
        UE_LOG(LogTemp, Log, TEXT("Hit"));
        actor->Hit(100, bIsHeavyAttacking, GetActorLocation());
        if (PunchCue && !actor->bIsInvulnerable && !actor->bIsDead)
        {
            UGameplayStatics::PlaySoundAtLocation(this, PunchCue, GetActorLocation());
        }
        if (actor->IsKilled())
        {
            _ChasedTarget = nullptr;
        }
    }
}

void ABasicEnemy::EnableFistCollision(bool bEnable)
{
    ECollisionEnabled::Type NewCollision = bEnable ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision;
    LeftFistCapsule->SetCollisionEnabled(NewCollision);
    RightFistCapsule->SetCollisionEnabled(NewCollision);

    if (bEnable)
    {
        LeftFistCapsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
        RightFistCapsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    }
    else
    {
        LeftFistCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);
        RightFistCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);
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
    if (MySoundCue)
    {
        UGameplayStatics::PlaySoundAtLocation(this, MySoundCue, GetActorLocation());
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

    UWorld* World = GetWorld();
    if (!World) return;

    ACourseProjectGameMode* GM = Cast<ACourseProjectGameMode>(UGameplayStatics::GetGameMode(World));
    if (GM)
    {
        GM->OnEnemyEliminated(this);
    }
}

void ABasicEnemy::OnSeePlayer(APawn* SensedPawn) {
    if (SensedPawn) {
        Chase(SensedPawn);

        GetWorldTimerManager().ClearTimer(LoseSightTimerHandle);
        GetWorldTimerManager().SetTimer(
            LoseSightTimerHandle,
            this,
            &ABasicEnemy::OnLosePlayer,
            LoseSightDelay,
            false
        );
    }
}

void ABasicEnemy::OnLosePlayer()
{
    _ChasedTarget = nullptr;
    UE_LOG(LogTemp, Warning, TEXT("Lost sight of player, setting _ChasedTarget to nullptr"));
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
