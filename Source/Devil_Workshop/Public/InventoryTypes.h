#pragma once

#include "CoreMinimal.h"
#include "ItemData.h"
#include "InventoryTypes.generated.h"

USTRUCT(BlueprintType)
struct FInventoryItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	int32 InstanceId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	TObjectPtr<UItemData> Data = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	TMap<FName, float> Values;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	TArray<FName> Flags;
};