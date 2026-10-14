#include "InventoryComponent.h"

#include "ItemData.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	for (const TObjectPtr<UItemData>& Data : StartingItems)
	{
		AddItem(Data);
	}
}

int32 UInventoryComponent::AddItem(UItemData* Data)
{
	if (!Data) return INDEX_NONE;

	FInventoryItem Item;
	Item.InstanceId = NextInstanceId++;
	Item.Data = Data;
	Item.Values = Data->DefaultValues;
	Item.Flags = Data->DefaultFlags;
	Items.Add(Item);

	ClampSelection();
	OnItemsChanged.Broadcast();

	return Item.InstanceId;
}

bool UInventoryComponent::RemoveItem(int32 InstanceId)
{
	const int32 Index = FindItemIndex(InstanceId);
	if (Index == INDEX_NONE) return false;

	Items.RemoveAt(Index);

	if (EquippedInstanceId == InstanceId)
	{
		EquippedInstanceId = INDEX_NONE;
		OnEquippedChanged.Broadcast(EquippedInstanceId);
	}

	ClampSelection();
	OnItemsChanged.Broadcast();

	return true;
}

int32 UInventoryComponent::FindItemIndex(int32 InstanceId) const
{
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		if (Items[i].InstanceId == InstanceId) return i;
	}

	return INDEX_NONE;
}

bool UInventoryComponent::GetEquippedItem(FInventoryItem& OutItem) const
{
	const int32 Index = FindItemIndex(EquippedInstanceId);
	if (Index == INDEX_NONE) return false;

	OutItem = Items[Index];
	return true;
}

void UInventoryComponent::ClampSelection()
{
	const int32 MinIndex = bAllowEmptyHand ? INDEX_NONE : 0;
	const int32 MaxIndex = Items.Num() - 1;

	int32 NewIndex = INDEX_NONE;
	if (MaxIndex >= MinIndex)
	{
		NewIndex = FMath::Clamp(SelectedIndex, MinIndex, MaxIndex);
	}

	if (NewIndex != SelectedIndex)
	{
		SelectedIndex = NewIndex;
		OnSelectionChanged.Broadcast(SelectedIndex);
	}
}

void UInventoryComponent::BeginSelection()
{
	SelectedIndex = FindItemIndex(EquippedInstanceId);
	ClampSelection();
	OnSelectionChanged.Broadcast(SelectedIndex);
}

void UInventoryComponent::ScrollSelection(int32 Direction)
{
	const int32 MinIndex = bAllowEmptyHand ? INDEX_NONE : 0;
	const int32 MaxIndex = Items.Num() - 1;
	if (MaxIndex < MinIndex) return;

	const int32 Count = MaxIndex - MinIndex + 1;
	const int32 Step = Direction > 0 ? 1 : -1;

	int32 Offset = (SelectedIndex - MinIndex + Step) % Count;
	if (Offset < 0) Offset += Count;

	SelectedIndex = MinIndex + Offset;
	OnSelectionChanged.Broadcast(SelectedIndex);
}

void UInventoryComponent::ConfirmSelection()
{
	int32 NewEquipped = INDEX_NONE;
	if (Items.IsValidIndex(SelectedIndex))
	{
		NewEquipped = Items[SelectedIndex].InstanceId;
	}

	if (NewEquipped != EquippedInstanceId)
	{
		EquippedInstanceId = NewEquipped;
		OnEquippedChanged.Broadcast(EquippedInstanceId);
	}
}

float UInventoryComponent::GetItemValue(int32 InstanceId, FName Key, float DefaultValue) const
{
	const int32 Index = FindItemIndex(InstanceId);
	if (Index == INDEX_NONE) return DefaultValue;

	const float* Found = Items[Index].Values.Find(Key);
	return Found ? *Found : DefaultValue;
}

void UInventoryComponent::SetItemValue(int32 InstanceId, FName Key, float Value)
{
	const int32 Index = FindItemIndex(InstanceId);
	if (Index == INDEX_NONE) return;

	Items[Index].Values.Add(Key, Value);
	OnItemsChanged.Broadcast();
}

bool UInventoryComponent::HasItemFlag(int32 InstanceId, FName Flag) const
{
	const int32 Index = FindItemIndex(InstanceId);
	return Index != INDEX_NONE && Items[Index].Flags.Contains(Flag);
}

void UInventoryComponent::SetItemFlag(int32 InstanceId, FName Flag, bool bEnabled)
{
	const int32 Index = FindItemIndex(InstanceId);
	if (Index == INDEX_NONE) return;

	if (bEnabled)
	{
		Items[Index].Flags.AddUnique(Flag);
	}
	else
	{
		Items[Index].Flags.Remove(Flag);
	}

	OnItemsChanged.Broadcast();
}