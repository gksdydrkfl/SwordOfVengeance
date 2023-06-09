// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Animation/AnimNotifyComboEnd.h"
#include "Character/Slay.h"
#include "SkillSystem/SkillSystemComponent.h"
#include "SkillSystem/Skill/KatanaSkill.h"
#include "SwordofVengeance/DebugMacro.h"
void UAnimNotifyComboEnd::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	ASlay* Slay = Cast<ASlay>(MeshComp->GetOwner());

	if (Slay)
	{
		USkillSystemComponent* SkillSystem = Slay->GetSkillSystem();
		if (SkillSystem)
		{
			USkill* Skill = SkillSystem->GetSkill(ESkillType::EST_KatanaBaseAttack);
			if (Skill)
			{
				UKatanaSkill* KatanaSkill = Cast<UKatanaSkill>(Skill);
				if (KatanaSkill)
				{
					KatanaSkill->ResetComboAttack();
				}
			}
		}
	}
}
