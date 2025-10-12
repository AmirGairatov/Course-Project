// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerAnimInstance.h"
#include "PlayerCharacter.h"

void UPlayerAnimInstance::OnStateAnimationEnds()
{
	if (State == EPlayerState::Heal)
	{
		State = EPlayerState::Locomotion;
	}
	if (State == EPlayerState::Roll)
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
	}
	else if (State == EPlayerState::ThirdAttack)
	{
		State = EPlayerState::Locomotion;
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
				State = EPlayerState::Die;
			}
		}
		else if (State == EPlayerState::Die)
		{
			//...
		}
	}
}
