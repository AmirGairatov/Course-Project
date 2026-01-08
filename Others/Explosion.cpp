// Fill out your copyright notice in the Description page of Project Settings.


#include "Explosion.h"
#include "CastleEnemy.h"
#include "BasicEnemy.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
// Sets default values
AExplosion::AExplosion()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
    ExplosionArea = CreateDefaultSubobject<USphereComponent>(TEXT("ExplosionArea"));
    ExplosionArea->SetSphereRadius(200.0f);
    ExplosionArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ExplosionArea->SetCollisionObjectType(ECC_WorldDynamic);
    ExplosionArea->SetCollisionResponseToAllChannels(ECR_Ignore);
}

// Called when the game starts or when spawned
void AExplosion::BeginPlay()
{
	Super::BeginPlay();
    if (ExplosionArea)
    {
        ExplosionArea->OnComponentBeginOverlap.AddDynamic(this, &AExplosion::OnAreaBeginOverlap);
    }
    if (BoomSoundCue)
    {
        UGameplayStatics::PlaySoundAtLocation(this, BoomSoundCue, GetActorLocation());
    }
    Boom();
}

// Called every frame
void AExplosion::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AExplosion::Boom()
{
    if (ExplosionEffectNiagara)
    {
        UNiagaraFunctionLibrary::SpawnSystemAttached(
            ExplosionEffectNiagara,
            ExplosionArea,
            NAME_None,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget,
            true
        );
    }

    
    ExplosionArea->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ExplosionArea->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

   UE_LOG(LogTemp, Warning, TEXT("ACTIVE"));

   GetWorldTimerManager().SetTimer(
       DestroyTimerHandle,
       this,
       &AExplosion::DestroySelf,
       1.0f,
       false
   );
    
}

void AExplosion::DestroySelf()
{
    Destroy();
}

void AExplosion::OnAreaBeginOverlap(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult
)
{
    UE_LOG(LogTemp, Log, TEXT("Someone Overlapped"));
    auto actor = Cast<ACastleEnemy>(OtherActor);
    if (actor != nullptr)
    {
        UE_LOG(LogTemp, Log, TEXT("boom Hit"));
        actor->Hit(800);
        return;
    }
    auto actor2 = Cast<ABasicEnemy>(OtherActor);
    if (actor2 != nullptr)
    {
        UE_LOG(LogTemp, Log, TEXT("boom Hit"));
        actor2->Hit(800);
    }

}