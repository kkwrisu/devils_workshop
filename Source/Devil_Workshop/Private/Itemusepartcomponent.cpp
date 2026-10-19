#include "ItemUsePartComponent.h"

#include "ItemData.h"
#include "Engine/EngineTypes.h"

UItemUsePartComponent::UItemUsePartComponent()
{
	SetBoxExtent(FVector(10.0, 10.0, 10.0));
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
}

bool UItemUsePartComponent::CanAcceptItem_Implementation(const FInventoryItem& Item) const
{
	if (bSingleUse && bUsed) return false;
	if (!Item.Data) return false;
	if (RequiredItemId != NAME_None && Item.Data->ItemId != RequiredItemId) return false;
	if (RequiredItemTag != NAME_None && !Item.Data->HasTag(RequiredItemTag)) return false;

	return true;
}

bool UItemUsePartComponent::TryUseItem(const FInventoryItem& Item, AActor* User, bool& bOutConsume)
{
	bOutConsume = false;

	if (!CanAcceptItem(Item))
	{
		OnItemRejected.Broadcast(Item, User);
		return false;
	}

	bOutConsume = bConsumeItem;

	if (bSingleUse)
	{
		bUsed = true;
		SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	OnItemUsed.Broadcast(Item, User);
	return true;
}

void UItemUsePartComponent::NotifyEmptyHandClick(AActor* User)
{
	OnEmptyHand.Broadcast(User);
}

void UItemUsePartComponent::ResetPart()
{
	bUsed = false;
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}