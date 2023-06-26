#include "Character/Slay.h"
#include "DrawDebugHelpers.h"
#include "SwordOfVengeance/DebugMacro.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "DataAsset/InputActions.h"
#include "Item/Equipment/Weapon/Weapon.h"
#include "Character/Animation/SlayAnimInstance.h"
#include "MotionWarpingComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TargetSystem/TargetSystemComponent.h"
#include "Components/CapsuleComponent.h"

ASlay::ASlay() :
	ActionState(EActionState::EAS_None),
	CharacterState(ECharacterState::ECS_UnEquipped),
	KatanaCombo(0),
	bCanAttack(true),
	CombatStateTime(15.f),
	bBattleMode(false),
	LastInputDirection(FVector::ZeroVector),
	RollingDirection(FVector::ZeroVector),
	BattleState(EBattleState::EBS_Idle)
{
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->SocketOffset = FVector(0.f, 0.f, 65.f);

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom);
	FollowCamera->bUsePawnControlRotation = false;
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -89.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.f, 0.0f);

	MotionWarpingComp = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComp"));

	TargetSystem = CreateDefaultSubobject<UTargetSystemComponent>(TEXT("TargetSystem"));
}

void ASlay::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	SlayAnimInstance = Cast<USlayAnimInstance>(GetMesh()->GetAnimInstance());

	//TODO
	//������ �������� �Ȱ��� ������ �ڵ带 �����
	FAttachmentTransformRules AttachmentTransformRules(EAttachmentRule::SnapToTarget, true);

	AWeapon* KatanaSheath = GetWorld()->SpawnActor<AWeapon>(KatanaSheathClass, FVector::ZeroVector, FRotator::ZeroRotator);
	if (KatanaSheath)
	{

		KatanaSheath->AttachToComponent(GetMesh(), AttachmentTransformRules, FName("BeltSwordSocket"));
	}

	AWeapon* Katana = GetWorld()->SpawnActor<AWeapon>(KatanaClass, FVector::ZeroVector, FRotator::ZeroRotator);
	if (Katana)
	{

		Katana->AttachToComponent(GetMesh(), AttachmentTransformRules, FName("PutSwordSocket"));

		CurrentWeapon = Katana;
	}

	if (UnarmedAnimLayer && SlayAnimInstance)
	{
		SlayAnimInstance->LinkAnimClassLayers(UnarmedAnimLayer);
	}
}

void ASlay::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ASlay::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (InputActions == nullptr) return;

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		/*EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);*/
		EnhancedInputComponent->BindAction(InputActions->MoveAction, ETriggerEvent::Triggered, this, &ASlay::Move);
		EnhancedInputComponent->BindAction(InputActions->LookAction, ETriggerEvent::Triggered, this, &ASlay::Look);
		EnhancedInputComponent->BindAction(InputActions->EquipWeaponAction, ETriggerEvent::Triggered, this, &ASlay::EquipWeapon);
		EnhancedInputComponent->BindAction(InputActions->AttackAction, ETriggerEvent::Triggered, this, &ASlay::Attack);
		EnhancedInputComponent->BindAction(InputActions->TargetLookOnAction, ETriggerEvent::Triggered, this, &ASlay::TargetLookOn);
		EnhancedInputComponent->BindAction(InputActions->RollingAction, ETriggerEvent::Triggered, this, &ASlay::Evasion);
		EnhancedInputComponent->BindAction(InputActions->GuardAction, ETriggerEvent::Triggered, this, &ASlay::Guard);
	}
}

void ASlay::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ASlay::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ASlay::EquipWeapon(const FInputActionValue& Value)
{
	if (CharacterState == ECharacterState::ECS_Equipped)
	{
		return;
	}

	if (SlayAnimInstance == nullptr)
	{
		return;
	}

	if (CurrentWeapon == nullptr)
	{
		return;
	}

	TSubclassOf<UAnimInstance> NonCombatAnimClassLayers = CurrentWeapon->GetNonCombatAnimClassLayers();
	if (NonCombatAnimClassLayers)
	{
		SlayAnimInstance->LinkAnimClassLayers(NonCombatAnimClassLayers);
	}

	SlayAnimInstance->PlayEquipMontage(EMontageState::EMS_Equip);

	CharacterState = ECharacterState::ECS_Equipped;

	ActionState = EActionState::EAS_EquippingWeapon;
}

void ASlay::Attack(const FInputActionValue& Value)
{
	bool bCanAttackingState = CanAttackingState(Value);
	if (bCanAttackingState == false)
	{
		return;
	}

	MotionWarpingComp->RemoveWarpTarget(FName("TargetWarp"));

	bCanAttack = false;

	if (KatanaCombo >= 3)
	{
		KatanaCombo = 0;
	}

	if (SlayAnimInstance)
	{
		FName SectionName = FName();
		switch (KatanaCombo)
		{
		case 0:
			SectionName = FName("Combo1");
			MotionWarpingTargectDistance = 35.f;
			break;
		case 1:
			SectionName = FName("Combo2");
			MotionWarpingTargectDistance = 35.f;
			break;
		case 2:
			SectionName = FName("Combo3");
			MotionWarpingTargectDistance = 100.f;
			break;
		}
		KatanaCombo++;

		SlayAnimInstance->PlayAttackMontage(SectionName);
	}

	SetCombatMode();
}

void ASlay::TargetLookOn(const FInputActionValue& Value)
{
	if (TargetSystem)
	{
		if (TargetSystem->GetRockOn() == false)
		{
			if (TargetSystem->TargetLockOn() == true)
			{
				TSubclassOf<UAnimInstance> CombatAnimClassLayers = CurrentWeapon->GetCombatAnimClassLayers();

				SetLinkAnimClassLayers(CombatAnimClassLayers);
			}
		}
		else
		{
			TargetSystem->TargetLockOff();

			TSubclassOf<UAnimInstance> NonCombatAnimClassLayers = CurrentWeapon->GetNonCombatAnimClassLayers();

			SetLinkAnimClassLayers(NonCombatAnimClassLayers);
		}
	}
}

void ASlay::Evasion(const FInputActionValue& Value)
{
	if (ActionState == EActionState::EAS_Attacking ||
		ActionState == EActionState::EAS_Evasion)
	{
		return;
	}

	if (SlayAnimInstance && Controller)
	{
		ActionState = EActionState::EAS_Evasion;

		if (TargetSystem)
		{
			bool bRockOn = TargetSystem->GetRockOn();

			if (bRockOn == false)
			{
				//LaunchCharacter(GetActorForwardVector() * 2000.f, false, false);
				//Debug::Log(GetActorForwardVector() * 2000.f);
			}
			SlayAnimInstance->PlayRollingMontage(bRockOn);
		}
	}
}

void ASlay::Guard(const FInputActionValue& Value)
{
	SlayAnimInstance->PlayGuardMontage();
}

void ASlay::SetCanAttack(const bool& Value)
{
	bCanAttack = Value;
}

ECharacterState ASlay::GetCharacterState() const
{
	return CharacterState;
}

EActionState ASlay::GetActionState() const
{
	return ActionState;
}

void ASlay::SetActionState(const EActionState& State)
{
	ActionState = State;
}

void ASlay::SetCharacterState(const ECharacterState& State)
{
	CharacterState = State;
}

AWeapon* ASlay::GetCurrentWeapon() const
{
	return CurrentWeapon;
}

bool ASlay::GetBattleMode() const
{
	return bBattleMode;
}

FVector ASlay::GetLastInputDirection() const
{
	return LastInputDirection;
}

UTargetSystemComponent* ASlay::GetTargetSystem() const
{
	return TargetSystem;
}

EBattleState ASlay::GetBattleState() const
{
	return BattleState;
}

void ASlay::AttachWeaponToSocket(const FName& SocketName)
{
	if (CurrentWeapon)
	{
		FAttachmentTransformRules AttachmentTransformRules(EAttachmentRule::SnapToTarget, true);

		CurrentWeapon->AttachToComponent(GetMesh(), AttachmentTransformRules, SocketName);
	}
}

void ASlay::ResetComboAttack()
{
	KatanaCombo = 0;
	bCanAttack = true;
	ActionState = EActionState::EAS_Unoccupied;
}

void ASlay::UpdateMotionWarping()
{
	if (TargetSystem == nullptr)
	{
		return;
	}

	if (MotionWarpingComp == nullptr)
	{
		return;
	}

	FVector LastLocation = GetLastMovementInputVector();

	if (LastLocation.IsZero() || TargetSystem->GetRockOn())
	{
		MotionWarpingComp->RemoveWarpTarget(FName("TargetWarp"));

		LastLocation = GetActorForwardVector();
	}

	FHitResult HitResult;
	const FVector Start = GetActorLocation() + (LastLocation * 34.f);
	const FVector End = Start + (LastLocation * 100.f);

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	TEnumAsByte<EObjectTypeQuery> ColiisionWorldStatic = UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldStatic);
	TEnumAsByte<EObjectTypeQuery> CollisionPawn = UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn);

	ObjectTypes.Add(ColiisionWorldStatic);
	ObjectTypes.Add(CollisionPawn);

	TArray<AActor*> IgnoreActor;


	IgnoreActor.Add(this);

	UKismetSystemLibrary::SphereTraceSingleForObjects(
		GetWorld(),
		Start,
		End,
		34.f,
		ObjectTypes,
		false,
		IgnoreActor,
		EDrawDebugTrace::Type::ForDuration,
		HitResult,
		true
	);

	if (HitResult.bBlockingHit)
	{
		MotionWarpingTargectDistance = HitResult.Distance;
	}
	else
	{
		MotionWarpingTargectDistance = 100.f;
	}

	const FVector Location = GetActorLocation();
	const FVector TargetLocation = (LastLocation.GetSafeNormal() * MotionWarpingTargectDistance) + Location;
	const FRotator TargetRotation = UKismetMathLibrary::MakeRotFromX(LastLocation.GetSafeNormal());

	MotionWarpingComp->AddOrUpdateWarpTargetFromLocationAndRotation(
		FName("TargetWarp"),
		TargetLocation,
		TargetRotation);
}

void ASlay::SetCombatMode()
{
	//ActionState = EActionState::EAS_CombatMode;

	bBattleMode = true;

	GetWorldTimerManager().SetTimer(
		CombatTimerHandle,
		this,
		&ASlay::SetNonCombatMode,
		CombatStateTime,
		false);
}

void ASlay::SetNonCombatMode()
{
	//ActionState = EActionState::EAS_NonCombatMode;

	CharacterState = ECharacterState::ECS_UnEquipped;

	bBattleMode = false;

	if (SlayAnimInstance == nullptr)
	{
		return;
	}

	SlayAnimInstance->PlayEquipMontage(EMontageState::EMS_UnEquip);

	if (UnarmedAnimLayer)
	{
		SlayAnimInstance->LinkAnimClassLayers(UnarmedAnimLayer);
	}
}

bool ASlay::CanAttackingState(const FInputActionValue& Value)
{
	if (ActionState == EActionState::EAS_Evasion)
	{
		return false;
	}

	if (CharacterState == ECharacterState::ECS_UnEquipped)
	{
		const FInputActionValue& FInputActionValue(Value);

		EquipWeapon(Value);

		return false;
	}

	if (bCanAttack == false)
	{
		return false;
	}

	if (MotionWarpingComp == nullptr)
	{
		return false;
	}

	if (ActionState == EActionState::EAS_EquippingWeapon)
	{
		return false;
	}

	return true;
}

void ASlay::SetLinkAnimClassLayers(const TSubclassOf<UAnimInstance>& InClass)
{
	if (SlayAnimInstance && InClass)
	{
		SlayAnimInstance->LinkAnimClassLayers(InClass);

		CurrentAnimLayer = InClass;
	}
}

TSubclassOf<UAnimInstance> ASlay::GetLinkAnimClassLayers() const
{
	return CurrentAnimLayer;
}

//TODO
bool ASlay::IsTargetable()
{
	return true;
}