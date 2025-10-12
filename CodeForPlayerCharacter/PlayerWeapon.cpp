#include "PlayerWeapon.h"
#include "PlayerCharacter.h"
#include "BasicEnemy.h"
// Sets default values
APlayerWeapon::APlayerWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	_StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Stasic Mesh"));
	SetRootComponent(_StaticMesh);

	_StaticMesh->SetGenerateOverlapEvents(true);
	_StaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
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
	UE_LOG(LogTemp, Log, TEXT("Enemy Overlapped"));
	auto actor = Cast<ABasicEnemy>(OtherActor);
	if (actor!=nullptr && bCanDealDamage)
	{
		bCanDealDamage = false;
		UE_LOG(LogTemp, Log, TEXT("Hit"));
		actor->Hit(100);
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

