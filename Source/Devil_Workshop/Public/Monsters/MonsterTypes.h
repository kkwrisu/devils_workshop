#pragma once

#include "CoreMinimal.h"
#include "GameplayTags.h"
#include "MonsterTypes.generated.h"

namespace MonsterTags
{
    // Noise Events
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Noise_Event);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Noise_Event_Loud);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Noise_Event_Quiet);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Noise_Event_ScrewTurn);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Noise_Event_Footstep_Walk);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Noise_Event_Footstep_Run);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Noise_Event_Door_Open);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Noise_Event_Door_Close);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Noise_Event_Item_Drop);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Noise_Event_Radio_Active);

    // Resources (FNAF style)
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Resource_Electricity);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Resource_Oil);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Resource_Battery);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Resource_Camera);

    // Creature States
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Idle);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Patrol);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Investigate);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Chase);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Attack);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Hiding);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Peeking);

    // Creature Types
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Creature_Peeker);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Creature_LightHater);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Creature_CeilingCrawler);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Creature_GlowHider);

    // Light
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Light_Source);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Light_Flashlight);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Light_Environmental);
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