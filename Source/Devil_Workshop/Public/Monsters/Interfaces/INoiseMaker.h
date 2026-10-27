#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Monsters/MonsterTypes.h"
#include "INoiseMaker.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UNoiseMaker : public UInterface
{
    GENERATED_BODY()
};

class INoiseMaker
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NoiseMaker")
    void MakeNoise(const FGameplayTag& NoiseTag, float Loudness = 1.0f, float OverrideRange = -1.0f);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NoiseMaker")
    void MakeNoiseEx(const FNoiseEvent& NoiseEvent);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NoiseMaker")
    bool IsMakingContinuousNoise() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NoiseMaker")
    FGameplayTag GetContinuousNoiseTag() const;
};