// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TargetSystem/TargetSystemInterface.h"
#include "Interface/HitInterface.h"
#include "Enemy.generated.h"

UCLASS()
class SWORDOFVENGEANCE_API AEnemy : public ACharacter, public ITargetSystemInterface, public IHitInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEnemy();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


private:

	UPROPERTY(EditAnywhere, Category = "Montage", meta = (AllowPrivateAccess = true))
	UAnimMontage* ReactMontage;

	UPROPERTY(EditAnywhere, Category = "Particle", meta = (AllowPrivateAccess = true))
	UParticleSystem* HitParticles;
	
public:
	virtual bool IsTargetable() override;
	virtual void GetHit(const FVector& HitImpact) override;
};
