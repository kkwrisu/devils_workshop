#include "ItemPickup.h"

#include "Components/StaticMeshComponent.h"
#include "InventoryComponent.h"
#include "ItemData.h"

AItemPickup::AItemPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
}

bool AItemPickup::Collect(UInventoryComponent* Inventory)
{
	if (!Inventory || !ItemData) return false;

	if (Inventory->AddItem(ItemData) == INDEX_NONE) return false;

	OnCollected();

	if (bDestroyOnCollect)
	{
		Destroy();
	}
	else
	{
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
	}

	return true;
}