// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "PlayerAnimInstance.h"
#include "NiagaraFunctionLibrary.h"
#include "CourseProjectGameMode.h"
// Sets default values
APlayerCharacter::APlayerCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	_springArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	_springArmComponent->SetupAttachment(RootComponent);
	//_springArmComponent->SetUsingAbsoluteRotation(true);
	_springArmComponent->TargetArmLength = 400.f;
	_springArmComponent->bUsePawnControlRotation = true;
	_springArmComponent->bDoCollisionTest = true;
	_cameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	_cameraComponent->SetupAttachment(_springArmComponent, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	_cameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	auto characterMovement = GetCharacterMovement();
	characterMovement->bOrientRotationToMovement = true; // Rotate character to moving direction
	characterMovement->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	characterMovement->JumpZVelocity = 700.f;
	characterMovement->AirControl = 0.35f;
	characterMovement->MaxWalkSpeed = 500.f;
	characterMovement->MinAnalogWalkSpeed = 20.f;
	characterMovement->BrakingDecelerationWalking = 2000.f;
	characterMovement->BrakingDecelerationFalling = 1500.0f;

	//Weapon
	static ConstructorHelpers::FClassFinder<APlayerWeapon> WeaponClassFinder(TEXT("/Game/ThirdPerson/Blueprints/BP_PlayerWeapon"));
	if (WeaponClassFinder.Succeeded())
	{
		_WeaponClass = WeaponClassFinder.Class;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to find BP_PlayerWeapon class!"));
		_WeaponClass = APlayerWeapon::StaticClass(); 
	}

}

void APlayerCharacter::HandleOnMontageNotifyIvents(FName a_MotifyName, const FBranchingPointNotifyPayload& a_BranchingPayload)
{
	if (a_MotifyName.ToString() == "Dodge")
	{
		bIsDodging = false;
	}
	if (a_MotifyName.ToString() == "FillHP")
	{
		if (_HealthPoints + 50 >= HealthPoints)
		{
			_HealthPoints = HealthPoints;
		}
		else
		{
			_HealthPoints += 200;
		}
		OnStatsChanged();
	}
	if (a_MotifyName.ToString() == "Invulnerable")
	{
		bIsInvulnerable = true;
	}
	else if (a_MotifyName.ToString() == "NotInvulnerable")
	{
		bIsInvulnerable = false;
	}
	if (a_MotifyName.ToString() == "Heal")
	{
		bIsHealing = false;
	}
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	UAnimInstance* AnimInst = GetMesh()->GetAnimInstance();
	_HealthPoints = HealthPoints;
	_StaminaPoints = StaminaPoints;
	_HealItems = MaxHealItems;
	if (AnimInst != nullptr)
	{
		AnimInst->OnPlayMontageNotifyBegin.AddDynamic(this, &APlayerCharacter::HandleOnMontageNotifyIvents);
	}
	_Weapon = Cast<APlayerWeapon>(GetWorld()->SpawnActor(_WeaponClass));
	if (_Weapon)
	{
		_Weapon->Holder = this;
		_Weapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, FName("lowerarm_lSocket"));
		_Weapon->DisableOverlapReaction();
	}

	if (StatsBarWidgetClass)
	{
		UUStatsWidget* CreatedWidget = CreateWidget<UUStatsWidget>(GetWorld(), StatsBarWidgetClass);
		if (CreatedWidget)
		{
			StatsBarWidgetInstance = CreatedWidget;
			StatsBarWidgetInstance->AddToViewport();

			UE_LOG(LogTemp, Log, TEXT("Stats widget created and added to viewport"));
			OnStatsChanged();
			OnHealItemsChanged();
		}
	}


}



// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UPlayerAnimInstance* animInst = Cast<UPlayerAnimInstance>(GetMesh()->GetAnimInstance());
	FVector Velocity = GetCharacterMovement()->Velocity;
	float TargetSpeed = Velocity.Size();
	animInst->Speed = FMath::FInterpTo(animInst->Speed, TargetSpeed, DeltaTime, 8.0f);
	if (GetCharacterMovement()->IsFalling() && !bIsFalling && !bIsDodging && !bIsHit)
	{
		if (animInst)
		{
			bIsFalling = true;
			animInst->State = EPlayerState::Fall;
		}
	}
	else if (!GetCharacterMovement()->IsFalling() && bIsFalling && !bIsInFallRecover && !bIsDodging)
	{
		if (animInst)
		{
			bIsFalling = false;
			bIsInFallRecover = true;
			animInst->State = EPlayerState::Land;
		}
	}
	// Calculate direction
	if (Velocity.SizeSquared() > 0.0f) // Only calculate direction if moving
	{
		// Get the controller's yaw rotation to determine forward direction
		const FRotator ControlRotation = Controller ? Controller->GetControlRotation() : FRotator::ZeroRotator;
		const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
		const FVector ForwardVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// Normalize velocity to get movement direction
		FVector MovementDirection = Velocity.GetSafeNormal();

		// Calculate the angle between forward vector and movement direction
		float DotProduct = FVector::DotProduct(ForwardVector, MovementDirection);
		float Angle = FMath::Acos(DotProduct); // Angle in radians
		Angle = FMath::RadiansToDegrees(Angle); // Convert to degrees

		// Determine if the movement is to the left or right using the cross product
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
	if (_HitCountingDown > 0.0f)
	{
		_HitCountingDown -= DeltaTime;
		if (_HitCountingDown <= 0)
		{
			bIsHit = false;
		}
	}
	
	if (_RollCountingDown > 0.0f)
	{
		_RollCountingDown -= DeltaTime;
	}
	if (_HealCountingDown > 0.0f)
	{
		_HealCountingDown -= DeltaTime;
	}
	if (_AttackCountingDown > 0.0f)
	{
		_AttackCountingDown -= DeltaTime;
		if (_AttackCountingDown <= 0.0f)
		{
			bIsAttacking = false;
			_Weapon->DisableOverlapReaction();
			_Weapon->bCanDealDamage = true;
		}
	}
	if (_StaminaPoints < StaminaPoints && !bIsAttacking && !bIsDodging)
	{
		_StaminaPoints += 1.1f;
		OnStatsChanged();

	}
	if (CanAttack() && ComboAttackIndex == 1 && !bIsDodging && !bIsHealing)
	{
		if (_StaminaPoints - 20 < 0)
		{
			_StaminaPoints = 0;
		}
		else
		{
			_StaminaPoints -= 20;
		}
		OnStatsChanged();
		_Weapon->EnableOverlapReaction();
		animInst->State = EPlayerState::SecondAttack;
		bIsAttacking = true;
		_AttackCountingDown = AttackInterval;
		ComboAttackIndex = 2;
	}
	if (CanAttack() && ComboAttackIndex == 3 && !bIsDodging && !bIsHealing)
	{
		if (_StaminaPoints - 20 < 0)
		{
			_StaminaPoints = 0;
		}
		else
		{
			_StaminaPoints -= 20;
		}
		OnStatsChanged();
		_Weapon->EnableOverlapReaction();
		animInst->State = EPlayerState::ThirdAttack;
		bIsAttacking = true;
		_AttackCountingDown = AttackInterval;
		ComboAttackIndex = 4;
	}
	if (LockedTarget)
	{
		
		FVector ToTarget = LockedTarget->GetActorLocation() - GetActorLocation();
		FRotator TargetRot = ToTarget.Rotation();
		FRotator CurrentRot = GetActorRotation();

		FRotator ControllerRot = GetControlRotation();
		FRotator NewControllerRot = FMath::RInterpTo(ControllerRot, TargetRot, DeltaTime, 8.f);

		constexpr float FixedPitchAngle = -20.f;  
		NewControllerRot.Pitch = FixedPitchAngle;

		ControllerRot.Yaw = NewControllerRot.Yaw;
		ControllerRot.Pitch = NewControllerRot.Pitch;

		GetController()->SetControlRotation(ControllerRot);
	}
}

void APlayerCharacter::OnAttackPressed()
{
	if (CanAttack() && !bIsDodging && !bIsAttacking && !bIsHealing && !bIsHit && !bIsFalling && !bIsInFallRecover && !bIsInRespawnProcess)
	{
		UPlayerAnimInstance* animInst = Cast<UPlayerAnimInstance>(GetMesh()->GetAnimInstance());
		if (animInst != nullptr)
		{
			if (_StaminaPoints >= 20)
			{
				if (Controller)
				{
					FRotator ControlRotation = Controller->GetControlRotation();
					FRotator NewRotation(0.f, ControlRotation.Yaw, 0.f); 
					SetActorRotation(NewRotation);
				}
				_Weapon->EnableOverlapReaction();
				animInst->State = EPlayerState::Attack;
				bIsAttacking = true;
				_AttackCountingDown = AttackInterval;
				if (_StaminaPoints - 20 < 0)
				{
					_StaminaPoints = 0;
				}
				else
				{
					_StaminaPoints -= 20;
				}
				OnStatsChanged();

				ComboAttackIndex = 0;
			}
		}
	}
	else
	{
		if (ComboAttackIndex == 0 )
		{
			if (!bIsDodging && _StaminaPoints >= 20 && !bIsHit && !bIsHealing && (_AttackCountingDown >0.2f && _AttackCountingDown < 0.7f))
			{
				ComboAttackIndex = 1;
			}
		}
		if (ComboAttackIndex == 2)
		{
			if (!bIsDodging && _StaminaPoints >= 20 && !bIsHit && !bIsHealing && (_AttackCountingDown > 0.2f && _AttackCountingDown < 0.7f))
			{
				ComboAttackIndex = 3;
			}
		}
	}
}

void APlayerCharacter::OnStatsChanged()
{
	if (StatsBarWidgetInstance->HealthProgressBar && StatsBarWidgetInstance->StaminaProgressBar)
	{
		float HealthPercent = FMath::Clamp((float)_HealthPoints / (float)HealthPoints, 0.f, 1.f);
		float StaminaPercent = FMath::Clamp((float)_StaminaPoints / (float)StaminaPoints, 0.f, 1.f);
		StatsBarWidgetInstance->HealthProgressBar->SetPercent(HealthPercent);
		StatsBarWidgetInstance->StaminaProgressBar->SetPercent(StaminaPercent);
	}
}

void APlayerCharacter::OnHealItemsChanged()
{
	if (StatsBarWidgetInstance->HealthKitCountText)
	{
		StatsBarWidgetInstance->HealthKitCountText->SetText(FText::AsNumber(_HealItems));
	}
}
void APlayerCharacter::OnTimerChanged(int32 minutes, int32 seconds)
{
	if (StatsBarWidgetInstance->TimerText)
	{
		FString TimeString = FString::Printf(TEXT("%02d:%02d"), minutes, seconds);

		FText TimeText = FText::FromString(TimeString);

		StatsBarWidgetInstance->TimerText->SetText(TimeText);
	}
}
// Called to bind functionality to input
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {


		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
	}
	InputComponent->BindAction("Roll", IE_Pressed, this, &APlayerCharacter::OnRollPressed);

	InputComponent->BindAction("Attack", IE_Pressed, this, &APlayerCharacter::OnAttackPressed);

	InputComponent->BindAction("Heal", IE_Pressed, this, &APlayerCharacter::OnHealPressed);

	InputComponent->BindAction("LockTarget", IE_Pressed, this, &APlayerCharacter::OnTargetLockPressed);

	InputComponent->BindAction("SpecialAttack", IE_Pressed, this, &APlayerCharacter::OnSpecialAttackPressed);

	InputComponent->BindAction("Pause", IE_Pressed, this, &APlayerCharacter::OnPausePressed);
}

void APlayerCharacter::OnPausePressed()
{
	UE_LOG(LogTemp, Log, TEXT("Pause"));
	if (!PauseWidgetClass) return;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	if (!bIsGamePaused)
	{
		UGameplayStatics::SetGamePaused(GetWorld(), true);
		bIsGamePaused = true;

		PauseWidget = CreateWidget<UUserWidget>(PC, PauseWidgetClass);
		if (!PauseWidget) return;
		PauseWidget->AddToViewport();

		PC->bShowMouseCursor = true;
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(PauseWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
	}
	else
	{
		UGameplayStatics::SetGamePaused(GetWorld(), false);
		bIsGamePaused = false;

		if (PauseWidget)
		{
			PauseWidget->RemoveFromParent();
			PauseWidget = nullptr;
		}

		PC->bShowMouseCursor = false;
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
	}
}

int APlayerCharacter::GetHealthPoints()
{
	return _HealthPoints;
}
int APlayerCharacter::GetComboIndex()
{
	return ComboAttackIndex;
}
bool APlayerCharacter::IsKilled()
{
	return (_HealthPoints <= 0.0f);
}

bool APlayerCharacter::CanAttack()
{
	return (_AttackCountingDown <=0);
}

bool APlayerCharacter::CanRoll()
{
	return (_RollCountingDown <= 0 && _StaminaPoints > 20);
}

bool APlayerCharacter::CanHeal()
{
	return (_HealCountingDown <= 0);
}

void APlayerCharacter::ChangeWeaponSocket()
{
	if (_Weapon)
	{
		_Weapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, FName("hand_rSocket"));
	}
}

void APlayerCharacter::Hit(int damage, bool IsHeavyHit, const FVector& HitOrigin)
{
	if (!bIsInvulnerable && !bIsDead && !bIsInRespawnProcess)
	{
		bIsAttacking = false;
		bIsDodging = false;
		_Weapon->DisableOverlapReaction();
		_Weapon->bCanDealDamage = true;
		ComboAttackIndex = 0;
		bIsHit = true;
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
		UPlayerAnimInstance* animInst = Cast<UPlayerAnimInstance>(GetMesh()->GetAnimInstance());
		if (_HealthPoints - damage <= 0 && !bHasRespawnBuff)
		{
			_HealthPoints = 0;
			OnStatsChanged();
			Die();
			return;
		}
		else if (_HealthPoints - damage <= 0 && bHasRespawnBuff)
		{
			_HealthPoints = 0;
			OnStatsChanged();
			bIsInRespawnProcess = true;
			animInst->State = EPlayerState::RespawnBuff;
			bHasRespawnBuff = false;
			bIsHit = false;
			return;
		}
		else
		{
			_HealthPoints -= damage;
		}
		if (IsHeavyHit && animInst->State!=EPlayerState::RecoverFromHeavyHit && animInst->State!=EPlayerState::HeavHit)
		{
			animInst->State = EPlayerState::HeavHit;
			if (Controller)
			{
				FVector DirectionToEnemy = HitOrigin - GetActorLocation();
				DirectionToEnemy.Z = 0.f;
				DirectionToEnemy.Normalize();

				FRotator LookAtRotation = DirectionToEnemy.Rotation();
				SetActorRotation(LookAtRotation);

				float LaunchDistance = 1400.f;
				FVector LaunchVelocity = -DirectionToEnemy * LaunchDistance; 

				LaunchCharacter(LaunchVelocity, true, false);
			}
		}
		else if (animInst->State != EPlayerState::Hit && !bIsInHitAnimation && _HitCountingDown<=0.0f && animInst->State != EPlayerState::HeavHit)
		{
			_HitCountingDown = HitInterval;
			bIsInHitAnimation = true;
			animInst->State = EPlayerState::Hit;

		}
		//else if (_HealCountingDown<=0)
		//{
		//	//animInst->State = EPlayerState::Hit;
		//	_HitCountingDown = HitInterval;
		//}
		if (bIsDodging)
		{
			animInst->Montage_Stop(0.1);
			bIsDodging = false;
		}

		OnStatsChanged();
	}

}


void APlayerCharacter::OnRollPressed()
{
	if (!bIsDodging && CanRoll() && !bIsAttacking && !bIsHealing && !bIsHit && CanHeal() && !bIsFalling && !bIsInFallRecover &&!bIsDead && !bIsInRespawnProcess)
	{
		UAnimInstance* AnimInst = GetMesh()->GetAnimInstance();
		UPlayerAnimInstance* animInst = Cast<UPlayerAnimInstance>(AnimInst);
		if (AnimInst != nullptr)
		{
			FVector MovementDirection = FVector::ZeroVector;

			if (LastMovementInput.SizeSquared() > KINDA_SMALL_NUMBER)
			{
				const FRotator ControlRot = Controller ? Controller->GetControlRotation() : FRotator::ZeroRotator;
				const FRotator YawRot(0.0f, ControlRot.Yaw, 0.0f);
				const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
				const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

				MovementDirection = (Forward * LastMovementInput.Y + Right * LastMovementInput.X).GetSafeNormal();
			}
			else
			{
				MovementDirection = GetActorForwardVector().GetSafeNormal();
			}

			if (MovementDirection.SizeSquared() > KINDA_SMALL_NUMBER)
			{
				float YawRad = FMath::Atan2(MovementDirection.Y, MovementDirection.X);
				float YawDeg = FMath::RadiansToDegrees(YawRad);

				FRotator NewRot = FRotator(0.0f, YawDeg, 0.0f);
				SetActorRotation(NewRot, ETeleportType::TeleportPhysics);
			}
			if (animInst)
			{
				animInst->State = EPlayerState::Roll;
			}
			bIsDodging = true;
			_RollCountingDown = RollInterval;
			if (_StaminaPoints - 20 < 0)
			{
				_StaminaPoints = 0;
			}
			else
			{
				_StaminaPoints -= 20;
			}
			OnStatsChanged();

			AnimInst->Montage_Play(m_pDodgeMontage);
		}
	}
}

void APlayerCharacter::GiveHeal(int amount)
{
	_HealItems = FMath::Min(MaxHealItems, _HealItems + amount);
	OnHealItemsChanged();
}

int APlayerCharacter::GetPlayerDamage()
{
	return Damage;
}

int APlayerCharacter::GetCurrentHealItems()
{
	return _HealItems;
}

void APlayerCharacter::OnHealPressed()
{
	if (!bIsDodging && CanHeal() && !bIsAttacking && !bIsHit && !bIsHealing && _HealItems>0 && !bIsFalling && !bIsInFallRecover && !bIsInRespawnProcess)
	{
		UAnimInstance* AnimInst = GetMesh()->GetAnimInstance();
		UPlayerAnimInstance* animInst = Cast<UPlayerAnimInstance>(GetMesh()->GetAnimInstance());
		if (AnimInst != nullptr)
		{
			_HealItems -= 1;
			OnHealItemsChanged();
			animInst->State = EPlayerState::Heal;
			bIsHealing = true;
			_HealCountingDown = HealInterval;
			AnimInst->Montage_Play(m_pHealMontage);
		}
	}
}

void APlayerCharacter::OnTargetLockPressed()
{
	if (LockedTarget)
	{
		LockedTarget = nullptr;
		UE_LOG(LogTemp, Warning, TEXT("Target lock OFF"));
	}
	else
	{
		LockedTarget = FindBestTarget();
		if (LockedTarget)
		{
			UE_LOG(LogTemp, Warning, TEXT("Target locked: %s"), *LockedTarget->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("No target found"));
		}
	}
}

void APlayerCharacter::OnSpecialAttackPressed()
{
	if (CanAttack() && !bIsDodging && !bIsAttacking && !bIsHealing && !bIsHit && !bIsInFallRecover && !bIsFalling)
	{
		UPlayerAnimInstance* animInst = Cast<UPlayerAnimInstance>(GetMesh()->GetAnimInstance());
		if (animInst != nullptr)
		{
			if (Controller)
			{
				FRotator ControlRotation = Controller->GetControlRotation();
				FRotator NewRotation(0.f, ControlRotation.Yaw, 0.f);
				SetActorRotation(NewRotation);
			}
			_Weapon->EnableOverlapReaction();
			animInst->State = EPlayerState::SpecialAttack;
			bIsAttacking = true;
			_AttackCountingDown = SpecialAttackInterval;
		}
	}
}

ACharacter* APlayerCharacter::FindBestTarget()
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	TArray<AActor*> FoundEnemies;
	UGameplayStatics::GetAllActorsOfClass(World, ACharacter::StaticClass(), FoundEnemies);

	FVector MyLocation = GetActorLocation();
	FVector Forward = GetFollowCamera() ? GetFollowCamera()->GetForwardVector() : GetActorForwardVector();

	ACharacter* BestTarget = nullptr;
	float BestScore = FLT_MAX;

	for (AActor* Actor : FoundEnemies)
	{
		ACharacter* Enemy = Cast<ACharacter>(Actor);
		if (!Enemy) continue;

		FVector ToEnemy = Enemy->GetActorLocation() - MyLocation;
		float Distance = ToEnemy.Size();
		if (Distance > TargetLockRadius) continue;

		float Angle = FMath::RadiansToDegrees(acosf(FVector::DotProduct(Forward.GetSafeNormal(), ToEnemy.GetSafeNormal())));
		if (Angle > TargetLockMaxAngle) continue;


		float Score = Distance + Angle * 10.f; 
		if (Score < BestScore)
		{
			BestScore = Score;
			BestTarget = Enemy;
		}
	}
	return BestTarget;
}

void APlayerCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void APlayerCharacter::ShowYouDiedWidget()
{
	UE_LOG(LogTemp, Log, TEXT("You Died"));
	if (!YouDiedWidgetClass) return;

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		YouDiedWidget = CreateWidget<UYouDiedWidget>(PC, YouDiedWidgetClass);
		if (!YouDiedWidget) return;
		YouDiedWidget->AddToViewport();


			YouDiedWidget->PlayAnimation(YouDiedWidget->FadeInAnimation);
		

		GetWorldTimerManager().SetTimer(TimerHandle_FadeOut, this, &APlayerCharacter::PerformRespawn, 2.0f, false);
	}
}

void APlayerCharacter::HideYouDiedWidget()
{
	if (!YouDiedWidget) return;
	if (YouDiedWidget->FadeOutAnimation)
	{
		YouDiedWidget->PlayAnimation(YouDiedWidget->FadeOutAnimation);
		UE_LOG(LogTemp, Log, TEXT("FadeOut"));
		FTimerHandle RemoveHandle;
		GetWorldTimerManager().SetTimer(RemoveHandle, [this]()
			{
				if (YouDiedWidget)
				{
					YouDiedWidget->RemoveFromParent();
					YouDiedWidget = nullptr;
				}
			}, 1.0f, false);
	}
	else
	{
		YouDiedWidget->RemoveFromParent();
		YouDiedWidget = nullptr;
	}
}

void APlayerCharacter::Die()
{
	if (bIsDead) return;
	bIsDead = true;

	UPlayerAnimInstance* animInst = Cast<UPlayerAnimInstance>(GetMesh()->GetAnimInstance());
	if (animInst)
	{
		animInst->State = EPlayerState::Die;
	}


}

void APlayerCharacter::RespawnBuff()
{
	if (ExplosionBlueprintClass)
	{
		FVector SpawnLocation = GetActorLocation();
		FRotator SpawnRotation = GetActorRotation();

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = GetInstigator();

		GetWorld()->SpawnActor<AExplosion>(ExplosionBlueprintClass, SpawnLocation, SpawnRotation, SpawnParams);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ExplosionBlueprintClass isn't setup!"));
	}
	_HealthPoints = 500;
	OnStatsChanged();
}

void APlayerCharacter::ActivateBuff()
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
	Damage = Damage + 50;
}

void APlayerCharacter::PerformRespawn()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	ACourseProjectGameMode* GameMode = Cast<ACourseProjectGameMode>(GetWorld()->GetAuthGameMode());
	if (!GameMode) return;

	_Weapon->Destroy();
	if (StatsBarWidgetInstance)
	{
		StatsBarWidgetInstance->RemoveFromParent();
		StatsBarWidgetInstance = nullptr;
	}
	Destroy();
	FVector RespawnLocation;
	if (InWhichLocation())
	{
		RespawnLocation = FVector(13474.492933f, -17617.287361f, -70.0f);
	}
	else
	{
		RespawnLocation = FVector(-16304.591487f, 18906.924007f, 2996.836762f);
	}

	FTransform SpawnTransform(RespawnLocation);
	GameMode->RestartPlayerAtTransform(PC, SpawnTransform);
	GameMode->RetargetAllEnemies();
	UE_LOG(LogTemp, Warning, TEXT("Restarted"));

	bIsDead = false;
	HideYouDiedWidget();
}

int APlayerCharacter::InWhichLocation()
{
	FVector CurrentLocation = this->GetActorLocation();

	if ((CurrentLocation.X >= 7119 && CurrentLocation.Y <= -9132)
		|| (CurrentLocation.X >= 5129 && CurrentLocation.Y <= -2632)
		|| (CurrentLocation.X >= 2619 && CurrentLocation.Y <= 1337)
		||(CurrentLocation.X >= 512 && CurrentLocation.Y <= 5967)
		|| (CurrentLocation.X >= -1370 && CurrentLocation.Y <= 11917)
		|| (CurrentLocation.X >= -4960 && CurrentLocation.Y <= 18387))
	{
		//Lava Location
		return 1;
	}
	return 0;
}

void APlayerCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();
	if (!bIsDodging && !bIsAttacking && !bIsHealing && !bIsHit && !bIsInFallRecover && !bIsDead && !bIsInRespawnProcess)
	{
		if (Controller != nullptr)
		{
			// find out which way is forward
			const FRotator Rotation = Controller->GetControlRotation();
			const FRotator YawRotation(0, Rotation.Yaw, 0);

			// get forward vector
			const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

			// get right vector 
			const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

			// add movement 
			AddMovementInput(ForwardDirection, MovementVector.Y);
			AddMovementInput(RightDirection, MovementVector.X);
		}
	}
	LastMovementInput = MovementVector;
	
}

void APlayerCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}
