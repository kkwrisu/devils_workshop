#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MonsterTypes.h"
#include "IMovableTool.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UIMovableTool : public UInterface
{
    GENERATED_BODY()
};

class IMovableTool
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MovableTool")
    bool OnPickedUpByCreature(AActor* Creature);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MovableTool")
    bool OnDroppedByCreature(AActor* Creature, const FVector& NewLocation);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MovableTool")
    FVector GetOriginalLocation() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MovableTool")
    void SetOriginalLocation(const FVector& Location);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MovableTool")
    bool IsInOriginalLocation() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MovableTool")
    float GetTheftPriority() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MovableTool")
    FGameplayTag GetToolTypeTag() const;
};