// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerAnimInstance.h"
#include "PlayerCharacter.h"

void UPlayerAnimInstance::OnStateAnimationEnds()
{
	if (State == EPlayerState::Heal)
	{
		bool bIsHealing = false;
		State = EPlayerState::Locomotion;
	}
	if (State == EPlayerState::Land)
	{
		UE_LOG(LogTemp, Error, TEXT("FallRecover"));
		State = EPlayerState::FallRecovery;
	}
	else if (State == EPlayerState::FallRecovery)
		{
		UE_LOG(LogTemp, Error, TEXT("FallRecoverEnd"));
		State = EPlayerState::Locomotion;
		auto ownerActor = this->GetOwningActor();
		auto playerCharacter = Cast<APlayerCharacter>(ownerActor);
		if (playerCharacter)
		{
			playerCharacter->bIsInFallRecover = false;
			playerCharacter->bIsAttacking = false;
		}

		}
	if (State == EPlayerState::Roll)
	{
		State = EPlayerState::Locomotion;
	}
	else if (State == EPlayerState::RespawnBuff)
	{
		auto ownerActor = this->GetOwningActor();
		auto playerCharacter = Cast<APlayerCharacter>(ownerActor);
		if (playerCharacter)
		{
			playerCharacter->bIsInRespawnProcess = false;
		}
		State = EPlayerState::Locomotion;
	}
	else if (State == EPlayerState::SpecialAttack)
	{
		State = EPlayerState::Locomotion;
	}
	else if (State == EPlayerState::Attack)
	{
		auto ownerActor = this->GetOwningActor();
		auto playerCharacter = Cast<APlayerCharacter>(ownerActor);
		if (playerCharacter->GetComboIndex() == 1)
		{
			State = EPlayerState::SecondAttack;
		}
		else
		{
			State = EPlayerState::Locomotion;
		}
		playerCharacter->bIsHit = false;
	}
	else if (State == EPlayerState::SecondAttack)
	{
		auto ownerActor = this->GetOwningActor();
		auto playerCharacter = Cast<APlayerCharacter>(ownerActor);
		if (playerCharacter->GetComboIndex() == 3)
		{
			State = EPlayerState::ThirdAttack;
		}
		else
		{
			State = EPlayerState::Locomotion;
		}
		playerCharacter->bIsHit = false;
	}
	else if (State == EPlayerState::ThirdAttack)
	{
		auto ownerActor = this->GetOwningActor();
		auto playerCharacter = Cast<APlayerCharacter>(ownerActor);

		State = EPlayerState::Locomotion;
		if (playerCharacter)
		{
			playerCharacter->bIsHit = false;
		}
	}
	else if (State == EPlayerState::Spawn)
	{
		auto ownerActor = this->GetOwningActor();
		auto playerCharacter = Cast<APlayerCharacter>(ownerActor);
		if (playerCharacter)
		{
			playerCharacter->bIsAttacking = false;
		}
		State = EPlayerState::Locomotion;
	}
	else if (State == EPlayerState::RecoverFromHeavyHit)
	{
		auto ownerActor = this->GetOwningActor();
		auto playerCharacter = Cast<APlayerCharacter>(ownerActor);
		State = EPlayerState::Locomotion;
		if (playerCharacter != nullptr)
		{
			playerCharacter->bIsHit = false;
			playerCharacter->bIsInHitAnimation = false;
		}
	}
	else
	{
		auto ownerActor = this->GetOwningActor();
		auto playerCharacter = Cast<APlayerCharacter>(ownerActor);
		if (playerCharacter == nullptr)
		{
			return;
		}
		if (State == EPlayerState::Hit)
		{
			if (playerCharacter->GetHealthPoints() > 0.0f)
			{
				State = EPlayerState::Locomotion;
			}
			else
			{
				State = EPlayerState::Locomotion;
			}
			playerCharacter->bIsHit = false;
			playerCharacter->bIsInHitAnimation = false;
		}
		else if (State == EPlayerState::HeavHit)
		{
			State = EPlayerState::RecoverFromHeavyHit;
		}
	}
}

void UPlayerAnimInstance::OnWeaponSocketChanged()
{
	auto ownerActor = this->GetOwningActor();
	auto playerCharacter = Cast<APlayerCharacter>(ownerActor);
	if (playerCharacter)
	{
		playerCharacter->ChangeWeaponSocket();
	}
}
void UPlayerAnimInstance::OnDeathWidgetShown()
{
	auto ownerActor = this->GetOwningActor();
	auto playerCharacter = Cast<APlayerCharacter>(ownerActor);
	if (playerCharacter)
	{
		playerCharacter->ShowYouDiedWidget();
	}
}

void UPlayerAnimInstance::Boom()
{
	auto ownerActor = this->GetOwningActor();
	auto playerCharacter = Cast<APlayerCharacter>(ownerActor);
	if (playerCharacter)
	{
		playerCharacter->RespawnBuff();
	}
}
