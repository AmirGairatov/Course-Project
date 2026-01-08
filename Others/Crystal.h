// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraSystem.h"
#include "Components/SphereComponent.h"
#include "Sound/SoundCue.h"
#include "Crystal.generated.h"

UCLASS()
class COURSEPROJECT_API ACrystal : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACrystal();
private:
	float RunningTime = 0.f;
	float BaseZ = 0.f;

	bool bIsShaking = false;
	FVector OriginalLocation;
	float ShakeElapsed = 0.f;
	float ShakeDuration = 0.7f; 
	float ShakeAmplitude = 10.f;
	float ShakeFrequency = 40.f; 


	FTimerHandle ShakeTimerHandle;

	void StopShaking();
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int HitStands = 3;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int CurrentHitAmount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* _StaticMesh;

	UFUNCTION()
	void OnCrystalBeginOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	bool bIsHintActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision")
	USphereComponent* _SphereComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	UNiagaraSystem* HitEffectNiagara;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	UNiagaraSystem* HitBlockEffectNiagara;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	UNiagaraSystem* DestroyEffectNiagara;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* _StaticMeshPortal;

	UPROPERTY(EditAnywhere, Category = "Audio")
	USoundCue* HitSoundCue;

};
