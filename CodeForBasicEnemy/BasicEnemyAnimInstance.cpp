// Fill out your copyright notice in the Description page of Project Settings.


#include "BasicEnemyAnimInstance.h"
#include "BasicEnemy.h"

void UBasicEnemyAnimInstance::OnStateAnimationEnds()
{
	if (State == EBasicEnemyState::Spawn)
	{
		auto enemy = Cast<ABasicEnemy>(GetOwningActor());
		if (enemy)
		{
			enemy->bAreHit = false;	
		}
		State = EBasicEnemyState::Locomotion;

	}
	if (State == EBasicEnemyState::Attack || State == EBasicEnemyState::AttackUP || State == EBasicEnemyState::HeavyAttack)
	{
		auto enemy = Cast<ABasicEnemy>(GetOwningActor());
		if (enemy)
		{
			enemy->bIsInAttackRecover = true;
			enemy->_AttackRecoverCountingDown = enemy->AttackRecoverInterval;
		}
		State = EBasicEnemyState::Locomotion;
	}
	else
	{
		auto enemy = Cast<ABasicEnemy>(GetOwningActor());
		if (State == EBasicEnemyState::Hit)
		{
			if (enemy->GetHealthPoints() > 0.0f)
			{
				State = EBasicEnemyState::Locomotion;
			}
			else
			{
				State = EBasicEnemyState::Die;
			}
		}
		else if (State == EBasicEnemyState::Die)
		{
			enemy->DieProcess();
		}
		
	}
}

void UBasicEnemyAnimInstance::DamageDealControl(bool wtf)
{
	auto enemy = Cast<ABasicEnemy>(GetOwningActor());
	if (enemy)
	{
		enemy->EnableFistCollision(wtf);
	}
}
