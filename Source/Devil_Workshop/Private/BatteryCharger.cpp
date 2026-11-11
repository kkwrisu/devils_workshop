#include "BatteryCharger.h"

#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "InventoryComponent.h"
#include "ItemData.h"
#include "PlayerBase.h"

ABatteryCharger::ABatteryCharger()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	// So responde ao clique do mouse (trace de Visibility)
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	BatteryMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BatteryMesh"));
	BatteryMesh->SetupAttachment(Mesh);
	BatteryMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BatteryMesh->SetVisibility(false);
}

bool ABatteryCharger::CanUseItem_Implementation(const FInventoryItem& Item)
{
	if (State != EChargerState::Empty) return false;
	if (!Item.Data || !Item.Data->HasTag(RequiredItemTag)) return false;

	// Bateria que ja esta carregada nao precisa voltar ao carregador
	if (Item.Flags.Contains(ChargedFlag)) return false;

	return true;
}

bool ABatteryCharger::UseItem_Implementation(const FInventoryItem& Item, AActor* User)
{
	if (State != EChargerState::Empty || !Item.Data) return false;

	StoredItemData = Item.Data;
	ChargeElapsed = 0.0f;
	State = EChargerState::Charging;

	if (UStaticMesh* Visual = Item.Data->InspectMesh)
	{
		BatteryMesh->SetStaticMesh(Visual);
		BatteryMesh->SetVisibility(true);
	}

	if (InsertSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, InsertSound, GetActorLocation());
	}

	if (ChargingLoopSound)
	{
		LoopAudio = UGameplayStatics::SpawnSoundAttached(
			ChargingLoopSound, GetRootComponent(), NAME_None, FVector::ZeroVector,
			EAttachLocation::KeepRelativeOffset, true);
	}

	SetActorTickEnabled(true);
	OnChargeStarted.Broadcast();

	UE_LOG(LogTemp, Warning, TEXT("Carregador '%s': carregando '%s' por %.1f s"), *GetName(), *Item.Data->GetName(), ChargeDuration);

	// true = o jogador perde a bateria do inventario (ela fica dentro do carregador)
	return true;
}

void ABatteryCharger::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (State != EChargerState::Charging)
	{
		SetActorTickEnabled(false);
		return;
	}

	ChargeElapsed += DeltaTime;
	if (ChargeElapsed >= ChargeDuration)
	{
		FinishCharging();
	}
}

void ABatteryCharger::FinishCharging()
{
	State = EChargerState::Done;
	SetActorTickEnabled(false);

	if (LoopAudio)
	{
		LoopAudio->Stop();
		LoopAudio = nullptr;
	}

	if (ChargeCompleteSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ChargeCompleteSound, GetActorLocation());
	}

	OnChargeCompleted.Broadcast();

	UE_LOG(LogTemp, Warning, TEXT("Carregador '%s': carga completa"), *GetName());
}

bool ABatteryCharger::TryCollect(APlayerBase* Player)
{
	if (State != EChargerState::Done || !StoredItemData || !Player || !Player->Inventory) return false;

	const int32 NewId = Player->Inventory->AddItem(StoredItemData);
	if (NewId == INDEX_NONE) return false;

	Player->Inventory->SetItemFlag(NewId, ChargedFlag, true);

	StoredItemData = nullptr;
	BatteryMesh->SetVisibility(false);
	State = EChargerState::Empty;

	UE_LOG(LogTemp, Warning, TEXT("Carregador '%s': bateria carregada devolvida ao jogador"), *GetName());
	return true;
}

float ABatteryCharger::GetChargeProgress() const
{
	switch (State)
	{
	case EChargerState::Charging:
		return FMath::Clamp(ChargeElapsed / ChargeDuration, 0.0f, 1.0f);
	case EChargerState::Done:
		return 1.0f;
	default:
		return 0.0f;
	}
}
