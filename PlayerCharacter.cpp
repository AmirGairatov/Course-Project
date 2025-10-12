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
	static ConstructorHelpers::FObjectFinder<UBlueprint> blueprint_finder (TEXT("Blueprint'/Game/ThirdPerson/Blueprints/BP_PlayerWeapon.BP_PlayerWeapon'"));

	_WeaponClass = (UClass*)blueprint_finder.Object->GeneratedClass;

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
			_HealthPoints += 50;
		}
		OnStatsChanged();
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
	_HealthPoints = HealthPoints - 200;
	_StaminaPoints = StaminaPoints;
	if (AnimInst != nullptr)
	{
		AnimInst->OnPlayMontageNotifyBegin.AddDynamic(this, &APlayerCharacter::HandleOnMontageNotifyIvents);
	}

	_Weapon = Cast<APlayerWeapon>(GetWorld()->SpawnActor(_WeaponClass));
	_Weapon->Holder = this;
	_Weapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, FName("hand_rSocket"));
	_Weapon->DisableOverlapReaction();

	if (StatsBarWidgetClass)
	{
		// —оздаЄм экземпл€р виджета
		UUStatsWidget* CreatedWidget = CreateWidget<UUStatsWidget>(GetWorld(), StatsBarWidgetClass);
		if (CreatedWidget)
		{
			StatsBarWidgetInstance = CreatedWidget;
			StatsBarWidgetInstance->AddToViewport();

			UE_LOG(LogTemp, Log, TEXT("Stats widget created and added to viewport"));

			// сразу выставим значение HP на полоске (если прогрессбар прив€зан)
			if (StatsBarWidgetInstance->HealthProgressBar && StatsBarWidgetInstance->StaminaProgressBar)
			{
				float HealthPercent = FMath::Clamp((float)_HealthPoints / (float)HealthPoints, 0.f, 1.f);
				float StaminaPercent = FMath::Clamp((float)_StaminaPoints / (float)StaminaPoints, 0.f, 1.f);
				StatsBarWidgetInstance->HealthProgressBar->SetPercent(HealthPercent);
				StatsBarWidgetInstance->StaminaProgressBar->SetPercent(StaminaPercent);
			}
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
			Angle = -Angle; // Negative angle for left side
		}

		// Set the direction in the animation instance
		animInst->Direction = Angle;
	}
	else
	{
		// If not moving, set direction to 0
		animInst->Direction = 0.0f;
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
		_StaminaPoints += 0.7f;
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
	if (CanAttack() && !bIsDodging && !bIsAttacking && !bIsHealing)
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
		if (ComboAttackIndex == 0)
		{
			if (!bIsDodging && _StaminaPoints >= 20  && !bIsHealing && (_AttackCountingDown >0.2f && _AttackCountingDown < 0.7f))
			{
				ComboAttackIndex = 1;
			}
		}
		if (ComboAttackIndex == 2)
		{
			if (!bIsDodging && _StaminaPoints >= 20 && !bIsHealing && (_AttackCountingDown > 0.2f && _AttackCountingDown < 0.7f))
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

void APlayerCharacter::Hit(int damage)
{
	_HealthPoints -= damage;
}


void APlayerCharacter::OnRollPressed()
{
	if (!bIsDodging && CanRoll() && !bIsAttacking && !bIsHealing)
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


void APlayerCharacter::OnHealPressed()
{
	if (!bIsDodging && CanHeal() && !bIsAttacking && !bIsHealing)
	{
		UAnimInstance* AnimInst = GetMesh()->GetAnimInstance();
		UPlayerAnimInstance* animInst = Cast<UPlayerAnimInstance>(GetMesh()->GetAnimInstance());
		if (AnimInst != nullptr)
		{
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
		// ≈сли уже есть цель Ц снимаем блокировку
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

ABasicEnemy* APlayerCharacter::FindBestTarget()
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	TArray<AActor*> FoundEnemies;
	UGameplayStatics::GetAllActorsOfClass(World, ABasicEnemy::StaticClass(), FoundEnemies);

	FVector MyLocation = GetActorLocation();
	FVector Forward = GetFollowCamera() ? GetFollowCamera()->GetForwardVector() : GetActorForwardVector();

	ABasicEnemy* BestTarget = nullptr;
	float BestScore = FLT_MAX;

	for (AActor* Actor : FoundEnemies)
	{
		ABasicEnemy* Enemy = Cast<ABasicEnemy>(Actor);
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


void APlayerCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();
	if (!bIsDodging && !bIsAttacking && !bIsHealing)
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
