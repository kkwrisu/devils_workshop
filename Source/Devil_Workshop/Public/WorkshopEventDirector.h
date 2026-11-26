#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorkshopEventDirector.generated.h"

class APlayerBase;
class USceneComponent;
class UWorkshopFlickerComponent;
class USoundAttenuation;
class USoundBase;

USTRUCT(BlueprintType)
struct FWorkshopAmbientSoundEvent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	TObjectPtr<AActor> SoundPoint = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	TObjectPtr<USoundBase> Sound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;
};

/** Schedules ambient workshop events and keeps the window apparition persistent between visits. */
UCLASS(Blueprintable)
class DEVIL_WORKSHOP_API AWorkshopEventDirector : public AActor
{
	GENERATED_BODY()

public:
	AWorkshopEventDirector();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Workshop|Events|Lighting")
	void StartManualLightFlicker(int32 LightIndex, float Duration = 2.0f);

	UFUNCTION(BlueprintCallable, Category = "Workshop|Events|Lighting")
	void StopManualLightFlicker(int32 LightIndex);

	UFUNCTION(BlueprintPure, Category = "Workshop|Events|Window")
	bool IsWindowHumanoidPresent() const { return bWindowHumanoidPresent; }

protected:
	UPROPERTY(EditAnywhere, Category = "Workshop|Events|General")
	bool bEnableRandomEvents = true;

	UPROPERTY(EditAnywhere, Category = "Workshop|Events|General", meta = (ClampMin = "0.1"))
	float FirstEventDelayMin = 25.0f;

	UPROPERTY(EditAnywhere, Category = "Workshop|Events|General", meta = (ClampMin = "0.1"))
	float FirstEventDelayMax = 45.0f;

	UPROPERTY(EditAnywhere, Category = "Workshop|Events|General", meta = (ClampMin = "0.1"))
	float EventIntervalMin = 40.0f;

	UPROPERTY(EditAnywhere, Category = "Workshop|Events|General", meta = (ClampMin = "0.1"))
	float EventIntervalMax = 75.0f;

	UPROPERTY(EditAnywhere, Category = "Workshop|Events|Sounds")
	TArray<FWorkshopAmbientSoundEvent> AmbientSounds;

	UPROPERTY(EditAnywhere, Category = "Workshop|Events|Sounds")
	TObjectPtr<USoundAttenuation> AmbientSoundAttenuation;

	UPROPERTY(EditAnywhere, Category = "Workshop|Events|Door Knock")
	TObjectPtr<AActor> EntranceKnockPoint;

	UPROPERTY(EditAnywhere, Category = "Workshop|Events|Door Knock")
	TObjectPtr<USoundBase> EntranceKnockSound;

	UPROPERTY(EditAnywhere, Category = "Workshop|Events|Door Knock")
	TObjectPtr<USoundAttenuation> EntranceKnockAttenuation;

	UPROPERTY(EditAnywhere, Category = "Workshop|Events|Lighting")
	TArray<TObjectPtr<UWorkshopFlickerComponent>> FlickerLights;

	UPROPERTY(EditAnywhere, Category = "Workshop|Events|Lighting", meta = (ClampMin = "0.1"))
	float EventFlickerDurationMin = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Workshop|Events|Lighting", meta = (ClampMin = "0.1"))
	float EventFlickerDurationMax = 2.5f;

	UPROPERTY(EditAnywhere, Category = "Workshop|Events|Weights", meta = (ClampMin = "0.0"))
	float AmbientSoundWeight = 55.0f;

	UPROPERTY(EditAnywhere, Category = "Workshop|Events|Weights", meta = (ClampMin = "0.0"))
	float DoorKnockWeight = 25.0f;

	UPROPERTY(EditAnywhere, Category = "Workshop|Events|Weights", meta = (ClampMin = "0.0"))
	float LightFlickerWeight = 20.0f;

	UPROPERTY(EditAnywhere, Category = "Workshop|Events|Window", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WindowAppearanceChance = 0.3f;

	UPROPERTY(EditAnywhere, Category = "Workshop|Events|Window", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WindowStayChanceAfterReturn = 0.5f;

	UPROPERTY(EditInstanceOnly, Category = "Workshop|Events|Window")
	TObjectPtr<AActor> WindowArea;

	UPROPERTY(EditInstanceOnly, Category = "Workshop|Events|Window")
	TObjectPtr<AActor> WorkshopStartArea;

	UPROPERTY(EditInstanceOnly, Category = "Workshop|Events|Window")
	TObjectPtr<AActor> WindowHumanoid;

	UPROPERTY(EditAnywhere, Category = "Workshop|Events|Window")
	float AreaPollInterval = 0.15f;

private:
	enum class ERandomWorkshopEvent : uint8
	{
		AmbientSound,
		EntranceKnock,
		LightFlicker
	};

	void InitializeDirector();
	void FindLevelReferences();
	void DiscoverFlickerComponents();
	AActor* FindActorWithTag(FName Tag) const;
	AActor* FindActorNamed(const FString& NameFragment) const;
	void ScheduleNextEvent(bool bFirstEvent);
	void TryRunRandomEvent();
	void RunAmbientSoundEvent();
	void RunEntranceKnockEvent();
	void RunLightFlickerEvent();
	bool IsPointInPlayerView(const FVector& Point, AActor* PointActor) const;
	void UpdateWindowState();
	void SetWindowHumanoidPresent(bool bPresent);

	UPROPERTY()
	TObjectPtr<APlayerBase> Player;

	UPROPERTY()
	TObjectPtr<AActor> LastStableArea;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	bool bInitialized = false;
	bool bWindowHumanoidPresent = false;
	bool bVisitedWindowSinceStart = false;
	bool bWindowAppearanceRolledThisCycle = false;
	double ActiveGameplayElapsed = 0.0;
	double NextEventAtActiveTime = 0.0;
	FTimerHandle InitializationTimerHandle;
};
