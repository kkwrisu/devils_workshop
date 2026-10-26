#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MonsterTypes.h"
#include "ILightSource.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UILightSource : public UInterface
{
    GENERATED_BODY()
};

class ILightSource
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LightSource")
    float GetLightIntensity() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LightSource")
    float GetLightRadius() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LightSource")
    FGameplayTag GetLightType() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LightSource")
    bool TryExtinguish(AActor* Extinguisher);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LightSource")
    bool TryDamage(AActor* Damager, float DamageAmount);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LightSource")
    bool IsActive() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LightSource")
    void OnExtinguished(AActor* Extinguisher);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LightSource")
    void OnDamaged(AActor* Damager, float DamageAmount);
};