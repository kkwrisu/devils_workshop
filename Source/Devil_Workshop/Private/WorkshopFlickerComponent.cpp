#include "WorkshopFlickerComponent.h"

#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"

UWorkshopFlickerComponent::UWorkshopFlickerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWorkshopFlickerComponent::BeginPlay()
{
	Super::BeginPlay();
	CacheOriginalLighting();
}

void UWorkshopFlickerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopFlicker();
	Super::EndPlay(EndPlayReason);
}

void UWorkshopFlickerComponent::CacheOriginalLighting()
{
	if (!BulbMesh && GetOwner())
	{
		BulbMesh = GetOwner()->FindComponentByClass<UStaticMeshComponent>();
	}

	DynamicMaterials.Reset();
	OriginalEmissiveScalars.Reset();
	OriginalEmissiveVectors.Reset();
	HasEmissiveScalar.Reset();
	HasEmissiveVector.Reset();

	if (Spotlight)
	{
		OriginalSpotlightIntensity = Spotlight->Intensity;
		bOriginalSpotlightVisible = Spotlight->IsVisible();
	}

	if (!BulbMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("WorkshopFlicker: '%s' nao tem BulbMesh configurado"), *GetNameSafe(GetOwner()));
		return;
	}

	for (int32 MaterialIndex = 0; MaterialIndex < BulbMesh->GetNumMaterials(); ++MaterialIndex)
	{
		UMaterialInstanceDynamic* DynamicMaterial = BulbMesh->CreateAndSetMaterialInstanceDynamic(MaterialIndex);
		DynamicMaterials.Add(DynamicMaterial);

		float ScalarValue = 1.0f;
		const bool bHasScalar = DynamicMaterial && !EmissiveScalarParameter.IsNone()
			&& DynamicMaterial->GetScalarParameterValue(FMaterialParameterInfo(EmissiveScalarParameter), ScalarValue);
		OriginalEmissiveScalars.Add(ScalarValue);
		HasEmissiveScalar.Add(bHasScalar);

		FLinearColor VectorValue = FLinearColor::White;
		const bool bHasVector = DynamicMaterial && !EmissiveVectorParameter.IsNone()
			&& DynamicMaterial->GetVectorParameterValue(FMaterialParameterInfo(EmissiveVectorParameter), VectorValue);
		OriginalEmissiveVectors.Add(VectorValue);
		HasEmissiveVector.Add(bHasVector);
	}

	if (!HasEmissiveScalar.Contains(true) && !HasEmissiveVector.Contains(true))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("WorkshopFlicker: material de '%s' nao expoe '%s' nem '%s'; o Spot Light ainda pode piscar"),
			*GetNameSafe(GetOwner()), *EmissiveScalarParameter.ToString(), *EmissiveVectorParameter.ToString());
	}
}

void UWorkshopFlickerComponent::StartFlicker(float Duration)
{
	if (!GetWorld() || Duration <= 0.0f)
	{
		return;
	}

	if (DynamicMaterials.IsEmpty())
	{
		CacheOriginalLighting();
	}

	GetWorld()->GetTimerManager().ClearTimer(PulseTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(EndTimerHandle);
	bIsFlickering = true;
	bFlickerCurrentlyLit = true;
	FlickerEndTime = GetWorld()->GetTimeSeconds() + Duration;
	ApplyFlickerState(true);
	PlayFlickerSound();
	GetWorld()->GetTimerManager().SetTimer(EndTimerHandle, this, &UWorkshopFlickerComponent::FinishFlicker, Duration, false);
	ToggleFlicker();
}

void UWorkshopFlickerComponent::StopFlicker()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(PulseTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(EndTimerHandle);
	}

	if (bIsFlickering)
	{
		bIsFlickering = false;
		ApplyFlickerState(true);
	}
}

void UWorkshopFlickerComponent::SetRandomEventEnabled(bool bEnabled)
{
	bAllowRandomEvents = bEnabled;
}

void UWorkshopFlickerComponent::ToggleFlicker()
{
	if (!bIsFlickering || !GetWorld())
	{
		return;
	}

	if (GetWorld()->GetTimeSeconds() >= FlickerEndTime)
	{
		FinishFlicker();
		return;
	}

	bFlickerCurrentlyLit = !bFlickerCurrentlyLit;
	ApplyFlickerState(bFlickerCurrentlyLit);
	if (!bFlickerCurrentlyLit)
	{
		PlayFlickerSound();
	}

	const float Interval = FMath::FRandRange(PulseIntervalMin, FMath::Max(PulseIntervalMin, PulseIntervalMax));
	GetWorld()->GetTimerManager().SetTimer(PulseTimerHandle, this, &UWorkshopFlickerComponent::ToggleFlicker, Interval, false);
}

void UWorkshopFlickerComponent::FinishFlicker()
{
	if (!bIsFlickering)
	{
		return;
	}

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(PulseTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(EndTimerHandle);
	}
	bIsFlickering = false;
	bFlickerCurrentlyLit = true;
	ApplyFlickerState(true);
}

void UWorkshopFlickerComponent::ApplyFlickerState(bool bLit)
{
	const float EmissiveScale = bLit ? 1.0f : EmissiveIntensityWhenOff;
	if (Spotlight)
	{
		Spotlight->SetVisibility(bLit ? bOriginalSpotlightVisible : false);
		Spotlight->SetIntensity(bLit ? OriginalSpotlightIntensity : 0.0f);
	}

	for (int32 Index = 0; Index < DynamicMaterials.Num(); ++Index)
	{
		UMaterialInstanceDynamic* Material = DynamicMaterials[Index];
		if (!Material)
		{
			continue;
		}
		if (HasEmissiveScalar.IsValidIndex(Index) && HasEmissiveScalar[Index])
		{
			Material->SetScalarParameterValue(EmissiveScalarParameter, OriginalEmissiveScalars[Index] * EmissiveScale);
		}
		else if (HasEmissiveVector.IsValidIndex(Index) && HasEmissiveVector[Index])
		{
			Material->SetVectorParameterValue(EmissiveVectorParameter, OriginalEmissiveVectors[Index] * EmissiveScale);
		}
	}
}

void UWorkshopFlickerComponent::PlayFlickerSound()
{
	if (!FlickerSound || !GetWorld() || !GetOwner())
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastFlickerSoundTime < FlickerSoundCooldown)
	{
		return;
	}
	LastFlickerSoundTime = Now;
	UGameplayStatics::PlaySoundAtLocation(this, FlickerSound, GetOwner()->GetActorLocation(), 1.0f, 1.0f, 0.0f, FlickerAttenuation);
}
