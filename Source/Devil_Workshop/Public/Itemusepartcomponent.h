#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "InventoryTypes.h"
#include "ItemUsePartComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemUsedOnPart, const FInventoryItem&, Item, AActor*, User);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemRejectedOnPart, const FInventoryItem&, Item, AActor*, User);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEmptyHandOnPart, AActor*, User);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UItemUsePartComponent : public UBoxComponent
{
	GENERATED_BODY()

public:
	UItemUsePartComponent();

	UPROPERTY(BlueprintAssignable, Category = "Item Use")
	FOnItemUsedOnPart OnItemUsed;

	UPROPERTY(BlueprintAssignable, Category = "Item Use")
	FOnItemRejectedOnPart OnItemRejected;

	UPROPERTY(BlueprintAssignable, Category = "Item Use")
	FOnEmptyHandOnPart OnEmptyHand;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Use")
	FName PartId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Use")
	FName RequiredItemId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Use")
	FName RequiredItemTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Use")
	bool bConsumeItem = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Use")
	bool bSingleUse = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Use")
	TObjectPtr<UItemData> RewardItem;

	UPROPERTY(BlueprintReadOnly, Category = "Item Use")
	bool bUsed = false;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item Use")
	bool CanAcceptItem(const FInventoryItem& Item) const;
	virtual bool CanAcceptItem_Implementation(const FInventoryItem& Item) const;

	UFUNCTION(BlueprintCallable, Category = "Item Use")
	void ResetPart();

	bool TryUseItem(const FInventoryItem& Item, AActor* User, bool& bOutConsume);
	void NotifyEmptyHandClick(AActor* User);
};