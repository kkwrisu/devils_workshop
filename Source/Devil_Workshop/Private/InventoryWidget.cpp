#include "InventoryWidget.h"

#include "InventoryComponent.h"

void UInventoryWidget::BindInventory(UInventoryComponent* InInventory)
{
	if (Inventory)
	{
		Inventory->OnItemsChanged.RemoveDynamic(this, &UInventoryWidget::HandleItemsChanged);
		Inventory->OnSelectionChanged.RemoveDynamic(this, &UInventoryWidget::HandleSelectionChanged);
	}

	Inventory = InInventory;

	if (Inventory)
	{
		Inventory->OnItemsChanged.AddDynamic(this, &UInventoryWidget::HandleItemsChanged);
		Inventory->OnSelectionChanged.AddDynamic(this, &UInventoryWidget::HandleSelectionChanged);
	}

	OnItemsRefreshed();
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
	SetVisibility(ESlateVisibility::Collapsed);
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