// Fill out your copyright notice in the Description page of Project Settings.


#include "CastleEnemy.h"
#include "Perception/PawnSensingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CastleEnemyController.h"
#include "NiagaraFunctionLibrary.h"
#include "CastleEnemyAnimInstance.h"
#include "Kismet/GameplayStatics.h"
#include "CourseProjectGameMode.h"
#include "NavigationSystem.h"
// Sets default values
ACastleEnemy::ACastleEnemy()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PawnSensingComponent = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("PawnSensor"));

	static ConstructorHelpers::FClassFinder<APlayerWeapon> WeaponClassFinder(TEXT("/Game/ThirdPerson/Blueprints/BP_CastleEnemyWeapon"));
	if (WeaponClassFinder.Succeeded())
	{
		_WeaponClass = WeaponClassFinder.Class;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to find BP_CastleWeapon class!"));
		_WeaponClass = APlayerWeapon::StaticClass();
	}

	AIControllerClass = ACastleEnemyController::StaticClass(); 
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

// Called when the game starts or when spawned
void ACastleEnemy::BeginPlay()
{
	Super::BeginPlay();
	_HealthPoints = HealthPoints;
	if (PawnSensingComponent) {
		PawnSensingComponent->OnSeePawn.AddDynamic(this, &ACastleEnemy::OnSeePlayer);
		UE_LOG(LogTemp, Error, TEXT("PawnSensing Set"));
	}
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	bUseControllerRotationYaw = false;

	GetCharacterMovement()->RotationRate = FRotator(0.f, 240.f, 0.f);

	_Weapon = Cast<APlayerWeapon>(GetWorld()->SpawnActor(_WeaponClass));
	if (_Weapon)
	{
		_Weapon->Holder = this;
		_Weapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, FName("WeaponStartSocket"));
		_Weapon->DisableOverlapReaction();
	}
	if (HealthBarWidget)
	{
		OnStatsChanged();
	}
	
}

// Called every frame
void ACastleEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	auto animInst = Cast<UCastleEnemyAnimInstance>(GetMesh()->GetAnimInstance());
	if (!animInst)
		return;

	FVector Velocity = GetCharacterMovement()->Velocity;
	float TargetSpeed = Velocity.Size();
	animInst->Speed = FMath::FInterpTo(animInst->Speed, TargetSpeed, DeltaTime, 8.0f);

	if (Velocity.SizeSquared() > KINDA_SMALL_NUMBER)
	{
		FVector LocalVelocity = GetActorTransform().InverseTransformVector(Velocity);

		float Angle = FMath::Atan2(LocalVelocity.Y, LocalVelocity.X) * (180.f / PI);

		animInst->Direction = Angle;
	}
	if (_ChasedTarget && !bIsAttacking && animInst->State==ECastleEnemyState::Locomotion)
	{
		FVector ToTarget = _ChasedTarget->GetActorLocation() - GetActorLocation();
		ToTarget.Z = 0.0f;
		FRotator TargetRotation = ToTarget.Rotation();
		float AngleToTarget = FMath::Abs(FRotator::NormalizeAxis(GetActorRotation().Yaw - TargetRotation.Yaw));
		constexpr float AttackAngleThreshold = 15.0f;

		if (AngleToTarget < AttackAngleThreshold && !bIsInHitRecover &&!bIsInAttackRecover)
		{
			auto enemyController = Cast<ACastleEnemyController>(GetController());
			if (enemyController)
			{
					enemyController->MakeAttackDecision(_ChasedTarget);
				
			}
		}
	}

	if (_ChasedTarget && !bIsAttacking)
	{
		FVector ToTarget = _ChasedTarget->GetActorLocation() - GetActorLocation();
		ToTarget.Z = 0.0f;

		if (ToTarget.SizeSquared() > KINDA_SMALL_NUMBER)
		{
			FRotator CurrentRotation = GetActorRotation();
			FRotator TargetRotation = ToTarget.Rotation();

			FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, 12.0f);
			SetActorRotation(NewRotation);
		}
	}

	if (_HitRecoverCountingDown > 0.0f)
	{
		_HitRecoverCountingDown -= DeltaTime;
	}
	else
	{
		bIsInHitRecover = false;

	}
	
	if (_AttackRecoverCountingDown > 0.0f && _ChasedTarget)
	{
		auto dist = FVector::Dist2D(this->GetActorLocation(), _ChasedTarget->GetTargetLocation());
		if (dist <= AttackRange)
		{
			_AttackRecoverCountingDown -= DeltaTime;
		}
	}
	else
	{
		bIsInAttackRecover = false;

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
}
void ACastleEnemy::SpawnProcessEnds()
{
	bIsAttacking = false;
	bIsInHitRecover = false;
}
void ACastleEnemy::SetAllParams(int health, int damage, float attackInterval)
{
	HealthPoints = health;
	BasicDamage = damage;
	AttackInterval = attackInterval;
	_HealthPoints = HealthPoints;
}


void ACastleEnemy::Chase(APawn* targetPawn)
{
	USkeletalMeshComponent* meshComp = GetMesh();

	_ChasedTarget = targetPawn;
	UAnimInstance* animInst = meshComp->GetAnimInstance();
	UCastleEnemyAnimInstance* enemyAnimInst = Cast<UCastleEnemyAnimInstance>(animInst);

	if (targetPawn && enemyAnimInst->State == ECastleEnemyState::Locomotion && !bAreHit &&!bIsAttacking && !bShouldBuff)
	{
		AController* controller = GetController();
		ACastleEnemyController* enemyController = Cast<ACastleEnemyController>(controller);
		//UE_LOG(LogTemp, Error, TEXT("Chasing"));

		enemyController->MoveToActor(targetPawn, 60.0f, true, true, true);
	}
	else if (targetPawn && enemyAnimInst->State == ECastleEnemyState::Locomotion && !bAreHit)
	{
		AController* controller = GetController();
		ACastleEnemyController* enemyController = Cast<ACastleEnemyController>(controller);
		//UE_LOG(LogTemp, Error, TEXT("Chasing"));

		enemyController->MoveToActor(targetPawn, 180.0f, true, true, true);
	}
}

void ACastleEnemy::Retreat(APawn* TargetPawn, float RetreatDistance)
{
	if (!TargetPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("Retreat: TargetPawn is null."));
		return;
	}

	ACastleEnemyController* AIController = Cast<ACastleEnemyController>(GetController());
	if (!AIController)
	{
		UE_LOG(LogTemp, Warning, TEXT("Retreat: AIController is null."));
		return;
	}


	FVector EnemyLocation = GetActorLocation();
	FVector ForwardVector = GetActorForwardVector();

	FVector DesiredRetreatLocation = EnemyLocation - ForwardVector * RetreatDistance;

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavSys)
	{
		FNavLocation NavLocation;
		bool bFoundLocation = NavSys->GetRandomReachablePointInRadius(DesiredRetreatLocation, RetreatDistance * 0.5f, NavLocation);
		if (bFoundLocation)
		{
			DesiredRetreatLocation = NavLocation.Location;
		}
	}

	ACharacter* Character = Cast<ACharacter>(this);
	if (Character)
	{
		Character->bUseControllerRotationYaw = false;
		if (Character->GetCharacterMovement())
		{
			Character->GetCharacterMovement()->bOrientRotationToMovement = false;
			Character->GetCharacterMovement()->MaxWalkSpeed = 200.0f;
		}
	}

	AIController->MoveToLocation(DesiredRetreatLocation, 50.0f, false);

	_ChasedTarget = TargetPawn;

#if !UE_BUILD_SHIPPING
	DrawDebugSphere(GetWorld(), DesiredRetreatLocation, 30.0f, 12, FColor::Blue, false, 2.0f);
#endif
}

void ACastleEnemy::OnSeePlayer(APawn* SensedPawn) {
	if (SensedPawn) {
		Chase(SensedPawn);

		GetWorldTimerManager().ClearTimer(LoseSightTimerHandle);
		GetWorldTimerManager().SetTimer(
			LoseSightTimerHandle,
			this,
			&ACastleEnemy::OnLosePlayer,
			LoseSightDelay,
			false
		);
	}
}
void ACastleEnemy::OnLosePlayer()
{
	_ChasedTarget = nullptr;
	UE_LOG(LogTemp, Warning, TEXT("Lost sight of player, setting _ChasedTarget to nullptr"));
}
void ACastleEnemy::ChangeWeaponSocket()
{
	if (_Weapon)
	{
		_Weapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, FName("hand_rSocket"));
	}
}

bool ACastleEnemy::CanAttack()
{
	USkeletalMeshComponent* meshComp = GetMesh();
	UAnimInstance* animInst = meshComp->GetAnimInstance();
	UCastleEnemyAnimInstance* enemyAnimInst = Cast<UCastleEnemyAnimInstance>(animInst);
	if (enemyAnimInst)
	{
		if (enemyAnimInst->State == ECastleEnemyState::Attack1 || enemyAnimInst->State == ECastleEnemyState::Attack2 || enemyAnimInst->State == ECastleEnemyState::Attack3 || enemyAnimInst->State == ECastleEnemyState::Locomotion
			|| !bAreHit || !bIsInHitRecover)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	return false;
}

void ACastleEnemy::Attack()
{
	UE_LOG(LogTemp, Error, TEXT("Attacking"));
	USkeletalMeshComponent* meshComp = GetMesh();
	_Weapon->EnableOverlapReaction();
	UAnimInstance* animInst = meshComp->GetAnimInstance();
	UCastleEnemyAnimInstance* enemyAnimInst = Cast<UCastleEnemyAnimInstance>(animInst);

	if (enemyAnimInst && !bAreHit && !bIsInHitRecover)
	{
		int32 AttackType = FMath::RandRange(0, 3);
		bIsAttacking = true;
		GetController()->StopMovement();
		switch (AttackType)
		{
		case 0:
			enemyAnimInst->State = ECastleEnemyState::Attack1;
			AttackIndex = 1;
			break;
		case 1:
			enemyAnimInst->State = ECastleEnemyState::Attack2;
			AttackIndex = 2;
			break;
		case 2:
			enemyAnimInst->State = ECastleEnemyState::Attack3;
			AttackIndex = 3;
			break;
		default:
			enemyAnimInst->State = ECastleEnemyState::Attack4;
			AttackIndex = 4;
			break;
		}
	}
	
}

void ACastleEnemy::RecoverFromAttack()
{
	UE_LOG(LogTemp, Error, TEXT("Recovering"));
	bIsAttacking = true;
	USkeletalMeshComponent* meshComp = GetMesh();
	_Weapon->DisableOverlapReaction();
	_Weapon->bCanDealDamage = true;
	UAnimInstance* animInst = meshComp->GetAnimInstance();
	UCastleEnemyAnimInstance* enemyAnimInst = Cast<UCastleEnemyAnimInstance>(animInst);
	if (enemyAnimInst && !bIsInHitRecover)
	{	
		switch (AttackIndex)
		{
		case 1:
			enemyAnimInst->State = ECastleEnemyState::Attack1Recovery;
			break;
		case 2:
			enemyAnimInst->State = ECastleEnemyState::Attack2Recovery;
			break;
		case 3:
			enemyAnimInst->State = ECastleEnemyState::Attack3Recovery;
			break;
		default:
			enemyAnimInst->State = ECastleEnemyState::Locomotion;
			break;
		}
		//if (HitRecived <= 2)
		//{
		//	bIsInHitRecover = true;
		//	_HitRecoverCountingDown = HitRecoverInterval + 1.0f;
		//}
		//else
		//{
		//	HitRecived = 0;
		//}
	}
}

void ACastleEnemy::ActivateBuff()
{
	if (BuffEffectNiagara && GetMesh())
	{
		FName SocketRName(TEXT("FX_Trail_R_01"));
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			BuffEffectNiagara,
			GetMesh(),
			SocketRName,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true
		);

		FName SocketLName(TEXT("FX_Trail_L_01"));
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			BuffEffectNiagara,
			GetMesh(),
			SocketLName,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true
		);
		FName SocketSName(TEXT("hand_rSocket_0"));
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			SwordBuffEffectNiagara,
			GetMesh(),
			SocketSName,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true
		);
	}
	BasicDamage = BasicDamage + 50;
}

void ACastleEnemy::OnStatsChanged()
{
	if (HealthBarWidget != nullptr)
	{
		float HealthPercent = FMath::Clamp((float)_HealthPoints / (float)HealthPoints, 0.0f, 1.0f);
		auto healthBar = Cast<UEnemyHealthBar>(HealthBarWidget);
		healthBar->HealthProgressBar->SetPercent(HealthPercent);
	}
}

void ACastleEnemy::Hit(int damage)
{
	_HealthPoints -= damage;
	auto animInst = GetMesh()->GetAnimInstance();
	auto enemyAnimInst = Cast<UCastleEnemyAnimInstance>(animInst);
	OnStatsChanged();
	AttackIndex = 0;
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

	if (_HealthPoints <= 0)
	{
		bAreHit = true;
		FName SocketName = TEXT("YourSocketName");

		USkeletalMeshComponent* MeshComp = GetMesh();

		enemyAnimInst->State = ECastleEnemyState::Die;
	}
	//else if (_HealthPoints <= HealthPoints / 2 && !bIsBuffed)
	//{
	//	//bShouldBuff = true;
	//	bIsBuffed = true;
	//	bIsAttacking = false;
	//	AttackIndex = 0;
	//	enemyAnimInst->State = ECastleEnemyState::Buff;
	//}
	else
	{
		if ( !bAreHit )
		{
			UE_LOG(LogTemp, Log, TEXT("Hit animation plays"));
			bAreHit = true;
			enemyAnimInst->State = ECastleEnemyState::Hit;
			bIsInHitRecover = true;
			_HitRecoverCountingDown = HitRecoverInterval;
		}
	}
}

int ACastleEnemy::GetEnemyDamage()
{
	return BasicDamage;
}
void ACastleEnemy::DieProcess()
{
	UWorld* World = GetWorld();
	if (!World) return;
	ACourseProjectGameMode* GM = Cast<ACourseProjectGameMode>(UGameplayStatics::GetGameMode(World));
	if (GM)
	{
		GM->OnEnemyEliminated(this);
		_Weapon->Destroy();
	}
	PrimaryActorTick.bCanEverTick = false;
	K2_DestroyActor();
	GEngine->ForceGarbageCollection(true);

}