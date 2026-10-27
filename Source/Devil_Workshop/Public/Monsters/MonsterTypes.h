#pragma once

#include "CoreMinimal.h"
#include "GameplayTags.h"
#include "MonsterTypes.generated.h"

namespace MonsterTags
{
    // Noise Events
    extern DEVIL_WORKSHOP_API FGameplayTag Noise_Event;
    extern DEVIL_WORKSHOP_API FGameplayTag Noise_Event_Loud;
    extern DEVIL_WORKSHOP_API FGameplayTag Noise_Event_Quiet;
    extern DEVIL_WORKSHOP_API FGameplayTag Noise_Event_ScrewTurn;
    extern DEVIL_WORKSHOP_API FGameplayTag Noise_Event_Footstep_Walk;
    extern DEVIL_WORKSHOP_API FGameplayTag Noise_Event_Footstep_Run;
    extern DEVIL_WORKSHOP_API FGameplayTag Noise_Event_Door_Open;
    extern DEVIL_WORKSHOP_API FGameplayTag Noise_Event_Door_Close;
    extern DEVIL_WORKSHOP_API FGameplayTag Noise_Event_Item_Drop;
    extern DEVIL_WORKSHOP_API FGameplayTag Noise_Event_Radio_Active;

    // Resources (FNAF style)
    extern DEVIL_WORKSHOP_API FGameplayTag Resource_Electricity;
    extern DEVIL_WORKSHOP_API FGameplayTag Resource_Oil;
    extern DEVIL_WORKSHOP_API FGameplayTag Resource_Battery;
    extern DEVIL_WORKSHOP_API FGameplayTag Resource_Camera;

    // Creature States
    extern DEVIL_WORKSHOP_API FGameplayTag State_Idle;
    extern DEVIL_WORKSHOP_API FGameplayTag State_Patrol;
    extern DEVIL_WORKSHOP_API FGameplayTag State_Investigate;
    extern DEVIL_WORKSHOP_API FGameplayTag State_Chase;
    extern DEVIL_WORKSHOP_API FGameplayTag State_Attack;
    extern DEVIL_WORKSHOP_API FGameplayTag State_Hiding;
    extern DEVIL_WORKSHOP_API FGameplayTag State_Peeking;

    // Creature Types
    extern DEVIL_WORKSHOP_API FGameplayTag Creature_Peeker;
    extern DEVIL_WORKSHOP_API FGameplayTag Creature_LightHater;
    extern DEVIL_WORKSHOP_API FGameplayTag Creature_CeilingCrawler;
    extern DEVIL_WORKSHOP_API FGameplayTag Creature_GlowHider;

    // Light
    extern DEVIL_WORKSHOP_API FGameplayTag Light_Source;
    extern DEVIL_WORKSHOP_API FGameplayTag Light_Flashlight;
    extern DEVIL_WORKSHOP_API FGameplayTag Light_Environmental;

    // Helper para inicializar todas as tags (chamar no BeginPlay do GameMode ou similar)
    DEVIL_WORKSHOP_API void InitializeTags();
}

UENUM(BlueprintType)
enum class EMonsterState : uint8
{
    Idle,
    Patrol,
    Investigate,
    Chase,
    Attack,
    Hiding,
    Peeking,
    Dead
};

UENUM(BlueprintType)
enum class ECreatureType : uint8
{
    Peeker,
    LightHater,
    CeilingCrawler,
    GlowHider
};

USTRUCT(BlueprintType)
struct FNoiseEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGameplayTag NoiseTag;

    UPROPERTY(BlueprintReadWrite)
    FVector Location;

    UPROPERTY(BlueprintReadWrite)
    float Loudness = 1.0f;

    UPROPERTY(BlueprintReadWrite)
    float HearRange = 1000.0f;

    UPROPERTY(BlueprintReadWrite)
    AActor* Instigator = nullptr;

    UPROPERTY(BlueprintReadWrite)
    float Timestamp = 0.0f;
};

USTRUCT(BlueprintType)
struct FResourceData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGameplayTag ResourceTag;

    UPROPERTY(BlueprintReadWrite)
    float CurrentValue = 100.0f;

    UPROPERTY(BlueprintReadWrite)
    float MaxValue = 100.0f;

    UPROPERTY(BlueprintReadWrite)
    float DrainRatePerSecond = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    bool bIsCritical = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMonsterStateChanged, EMonsterState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMonsterDetectedPlayer, AActor*, PlayerActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMonsterJumpscare, AActor*, MonsterActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnResourceChanged, FGameplayTag, ResourceTag, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnResourceDepleted, FGameplayTag, ResourceTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnResourceCritical, FGameplayTag, ResourceTag);