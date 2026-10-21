#include "RepairPart.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "InventoryTypes.h"
#include "ItemData.h"
#include "PlayerBase.h"

ARepairPart::ARepairPart()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	SetRootComponent(Root);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

void ARepairPart::BeginPlay()
{
	Super::BeginPlay();

	InitialLocation = Mesh->GetRelativeLocation();
	InitialRotation = Mesh->GetRelativeRotation().Quaternion();
}

void ARepairPart::SetInteractable(bool bInteractable)
{
	const bool bEnable = bInteractable && State == ERepairPartState::Fixed;
	Mesh->SetCollisionEnabled(bEnable ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);

	if (!bEnable && Worker.IsValid())
	{
		StopWork(false);
	}
}

bool ARepairPart::CanUseItem_Implementation(const FInventoryItem& Item)
{
	if (State != ERepairPartState::Fixed || Worker.IsValid()) return false;
	if (!Item.Data || !Item.Data->HasTag(RequiredToolTag)) return false;

	for (const TObjectPtr<ARepairPart>& Required : RequiredRemoved)
	{
		if (Required && Required->State != ERepairPartState::Removed) return false;
	}

	return true;
}

bool ARepairPart::UseItem_Implementation(const FInventoryItem& Item, AActor* User)
{
	APlayerBase* Player = Cast<APlayerBase>(User);
	if (!Player) return false;

	Worker = Player;
	Progress = 0.0f;
	Player->SetWorkingOnPart(true);
	SetActorTickEnabled(true);

	// false = a ferramenta nao e consumida
	return false;
}

void ARepairPart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	APlayerBase* Player = Worker.Get();
	if (!Player || !Player->IsClickHeld())
	{
		StopWork(false);
		return;
	}

	Progress = FMath::Min(1.0f, Progress + DeltaTime / WorkDuration);
	ApplyProgressVisual();

	if (Progress >= 1.0f)
	{
		StopWork(true);
	}
}

void ARepairPart::StopWork(bool bCompleted)
{
	if (APlayerBase* Player = Worker.Get())
	{
		Player->SetWorkingOnPart(false);
	}

	Worker.Reset();
	SetActorTickEnabled(false);

	if (bCompleted)
	{
		State = ERepairPartState::Removed;
		Mesh->SetVisibility(false);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		OnStateChanged.Broadcast(this, State);
	}
	else
	{
		Progress = 0.0f;
		ApplyProgressVisual();
	}
}

void ARepairPart::ApplyProgressVisual()
{
	const FVector Axis = TurnAxis.GetSafeNormal();
	const FQuat Turn(Axis, FMath::DegreesToRadians(TurnDegrees * Progress));

	Mesh->SetRelativeRotation(InitialRotation * Turn);
	Mesh->SetRelativeLocation(InitialLocation + InitialRotation.RotateVector(Axis) * LiftDistance * Progress);
}