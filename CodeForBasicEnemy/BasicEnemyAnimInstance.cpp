#include "BasicEnemyAnimInstance.h"
#include "BasicEnemy.h"

void UBasicEnemyAnimInstance::OnStateAnimationEnds()
{
	if (State == EBasicEnemyState::Spawn)
	{
		State = EBasicEnemyState::Locomotion;
	}
	if (State == EBasicEnemyState::Attack || State == EBasicEnemyState::AttackUP || State == EBasicEnemyState::HeavyAttack)
	{
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
