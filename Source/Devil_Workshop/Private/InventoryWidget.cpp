#include "InventoryWidget.h"

#include "InventoryComponent.h"

void UInventoryWidget::BindInventory(UInventoryComponent* InInventory)
{
	if (Inventory)
	{
		Inventory->OnItemsChanged.RemoveDynamic(this, &UInventoryWidget::HandleItemsChanged);
		Inventory->OnSelectionChanged.RemoveDynamic(this, &UInventoryWidget::HandleSelectionChanged);
		Inventory->OnEquippedChanged.RemoveDynamic(this, &UInventoryWidget::HandleEquippedChanged);
	}

	Inventory = InInventory;

	if (Inventory)
	{
		Inventory->OnItemsChanged.AddDynamic(this, &UInventoryWidget::HandleItemsChanged);
		Inventory->OnSelectionChanged.AddDynamic(this, &UInventoryWidget::HandleSelectionChanged);
		Inventory->OnEquippedChanged.AddDynamic(this, &UInventoryWidget::HandleEquippedChanged);
	}

	OnItemsRefreshed();
	OnEquippedUpdated(Inventory ? Inventory->GetEquippedInstanceId() : INDEX_NONE);
}

void UInventoryWidget::Open()
{
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	OnInventoryOpened();
	OnItemsRefreshed();

	if (Inventory)
	{
		OnSelectionUpdated(Inventory->GetSelectedIndex());
	}
}

void UInventoryWidget::Close()
{
	SetVisibility(bKeepVisibleWhenClosed ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	OnInventoryClosed();
}

void UInventoryWidget::HandleItemsChanged()
{
	OnItemsRefreshed();
}

void UInventoryWidget::HandleSelectionChanged(int32 SelectedIndex)
{
	OnSelectionUpdated(SelectedIndex);
}

void UInventoryWidget::HandleEquippedChanged(int32 EquippedInstanceId)
{
	OnEquippedUpdated(EquippedInstanceId);
}