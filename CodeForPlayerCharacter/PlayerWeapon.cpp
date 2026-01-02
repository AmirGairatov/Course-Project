// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerWeapon.h"
#include "PlayerCharacter.h"
#include "BasicEnemy.h"
#include "CastleEnemy.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
// Sets default values
APlayerWeapon::APlayerWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	_StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	SetRootComponent(_StaticMesh);

	_StaticMesh->SetGenerateOverlapEvents(true);
	_StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); 
	_StaticMesh->SetCollisionObjectType(ECC_WorldDynamic);
	_StaticMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	_StaticMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

// Called when the game starts or when spawned
void APlayerWeapon::BeginPlay()
{
	Super::BeginPlay();
	OnActorBeginOverlap.AddDynamic(this, &APlayerWeapon::OnWeaponBeginOverlap);
}

// Called every frame
void APlayerWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APlayerWeapon::OnWeaponBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	UE_LOG(LogTemp, Warning, TEXT("OverlappedActor class: %s"), OverlappedActor ? *OverlappedActor->GetClass()->GetName() : TEXT("nullptr"));
	UE_LOG(LogTemp, Warning, TEXT("OtherActor class: %s"), OtherActor ? *OtherActor->GetClass()->GetName() : TEXT("nullptr"));
	if (bCanDealDamage)
	{
		UE_LOG(LogTemp, Log, TEXT("Hit"));
		APlayerCharacter* Player = Cast<APlayerCharacter>(Holder);
		if (Player)
		{
			UE_LOG(LogTemp, Log, TEXT("Enemy Overlapped"));
			auto actor = Cast<ABasicEnemy>(OtherActor);
			if (actor!=nullptr)
			{
				bCanDealDamage = false;
				actor->Hit(Player->GetPlayerDamage());
				if (GolemHitSoundCue)
				{
					UGameplayStatics::PlaySoundAtLocation(this, GolemHitSoundCue, GetActorLocation());
				}
			}
			else
			{
				auto actor2 = Cast<ACastleEnemy>(OtherActor);
				if (actor2)
				{
					bCanDealDamage = false;
					actor2->Hit(Player->GetPlayerDamage());
					if (HumanHitSoundCue)
					{
						UGameplayStatics::PlaySoundAtLocation(this, HumanHitSoundCue, GetActorLocation());
					}
				}
			}
		}
		else
		{
			auto player = Cast<APlayerCharacter>(OtherActor);
			ACastleEnemy *CastleEnemy = Cast<ACastleEnemy>(Holder);
			if (CastleEnemy && player != nullptr)
			{
				UE_LOG(LogTemp, Log, TEXT("Player Overlapped"));
			}
			if (player != nullptr)
			{
				UE_LOG(LogTemp, Log, TEXT("Player Hit"));
				bCanDealDamage = false;
				player->Hit(CastleEnemy->GetEnemyDamage(), false, CastleEnemy->GetActorLocation());
				if (HumanHitSoundCue && !player->bIsDead && !player->bIsInvulnerable)
				{
					UGameplayStatics::PlaySoundAtLocation(this, HumanHitSoundCue, GetActorLocation());
				}
				if (player->IsKilled())
				{
					CastleEnemy->_ChasedTarget = nullptr;
				}
			}
		}
		
	}
}

void APlayerWeapon::EnableOverlapReaction()
{
	if (_StaticMesh)
	{
		_StaticMesh->SetGenerateOverlapEvents(true);
		_StaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		UE_LOG(LogTemp, Log, TEXT("Overlap reaction enabled"));
	}
}

void APlayerWeapon::DisableOverlapReaction()
{
	if (_StaticMesh)
	{
		_StaticMesh->SetGenerateOverlapEvents(false);
		_StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		UE_LOG(LogTemp, Log, TEXT("Overlap reaction disabled"));
	}
}

