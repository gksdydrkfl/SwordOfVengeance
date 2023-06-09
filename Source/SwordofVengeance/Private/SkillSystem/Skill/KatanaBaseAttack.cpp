// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/Skill/KatanaBaseAttack.h"
#include "Character/Slay.h"
#include "MotionWarpingComponent.h"
#include "Character/Animation/SlayAnimInstance.h"
#include "Item/Equipment/Weapon/Katana.h"
#include "SwordofVengeance/DebugMacro.h"
UKatanaBaseAttack::UKatanaBaseAttack()
{
	SkillType = ESkillType::EST_KatanaBaseAttack;

	MotionWarpingComp = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComp"));
}

void UKatanaBaseAttack::StartSkill()
{
	if (Slay == nullptr)
	{
		return;
	}

	bool bCanAttackingState = CanAttackingState();
	if (bCanAttackingState == false)
	{
		return;
	}

	MotionWarpingComp->RemoveWarpTarget(FName("TargetWarp"));

	Slay->SetCanAttack(false);

	if (KatanaCombo >= 3)
	{
		KatanaCombo = 0;
	}


	USlayAnimInstance* SlayAnimInstance = Cast<USlayAnimInstance>(Slay->GetMesh()->GetAnimInstance());
	EWeaponSound WeaponSound = EWeaponSound();

	if (SlayAnimInstance)
	{
		FName SectionName = FName();
		switch (KatanaCombo)
		{
		case 0:
			SectionName = FName("Combo1");
			MotionWarpingTargectDistance = 35.f;
			WeaponSound = EWeaponSound::EWS_SwordSwing1;
			break;
		case 1:
			SectionName = FName("Combo2");
			MotionWarpingTargectDistance = 35.f;
			WeaponSound = EWeaponSound::EWS_SwordSwing1;
			break;
		case 2:
			SectionName = FName("Combo3");
			MotionWarpingTargectDistance = 100.f;
			WeaponSound = EWeaponSound::EWS_SwordSwing2;
			break;
		}
		KatanaCombo++;


		Slay->SetMotionWarpingTargectDistance(MotionWarpingTargectDistance);

		SlayAnimInstance->PlayAttackMontage(SectionName);
	}

	AKatana* Katana = Cast<AKatana>(Slay->GetCurrentWeapon());
	if (Katana)
	{
		Katana->SetWeaponSound(WeaponSound);
	}

	Slay->SetCombatMode();
}
