// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AWeapon::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const FString WeaponTablePath{TEXT("DataTable'/Game/DataTable/WeaponDataTable.WeaponDataTable'")};
	UDataTable* WeaponTableObject = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *WeaponTablePath));

	if (WeaponTableObject)
	{
		FWeaponDataTable* WeaponDataRow = nullptr;
		
		switch (WeaponType)
		{
		case EWeaponType::EWT_SubmachineGun:
			WeaponDataRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("SubmachineGun"), TEXT(""));
			break;
		case EWeaponType::EWT_AssaultRifle:
			WeaponDataRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("AssaultRifle"), TEXT(""));
			break;
		case EWeaponType::EWT_Pistol:
			WeaponDataRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("Pistol"), TEXT(""));
			break;
		default:
			break;
		}

		if (WeaponDataRow)
		{
			AmmoType = WeaponDataRow->AmmoType;
			Ammo = WeaponDataRow->WeaponAmmo;
			MagazineCapacity = WeaponDataRow->MagazineCapacity;
			SetPickupSound(WeaponDataRow->PickupSound);
			SetEquipSound(WeaponDataRow->EquipSound);
			SetItemName(WeaponDataRow->ItemName);
			SetIconItem(WeaponDataRow->InventoryIcon);
			SetAmmoIcon(WeaponDataRow->AmmoIcon);
			SetClipBoneName(WeaponDataRow->ClipBoneName);
			SetReloadMontageSection(WeaponDataRow->ReloadMontageSection);
			CrosshairsMiddle = WeaponDataRow->CrosshairsMiddle;
			CrosshairsLeft = WeaponDataRow->CrosshairsLeft;
			CrosshairsRight = WeaponDataRow->CrosshairsRight;
			CrosshairsBottom = WeaponDataRow->CrosshairsBottom;
			CrosshairsTop = WeaponDataRow->CrosshairsTop;
			AutoFireRate = WeaponDataRow->AutoFireRate;
			FireSound = WeaponDataRow->FireSound;
			MuzzleFlash = WeaponDataRow->MuzzleFlash;
			BoneToHide = WeaponDataRow->BoneToHide;
			bAutomatic = WeaponDataRow->bAutomatic;
			Damage = WeaponDataRow->BaseDamage;
			HeadShotDamage = WeaponDataRow->HeadShotDamage;
			if (GetItemMesh())
			{
				if (BoneToHide != FName("")) GetItemMesh()->HideBoneByName(BoneToHide, EPhysBodyOp::PBO_None);
				GetItemMesh()->SetSkeletalMesh(WeaponDataRow->ItemMesh);
				GetItemMesh()->SetAnimInstanceClass(WeaponDataRow->AnimBP);

			}
			
			PreviousMaterialIndex = GetMaterialIndex();
			// Clear the Material at this index since if we change the weapon, the index could be different and the
			// weapon could end up with 2 materials in different index's
			GetItemMesh()->SetMaterial(PreviousMaterialIndex, nullptr);
			SetMaterialIndex(WeaponDataRow->MaterialIndex);

			if (WeaponDataRow->MaterialInstance)
				SetMaterialInstance(WeaponDataRow->MaterialInstance);
		}

		if (GetMaterialInstance())
		{
			SetDynamicMaterialInstance(UMaterialInstanceDynamic::Create(GetMaterialInstance(), this));
			GetDynamicMaterialInstance()->SetVectorParameterValue(TEXT("FresnelColor"), GetGlowColor());
			if (GetItemMesh())
				GetItemMesh()->SetMaterial(GetMaterialIndex(), GetDynamicMaterialInstance());

			EnableGlowMaterial();
		}
	}
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	if (BoneToHide != FName("")) GetItemMesh()->HideBoneByName(BoneToHide, EPhysBodyOp::PBO_None);
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Keep the Weapon upright
	if (GetItemState() == EItemState::EIS_Falling && bFalling && GetItemMesh())
	{
		const FRotator MeshRotation(0.f, GetItemMesh()->GetComponentRotation().Yaw, 0.f);
		GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}

	UpdateSlideDisplacement();
}

void AWeapon::UpdateSlideDisplacement()
{
	if (SlideDisplacementCurve == nullptr || !bMovingSlide) return;

	const float ElapsedTime{ GetWorldTimerManager().GetTimerElapsed(SlideTimer) };
	const float CurveValue{ SlideDisplacementCurve->GetFloatValue(ElapsedTime) };

	SlideDisplacement = CurveValue * MaxSlideDisplacement;
	RecoilRotation = CurveValue * MaxRecoilRotation;
}

void AWeapon::FinishMovingSlide()
{
	bMovingSlide = false;
}

void AWeapon::ThrowWeapon()
{
	if (GetItemMesh())
	{
		const FRotator MeshRotation(0.f, GetItemMesh()->GetComponentRotation().Yaw, 0.f);
		GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);

		const FVector MeshForward(GetItemMesh()->GetForwardVector());
		const FVector MeshRight(GetItemMesh()->GetRightVector());
		
		// Direction in which we throw the weapon
		FVector ImpulseDirection = MeshRight.RotateAngleAxis(-20.f, MeshForward);

		const float RandomRotation(FMath::FRandRange(0.f, 30.f));
		ImpulseDirection = ImpulseDirection.RotateAngleAxis(RandomRotation, FVector(0.f, 0.f, 1.f));
		ImpulseDirection *= 10'000.f;
		
		GetItemMesh()->AddImpulse(ImpulseDirection);

		bFalling = true;
		GetWorldTimerManager().SetTimer(ThrowWeaponTimer, this, &ThisClass::StopFalling, ThrowWeaponTime);
	}

	EnableGlowMaterial();
}

void AWeapon::DecrementAmmo()
{
	if (Ammo - 1 <= 0)
	{
		Ammo = 0;
		return;
	}

	--Ammo;
}

void AWeapon::ReloadAmmo(int32 Amount)
{
	// It will stop the execution of the function if true
	checkf(Ammo + Amount <= MagazineCapacity, TEXT("Attempted to reload with more than magazine capacity."));
	Ammo += Amount;
}

bool AWeapon::IsClipFull() const
{
	return Ammo >= MagazineCapacity;
}

void AWeapon::StartSlideTimer()
{
	bMovingSlide = true;
	GetWorldTimerManager().SetTimer(SlideTimer, this, &AWeapon::FinishMovingSlide, SlideDisplacementTime);
}

void AWeapon::StopFalling()
{
	bFalling = false;
	SetItemState(EItemState::EIS_Pickup);
	StartPulseTimer();
}

