#include "BatteryCharger.h"

#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "InventoryComponent.h"
#include "ItemData.h"
#include "PlayerBase.h"
#include "Monster/MonsterBase.h"
#include "Perception/AISense_Hearing.h"
#include "Engine/TargetPoint.h"

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

void ABatteryCharger::BeginPlay()
{
	Super::BeginPlay();

	// O mesh pode ficar visivel como guia no editor do Blueprint para posicionar a bateria.
	// Em runtime, ela so aparece quando uma bateria e inserida.
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
	NextChargingNoiseTime = FMath::Max(ChargingNoiseInterval, 0.1f);
	ChargeInstigator = User;
	EncounterPlayer = Cast<APlayerBase>(User);
	bMonsterSpawnedForCurrentBattery = false;
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
	if (APlayerBase* Player = EncounterPlayer.Get(); Player && Player->IsInCorridor())
	{
		GetWorldTimerManager().SetTimer(
			MonsterSpawnTimerHandle, this, &ABatteryCharger::SpawnCorridorMonster,
			FMath::Max(MonsterSpawnDelay, 0.1f), false);
		UE_LOG(LogTemp, Warning, TEXT("BatteryCharger '%s': monstro agendado para surgir em %.1f s"), *GetName(), MonsterSpawnDelay);
	}
	ReportHearingNoise(FName("BatteryCharging"), ChargingNoiseLoudness, ChargingNoiseRange);

	UE_LOG(LogTemp, Warning, TEXT("Carregador '%s': carregando '%s' por %.1f s"), *GetName(), *Item.Data->GetName(), ChargeDuration);

	// true = o jogador perde a bateria do inventario (ela fica dentro do carregador)
	return true;
}

void ABatteryCharger::HandlePlayerCorridorChanged(APlayerBase* Player, bool bEnteredCorridor)
{
	if (!Player || Player != EncounterPlayer.Get()) return;

	if (!bEnteredCorridor)
	{
		GetWorldTimerManager().ClearTimer(MonsterSpawnTimerHandle);
		if (AMonsterBase* Monster = CorridorMonster.Get())
		{
			Monster->Destroy();
			UE_LOG(LogTemp, Warning, TEXT("BatteryCharger '%s': jogador saiu do corredor; perseguição encerrada"), *GetName());
		}
		CorridorMonster.Reset();
		return;
	}

	// Se ainda estiver carregando e o jogador retornar antes do surgimento,
	// inicia uma nova janela de 15 segundos dentro do corredor.
	if (State == EChargerState::Charging && !bMonsterSpawnedForCurrentBattery && !GetWorldTimerManager().IsTimerActive(MonsterSpawnTimerHandle))
	{
		GetWorldTimerManager().SetTimer(
			MonsterSpawnTimerHandle, this, &ABatteryCharger::SpawnCorridorMonster,
			FMath::Max(MonsterSpawnDelay, 0.1f), false);
	}
}

void ABatteryCharger::SpawnCorridorMonster()
{
	APlayerBase* Player = EncounterPlayer.Get();
	if (State != EChargerState::Charging || !IsValid(Player) || !Player->IsInCorridor())
	{
		UE_LOG(LogTemp, Warning, TEXT("BatteryCharger '%s': surgimento cancelado; carregamento/player/corredor inválido"), *GetName());
		return;
	}
	if (!CorridorMonsterClass)
	{
		UE_LOG(LogTemp, Error, TEXT("BatteryCharger '%s': CorridorMonsterClass não foi configurado"), *GetName());
		return;
	}

	TArray<AActor*> SpawnPoints;
	UGameplayStatics::GetAllActorsWithTag(this, MonsterSpawnPointTag, SpawnPoints);
	if (SpawnPoints.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("BatteryCharger '%s': nenhum ponto com tag '%s' na fase"), *GetName(), *MonsterSpawnPointTag.ToString());
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AMonsterBase* SpawnedMonster = GetWorld()->SpawnActor<AMonsterBase>(
		CorridorMonsterClass, SpawnPoints[0]->GetActorTransform(), SpawnParams);
	if (!SpawnedMonster)
	{
		UE_LOG(LogTemp, Error, TEXT("BatteryCharger '%s': falha ao criar monstro"), *GetName());
		return;
	}

	CorridorMonster = SpawnedMonster;
	bMonsterSpawnedForCurrentBattery = true;
	SpawnedMonster->StartCorridorChase(Player);
	UE_LOG(LogTemp, Warning, TEXT("BatteryCharger '%s': monstro '%s' surgiu no corredor"), *GetName(), *SpawnedMonster->GetName());
}

void ABatteryCharger::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(MonsterSpawnTimerHandle);
	if (AMonsterBase* Monster = CorridorMonster.Get())
	{
		Monster->Destroy();
	}
	Super::EndPlay(EndPlayReason);
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
	if (ChargeElapsed >= NextChargingNoiseTime)
	{
		ReportHearingNoise(FName("BatteryCharging"), ChargingNoiseLoudness, ChargingNoiseRange);
		NextChargingNoiseTime += FMath::Max(ChargingNoiseInterval, 0.1f);
	}

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
	ReportHearingNoise(FName("BatteryCharged"), ChargeCompleteNoiseLoudness, ChargeCompleteNoiseRange);

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
	ChargeInstigator.Reset();
	BatteryMesh->SetVisibility(false);
	State = EChargerState::Empty;

	UE_LOG(LogTemp, Warning, TEXT("Carregador '%s': bateria carregada devolvida ao jogador"), *GetName());
	return true;
}

void ABatteryCharger::ReportHearingNoise(FName EventTag, float Loudness, float MaxRange)
{
	if (!GetWorld()) return;

	AActor* InstigatorActor = ChargeInstigator.Get();
	UAISense_Hearing::ReportNoiseEvent(
		this,
		GetActorLocation(),
		Loudness,
		InstigatorActor ? InstigatorActor : this,
		MaxRange,
		EventTag);
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
