#include "ItemPickup.h"

#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "InventoryComponent.h"
#include "ItemData.h"
#include "PlayerBase.h"

AItemPickup::AItemPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	// So responde ao clique do mouse (trace de Visibility)
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

bool AItemPickup::TryPickup(APlayerBase* Player)
{
	if (!Player || !ItemData || !Player->Inventory)
	{
		UE_LOG(LogTemp, Warning, TEXT("ItemPickup '%s': nao deu para pegar (Item Data vazio ou jogador sem inventario)"), *GetName());
		return false;
	}

	const int32 NewId = Player->Inventory->AddItem(ItemData);
	if (NewId == INDEX_NONE) return false;

	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
	}

	UE_LOG(LogTemp, Warning, TEXT("ItemPickup '%s': item '%s' foi para o inventario"), *GetName(), *ItemData->GetName());
	Destroy();
	return true;
}
