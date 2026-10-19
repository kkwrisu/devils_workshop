#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryWidget.generated.h"

class UInventoryComponent;

UCLASS(Blueprintable)
class UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Inventario")
	TObjectPtr<UInventoryComponent> Inventory;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventario")
	bool bKeepVisibleWhenClosed = false;

	UFUNCTION(BlueprintCallable, Category = "Inventario")
	void BindInventory(UInventoryComponent* InInventory);

	UFUNCTION(BlueprintCallable, Category = "Inventario")
	void Open();

	UFUNCTION(BlueprintCallable, Category = "Inventario")
	void Close();

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventario")
	void OnInventoryOpened();

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventario")
	void OnInventoryClosed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventario")
	void OnItemsRefreshed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventario")
	void OnSelectionUpdated(int32 SelectedIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventario")
	void OnEquippedUpdated(int32 EquippedInstanceId);

protected:
	UFUNCTION()
	void HandleItemsChanged();

	UFUNCTION()
	void HandleSelectionChanged(int32 SelectedIndex);

	UFUNCTION()
	void HandleEquippedChanged(int32 EquippedInstanceId);
};