// Fill out your copyright notice in the Description page of Project Settings.


#include "CastleEnemyAnimInstance.h"
#include "CastleEnemy.h"
#include "CastleEnemyController.h"
void UCastleEnemyAnimInstance::OnStateAnimationEnds()
{
	UE_LOG(LogTemp, Error, TEXT("OnAnimEnds"));
	if (State == ECastleEnemyState::Spawn)
	{
		auto ownerActor = this->GetOwningActor();
		auto castleEn = Cast<ACastleEnemy>(ownerActor);
		if (castleEn)
		{
			castleEn->SpawnProcessEnds();
		}
		State = ECastleEnemyState::Locomotion;
	}
	if (State == ECastleEnemyState::Attack1 || State == ECastleEnemyState::Attack2 || State == ECastleEnemyState::Attack3)
	{
		UE_LOG(LogTemp, Error, TEXT("EnterRecovery"));
		auto ownerActor = this->GetOwningActor();
		auto castleEn = Cast<ACastleEnemy>(ownerActor);
		if (castleEn)
		{
			auto enemyController = Cast<ACastleEnemyController>(castleEn->GetController());
			if (enemyController)
			{
				castleEn->_Weapon->DisableOverlapReaction();
				castleEn->_Weapon->bCanDealDamage = true;
				castleEn->_AttackRecoverCountingDown = castleEn->AttackRecoverInterval;
				castleEn->bIsInAttackRecover = true;
				enemyController->MakeAttackDecision(castleEn->_ChasedTarget);
			}
		}

		//State = ECastleEnemyState::Attack2;
	}
	else if (State == ECastleEnemyState::Attack1Recovery || State == ECastleEnemyState::Attack2Recovery || State == ECastleEnemyState::Attack3Recovery || State == ECastleEnemyState::Attack4)
	{
		auto ownerActor = this->GetOwningActor();
		auto castleEn = Cast<ACastleEnemy>(ownerActor);
		if (castleEn)
		{
			castleEn->bIsAttacking = false;

			castleEn->AttackIndex = 0;
			
			castleEn->bIsInAttackRecover = true;
			castleEn->_AttackRecoverCountingDown = castleEn->AttackRecoverInterval;
		}
		if (State == ECastleEnemyState::Attack4)
		{
			/*castleEn->_HitRecoverCountingDown = castleEn->HitRecoverInterval+1.5f;*/
		}
		State = ECastleEnemyState::Locomotion;
	}
	if (State == ECastleEnemyState::Buff)
	{
		State = ECastleEnemyState::Locomotion;
	}
	if (State == ECastleEnemyState::Hit)
	{
		State = ECastleEnemyState::Locomotion;
		auto ownerActor = this->GetOwningActor();
		auto castleEn = Cast<ACastleEnemy>(ownerActor);
		if (castleEn)
		{
			castleEn->bAreHit = false;
			castleEn->bIsAttacking = false;
			UE_LOG(LogTemp, Error, TEXT("NotHit"));
		}
	}
	if (State == ECastleEnemyState::Die)
	{
		auto ownerActor = this->GetOwningActor();
		auto castleEn = Cast<ACastleEnemy>(ownerActor);
		if (castleEn)
		{
			castleEn->DieProcess();
		}
	}
	//if (State == EBasicEnemyState::Attack || State == EBasicEnemyState::AttackUP || State == EBasicEnemyState::HeavyAttack)
	//{
	//	State = EBasicEnemyState::Locomotion;
	//}
	//else
	//{
	//	auto enemy = Cast<ABasicEnemy>(GetOwningActor());
	//	if (State == EBasicEnemyState::Hit)
	//	{
	//		if (enemy->GetHealthPoints() > 0.0f)
	//		{
	//			State = EBasicEnemyState::Locomotion;
	//		}
	//		else
	//		{
	//			State = EBasicEnemyState::Die;
	//		}
	//	}
	//	else if (State == EBasicEnemyState::Die)
	//	{
	//		enemy->DieProcess();
	//	}

	//}
}

void UCastleEnemyAnimInstance::OnWeaponSocketChanged()
{
	auto ownerActor = this->GetOwningActor();
	auto castleEn = Cast<ACastleEnemy>(ownerActor);
	if (castleEn)
	{
		castleEn->ChangeWeaponSocket();
	}
}

void UCastleEnemyAnimInstance::Buff()
{
	auto ownerActor = this->GetOwningActor();
	auto castleEn = Cast<ACastleEnemy>(ownerActor);
	if (castleEn)
	{
		castleEn->ActivateBuff();
	}
}