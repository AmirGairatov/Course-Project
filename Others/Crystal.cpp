// Fill out your copyright notice in the Description page of Project Settings.


#include "Crystal.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "CourseProjectGameMode.h"
#include "PlayerWeapon.h"
#include "Kismet/GameplayStatics.h"
// Sets default values
ACrystal::ACrystal()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	_StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Static Mesh"));
	SetRootComponent(_StaticMesh);
	_StaticMeshPortal = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Static Mesh Portal"));

	_SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	_SphereComponent->SetupAttachment(_StaticMesh);
	_SphereComponent->InitSphereRadius(50.0f);
	_SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	_SphereComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	_SphereComponent->SetGenerateOverlapEvents(true);

}

// Called when the game starts or when spawned
void ACrystal::BeginPlay()
{
	Super::BeginPlay();
	_StaticMeshPortal->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
	_SphereComponent->OnComponentBeginOverlap.AddDynamic(this, &ACrystal::OnCrystalBeginOverlap);
	FVector ActorWorldLocation = GetActorLocation();
	_StaticMeshPortal->SetWorldLocation(ActorWorldLocation + FVector(0.f, 0.f, 6500.f));
	_StaticMeshPortal->SetVisibility(false);
	BaseZ = GetActorLocation().Z;
}

// Called every frame
void ACrystal::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RunningTime += DeltaTime;

	float Amplitude = 20.f;
	float Frequency = 1.f;
	float VerticalOffset = FMath::Sin(RunningTime * Frequency * 2 * PI) * Amplitude;

	FVector Location = GetActorLocation();
	Location.Z = BaseZ + VerticalOffset;
	SetActorLocation(Location);

	FQuat rotQuat = FQuat(FRotator(300.f * DeltaTime, 300.f * DeltaTime, 300.f * DeltaTime));
	AddActorLocalRotation(rotQuat);

	if (bIsShaking)
	{
		ShakeElapsed += DeltaTime;

		float OffsetX = FMath::Sin(ShakeElapsed * ShakeFrequency) * ShakeAmplitude;
		float OffsetY = FMath::Sin(ShakeElapsed * ShakeFrequency * 0.8f) * ShakeAmplitude * 0.8f;

		SetActorLocation(OriginalLocation + FVector(OffsetX, OffsetY, 0));
	}
}

void ACrystal::OnCrystalBeginOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	UE_LOG(LogTemp, Warning, TEXT("World or CrystalBPClass is null!"));
	auto weapon = Cast<AActor>(OtherActor);
	if (weapon == nullptr)
	{
		return;
	}
	auto Weapon = Cast<APlayerWeapon>(weapon);
	ACourseProjectGameMode* GameMode = Cast<ACourseProjectGameMode>(GetWorld()->GetAuthGameMode());
	if (!GameMode) return;
	if (Weapon && GameMode->AliveEnemies.IsEmpty())
	{
		if (!bIsShaking && CurrentHitAmount != HitStands)
		{
			bIsShaking = true;
			ShakeElapsed = 0.f;
			CurrentHitAmount++;
			if (HitSoundCue)
			{
				UGameplayStatics::PlaySoundAtLocation(this, HitSoundCue, GetActorLocation());
			}
			OriginalLocation = GetActorLocation();
			if (HitEffectNiagara)
			{
				UNiagaraFunctionLibrary::SpawnSystemAttached(
					HitEffectNiagara,          
					_StaticMesh,             
					NAME_None,              
					FVector::ZeroVector,     
					FRotator::ZeroRotator,   
					EAttachLocation::SnapToTarget,
					true                     
				);
			}
			GetWorldTimerManager().SetTimer(ShakeTimerHandle, this, &ACrystal::StopShaking, ShakeDuration, false);
		}
		else if (HitStands == CurrentHitAmount)
		{
			if (DestroyEffectNiagara)
			{
				UNiagaraFunctionLibrary::SpawnSystemAttached(
					DestroyEffectNiagara,
					_StaticMesh,
					NAME_None,
					FVector::ZeroVector,
					FRotator::ZeroRotator,
					EAttachLocation::SnapToTarget,
					true
				);
			}
			CurrentHitAmount++;
			GetWorldTimerManager().SetTimer(ShakeTimerHandle, this, &ACrystal::StopShaking, ShakeDuration-0.5f, false);
		}
		
	}
	else
	{
		if (HitBlockEffectNiagara)
		{
			UNiagaraFunctionLibrary::SpawnSystemAttached(
				HitBlockEffectNiagara,
				_StaticMesh,
				NAME_None,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::SnapToTarget,
				true);
		}
	}
}

void ACrystal::StopShaking()
{
	if (HitStands < CurrentHitAmount)
	{
		ACourseProjectGameMode* GameMode = Cast<ACourseProjectGameMode>(GetWorld()->GetAuthGameMode());
		if (GameMode)
		{
			GameMode->CrystalArray.RemoveSingle(this);
			if (bIsHintActive)
			{
				GameMode->bIsHintExists = false;
			}
			GameMode->DecreaseArtefactAmoint();
		}
		PrimaryActorTick.bCanEverTick = false;

		K2_DestroyActor();

		// GEngine->ForceGarbageCollection(true);
	}
	bIsShaking = false;
	SetActorLocation(OriginalLocation);
	GetWorldTimerManager().ClearTimer(ShakeTimerHandle);
}