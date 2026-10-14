#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryTypes.h"
#include "InventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryItemsChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventorySelectionChanged, int32, SelectedIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryEquippedChanged, int32, EquippedInstanceId);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	UPROPERTY(BlueprintAssignable, Category = "Inventario")
	FOnInventoryItemsChanged OnItemsChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventario")
	FOnInventorySelectionChanged OnSelectionChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventario")
	FOnInventoryEquippedChanged OnEquippedChanged;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventario")
	bool bAllowEmptyHand = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventario")
	TArray<TObjectPtr<UItemData>> StartingItems;

	UFUNCTION(BlueprintCallable, Category = "Inventario")
	int32 AddItem(UItemData* Data);

	UFUNCTION(BlueprintCallable, Category = "Inventario")
	bool RemoveItem(int32 InstanceId);

	UFUNCTION(BlueprintPure, Category = "Inventario")
	int32 FindItemIndex(int32 InstanceId) const;

	UFUNCTION(BlueprintPure, Category = "Inventario")
	const TArray<FInventoryItem>& GetItems() const { return Items; }

	UFUNCTION(BlueprintPure, Category = "Inventario")
	int32 GetSelectedIndex() const { return SelectedIndex; }

	UFUNCTION(BlueprintPure, Category = "Inventario")
	int32 GetEquippedInstanceId() const { return EquippedInstanceId; }

	UFUNCTION(BlueprintPure, Category = "Inventario")
	bool HasEquippedItem() const { return FindItemIndex(EquippedInstanceId) != INDEX_NONE; }

	UFUNCTION(BlueprintCallable, Category = "Inventario")
	bool GetEquippedItem(FInventoryItem& OutItem) const;

	UFUNCTION(BlueprintCallable, Category = "Inventario")
	void BeginSelection();

	UFUNCTION(BlueprintCallable, Category = "Inventario")
	void ScrollSelection(int32 Direction);

	UFUNCTION(BlueprintCallable, Category = "Inventario")
	void ConfirmSelection();

	UFUNCTION(BlueprintPure, Category = "Inventario")
	float GetItemValue(int32 InstanceId, FName Key, float DefaultValue) const;

	UFUNCTION(BlueprintCallable, Category = "Inventario")
	void SetItemValue(int32 InstanceId, FName Key, float Value);

	UFUNCTION(BlueprintPure, Category = "Inventario")
	bool HasItemFlag(int32 InstanceId, FName Flag) const;

	UFUNCTION(BlueprintCallable, Category = "Inventario")
	void SetItemFlag(int32 InstanceId, FName Flag, bool bEnabled);

protected:
	virtual void BeginPlay() override;

	void ClampSelection();

	UPROPERTY()
	TArray<FInventoryItem> Items;

	int32 SelectedIndex = INDEX_NONE;
	int32 EquippedInstanceId = INDEX_NONE;
	int32 NextInstanceId = 1;
};