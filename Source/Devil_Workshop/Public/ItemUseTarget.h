#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InventoryTypes.h"
#include "ItemUseTarget.generated.h"

UINTERFACE(BlueprintType)
class UItemUseTarget : public UInterface
{
	GENERATED_BODY()
};

class IItemUseTarget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item")
	bool CanUseItem(const FInventoryItem& Item);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item")
	bool UseItem(const FInventoryItem& Item, AActor* User);
};