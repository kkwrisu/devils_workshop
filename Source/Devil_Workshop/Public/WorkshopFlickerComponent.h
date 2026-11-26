#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WorkshopFlickerComponent.generated.h"

class UMaterialInstanceDynamic;
class USoundAttenuation;
class USoundBase;
class USpotLightComponent;
class UStaticMeshComponent;

/** Controls a bulb mesh's emissive material and its associated spotlight together. */
UCLASS(ClassGroup = (Lighting), meta = (BlueprintSpawnableComponent))
class DEVIL_WORKSHOP_API UWorkshopFlickerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWorkshopFlickerComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Workshop|Lighting")
	void StartFlicker(float Duration = 2.0f);

	UFUNCTION(BlueprintCallable, Category = "Workshop|Lighting")
	void StopFlicker();

	UFUNCTION(BlueprintCallable, Category = "Workshop|Lighting")
	void SetRandomEventEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Workshop|Lighting")
	bool CanBeTriggeredByRandomEvent() const { return bAllowRandomEvents; }

	UFUNCTION(BlueprintPure, Category = "Workshop|Lighting")
	bool IsFlickering() const { return bIsFlickering; }

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Workshop|Lighting")
	TObjectPtr<USpotLightComponent> Spotlight;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Workshop|Lighting")
	TObjectPtr<UStaticMeshComponent> BulbMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Lighting")
	bool bAllowRandomEvents = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Lighting|Material")
	FName EmissiveScalarParameter = TEXT("EmissiveIntensity");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Lighting|Material")
	FName EmissiveVectorParameter = TEXT("EmissiveColor");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Lighting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float EmissiveIntensityWhenOff = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Lighting|Audio")
	TObjectPtr<USoundBase> FlickerSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Lighting|Audio")
	TObjectPtr<USoundAttenuation> FlickerAttenuation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Lighting|Audio", meta = (ClampMin = "0.0"))
	float FlickerSoundCooldown = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Lighting|Timing", meta = (ClampMin = "0.01"))
	float PulseIntervalMin = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Workshop|Lighting|Timing", meta = (ClampMin = "0.01"))
	float PulseIntervalMax = 0.22f;

private:
	void CacheOriginalLighting();
	void ApplyFlickerState(bool bLit);
	void ToggleFlicker();
	void FinishFlicker();
	void PlayFlickerSound();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterials;

	TArray<float> OriginalEmissiveScalars;
	TArray<FLinearColor> OriginalEmissiveVectors;
	TArray<bool> HasEmissiveScalar;
	TArray<bool> HasEmissiveVector;
	float OriginalSpotlightIntensity = 0.0f;
	float FlickerEndTime = 0.0f;
	float LastFlickerSoundTime = -1000.0f;
	bool bOriginalSpotlightVisible = true;
	bool bIsFlickering = false;
	bool bFlickerCurrentlyLit = true;
	FTimerHandle PulseTimerHandle;
	FTimerHandle EndTimerHandle;
};
