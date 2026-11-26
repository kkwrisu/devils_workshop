#include "WorkshopEventDirector.h"

#include "Camera/CameraComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerBase.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "WorkshopFlickerComponent.h"

AWorkshopEventDirector::AWorkshopEventDirector()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.15f;
}

void AWorkshopEventDirector::BeginPlay()
{
	Super::BeginPlay();
	GetWorldTimerManager().SetTimer(InitializationTimerHandle, this, &AWorkshopEventDirector::InitializeDirector, 0.2f, false);
}

void AWorkshopEventDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bInitialized)
	{
		UpdateWindowState();
		if (bEnableRandomEvents && IsValid(Player) && Player->CanRunWorkshopAmbientEvents())
		{
			ActiveGameplayElapsed += DeltaSeconds;
			if (ActiveGameplayElapsed >= NextEventAtActiveTime)
			{
				TryRunRandomEvent();
				ScheduleNextEvent(false);
			}
		}
	}
}

void AWorkshopEventDirector::InitializeDirector()
{
	Player = Cast<APlayerBase>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Player)
	{
		GetWorldTimerManager().SetTimer(InitializationTimerHandle, this, &AWorkshopEventDirector::InitializeDirector, 0.5f, false);
		return;
	}

	FindLevelReferences();
	DiscoverFlickerComponents();
	LastStableArea = Player->CurrentAreaActor;
	bInitialized = true;

	if (WindowHumanoid)
	{
		WindowHumanoid->SetActorEnableCollision(false);
		WindowHumanoid->SetActorHiddenInGame(true);
	}

	if (bEnableRandomEvents)
	{
		ScheduleNextEvent(true);
	}

	UE_LOG(LogTemp, Log, TEXT("WorkshopEvents: inicializado | Player=%s | WindowArea=%s | StartArea=%s | Humanoid=%s | FlickerLights=%d"),
		*GetNameSafe(Player), *GetNameSafe(WindowArea), *GetNameSafe(WorkshopStartArea), *GetNameSafe(WindowHumanoid), FlickerLights.Num());
}

void AWorkshopEventDirector::FindLevelReferences()
{
	if (!WindowArea)
	{
		WindowArea = FindActorWithTag(TEXT("WorkshopWindowPosition"));
		if (!WindowArea) WindowArea = FindActorNamed(TEXT("Pos_Janela"));
	}
	if (!WorkshopStartArea)
	{
		WorkshopStartArea = FindActorWithTag(TEXT("WorkshopStartPosition"));
	}
	if (!WindowHumanoid)
	{
		WindowHumanoid = FindActorWithTag(TEXT("WindowHumanoid"));
	}
	if (!EntranceKnockPoint)
	{
		EntranceKnockPoint = FindActorWithTag(TEXT("WorkshopEntrance"));
	}
}

void AWorkshopEventDirector::DiscoverFlickerComponents()
{
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(this, AActor::StaticClass(), Actors);
	for (AActor* Actor : Actors)
	{
		if (!IsValid(Actor)) continue;
		UWorkshopFlickerComponent* Flicker = Actor->FindComponentByClass<UWorkshopFlickerComponent>();
		if (Flicker)
		{
			FlickerLights.AddUnique(Flicker);
		}
	}
}

AActor* AWorkshopEventDirector::FindActorWithTag(FName Tag) const
{
	if (Tag.IsNone() || !GetWorld()) return nullptr;
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsWithTag(this, Tag, Actors);
	return Actors.IsEmpty() ? nullptr : Actors[0];
}

AActor* AWorkshopEventDirector::FindActorNamed(const FString& NameFragment) const
{
	if (NameFragment.IsEmpty() || !GetWorld()) return nullptr;
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(this, AActor::StaticClass(), Actors);
	for (AActor* Actor : Actors)
	{
		if (IsValid(Actor) && Actor->GetName().Contains(NameFragment, ESearchCase::IgnoreCase))
		{
			return Actor;
		}
	}
	return nullptr;
}

void AWorkshopEventDirector::ScheduleNextEvent(bool bFirstEvent)
{
	if (!bEnableRandomEvents) return;

	const float MinDelay = bFirstEvent ? FirstEventDelayMin : EventIntervalMin;
	const float MaxDelay = bFirstEvent ? FirstEventDelayMax : EventIntervalMax;
	const float Delay = FMath::FRandRange(FMath::Min(MinDelay, MaxDelay), FMath::Max(MinDelay, MaxDelay));
	NextEventAtActiveTime = ActiveGameplayElapsed + Delay;
}

void AWorkshopEventDirector::TryRunRandomEvent()
{
	if (!bEnableRandomEvents)
	{
		return;
	}
	if (!IsValid(Player) || !Player->CanRunWorkshopAmbientEvents()) return;

	TArray<int32> ValidAmbientIndices;
	for (int32 Index = 0; Index < AmbientSounds.Num(); ++Index)
	{
		const FWorkshopAmbientSoundEvent& Entry = AmbientSounds[Index];
		if (Entry.Sound && Entry.SoundPoint && Entry.Weight > 0.0f
			&& !IsPointInPlayerView(Entry.SoundPoint->GetActorLocation(), Entry.SoundPoint))
		{
			ValidAmbientIndices.Add(Index);
		}
	}

	TArray<TObjectPtr<UWorkshopFlickerComponent>> ValidFlickerLights;
	for (UWorkshopFlickerComponent* Flicker : FlickerLights)
	{
		if (IsValid(Flicker) && Flicker->CanBeTriggeredByRandomEvent() && !Flicker->IsFlickering())
		{
			ValidFlickerLights.Add(Flicker);
		}
	}

	TArray<ERandomWorkshopEvent> Options;
	TArray<float> Weights;
	if (!ValidAmbientIndices.IsEmpty() && AmbientSoundWeight > 0.0f)
	{
		Options.Add(ERandomWorkshopEvent::AmbientSound);
		Weights.Add(AmbientSoundWeight);
	}
	if (EntranceKnockPoint && EntranceKnockSound && DoorKnockWeight > 0.0f)
	{
		Options.Add(ERandomWorkshopEvent::EntranceKnock);
		Weights.Add(DoorKnockWeight);
	}
	if (!ValidFlickerLights.IsEmpty() && LightFlickerWeight > 0.0f)
	{
		Options.Add(ERandomWorkshopEvent::LightFlicker);
		Weights.Add(LightFlickerWeight);
	}

	float TotalWeight = 0.0f;
	for (float Weight : Weights) TotalWeight += Weight;
	if (TotalWeight <= 0.0f)
	{
		ScheduleNextEvent(false);
		return;
	}

	float Roll = FMath::FRandRange(0.0f, TotalWeight);
	int32 ChosenIndex = 0;
	for (; ChosenIndex < Weights.Num() - 1; ++ChosenIndex)
	{
		Roll -= Weights[ChosenIndex];
		if (Roll <= 0.0f) break;
	}

	switch (Options[ChosenIndex])
	{
	case ERandomWorkshopEvent::AmbientSound:
		RunAmbientSoundEvent();
		break;
	case ERandomWorkshopEvent::EntranceKnock:
		RunEntranceKnockEvent();
		break;
	case ERandomWorkshopEvent::LightFlicker:
		RunLightFlickerEvent();
		break;
	default:
		break;
	}

	ScheduleNextEvent(false);
}

void AWorkshopEventDirector::RunAmbientSoundEvent()
{
	TArray<int32> Candidates;
	float TotalWeight = 0.0f;
	for (int32 Index = 0; Index < AmbientSounds.Num(); ++Index)
	{
		const FWorkshopAmbientSoundEvent& Entry = AmbientSounds[Index];
		if (Entry.Sound && Entry.SoundPoint && Entry.Weight > 0.0f
			&& !IsPointInPlayerView(Entry.SoundPoint->GetActorLocation(), Entry.SoundPoint))
		{
			Candidates.Add(Index);
			TotalWeight += Entry.Weight;
		}
	}
	if (Candidates.IsEmpty() || TotalWeight <= 0.0f) return;

	float Roll = FMath::FRandRange(0.0f, TotalWeight);
	int32 ChosenIndex = Candidates.Last();
	for (int32 CandidateIndex : Candidates)
	{
		Roll -= AmbientSounds[CandidateIndex].Weight;
		if (Roll <= 0.0f)
		{
			ChosenIndex = CandidateIndex;
			break;
		}
	}

	const FWorkshopAmbientSoundEvent& Entry = AmbientSounds[ChosenIndex];
	UGameplayStatics::PlaySoundAtLocation(this, Entry.Sound, Entry.SoundPoint->GetActorLocation(), 1.0f, 1.0f, 0.0f, AmbientSoundAttenuation);
	UE_LOG(LogTemp, Log, TEXT("WorkshopEvents: som ambiente '%s' em '%s'"), *GetNameSafe(Entry.Sound), *GetNameSafe(Entry.SoundPoint));
}

void AWorkshopEventDirector::RunEntranceKnockEvent()
{
	if (!EntranceKnockPoint || !EntranceKnockSound) return;
	UGameplayStatics::PlaySoundAtLocation(this, EntranceKnockSound, EntranceKnockPoint->GetActorLocation(), 1.0f, 1.0f, 0.0f, EntranceKnockAttenuation);
	UE_LOG(LogTemp, Log, TEXT("WorkshopEvents: batida na entrada em '%s'"), *GetNameSafe(EntranceKnockPoint));
}

void AWorkshopEventDirector::RunLightFlickerEvent()
{
	TArray<UWorkshopFlickerComponent*> Candidates;
	for (UWorkshopFlickerComponent* Flicker : FlickerLights)
	{
		if (IsValid(Flicker) && Flicker->CanBeTriggeredByRandomEvent() && !Flicker->IsFlickering())
		{
			Candidates.Add(Flicker);
		}
	}
	if (Candidates.IsEmpty()) return;

	const int32 Count = FMath::RandRange(1, FMath::Min(2, Candidates.Num()));
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const int32 ChosenIndex = FMath::RandRange(0, Candidates.Num() - 1);
		UWorkshopFlickerComponent* Chosen = Candidates[ChosenIndex];
		Candidates.RemoveAtSwap(ChosenIndex);
		Chosen->StartFlicker(FMath::FRandRange(EventFlickerDurationMin, FMath::Max(EventFlickerDurationMin, EventFlickerDurationMax)));
	}
	UE_LOG(LogTemp, Log, TEXT("WorkshopEvents: evento de luzes piscando"));
}

bool AWorkshopEventDirector::IsPointInPlayerView(const FVector& Point, AActor* PointActor) const
{
	if (!IsValid(Player) || !Player->Camera) return false;

	const FVector CameraLocation = Player->Camera->GetComponentLocation();
	const FVector ToPoint = Point - CameraLocation;
	if (FVector::DotProduct(Player->Camera->GetForwardVector(), ToPoint.GetSafeNormal()) <= 0.0f)
	{
		return false;
	}

	APlayerController* PlayerController = Cast<APlayerController>(Player->GetController());
	if (!PlayerController) return false;

	FVector2D ScreenPosition;
	if (!PlayerController->ProjectWorldLocationToScreen(Point, ScreenPosition)) return false;

	int32 ViewportX = 0;
	int32 ViewportY = 0;
	PlayerController->GetViewportSize(ViewportX, ViewportY);
	if (ScreenPosition.X < 0.0f || ScreenPosition.Y < 0.0f
		|| ScreenPosition.X > ViewportX || ScreenPosition.Y > ViewportY)
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WorkshopEventVisibility), true);
	QueryParams.AddIgnoredActor(Player);
	if (PointActor) QueryParams.AddIgnoredActor(PointActor);
	FHitResult Hit;
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, CameraLocation, Point, ECC_Visibility, QueryParams);
	return !bBlocked;
}

void AWorkshopEventDirector::UpdateWindowState()
{
	if (!IsValid(Player) || Player->IsTransitioning) return;
	AActor* StableArea = Player->CurrentAreaActor;
	if (!IsValid(StableArea) || StableArea == LastStableArea) return;

	LastStableArea = StableArea;
	if (StableArea == WindowArea || StableArea->GetName().Contains(TEXT("Pos_Janela"), ESearchCase::IgnoreCase))
	{
		bVisitedWindowSinceStart = true;
		if (!bWindowHumanoidPresent && !bWindowAppearanceRolledThisCycle)
		{
			bWindowAppearanceRolledThisCycle = true;
			SetWindowHumanoidPresent(FMath::FRand() <= WindowAppearanceChance);
		}
		return;
	}

	const bool bIsStartArea = StableArea == WorkshopStartArea || StableArea->ActorHasTag(TEXT("WorkshopStartPosition"));
	if (bIsStartArea && bVisitedWindowSinceStart)
	{
		if (bWindowHumanoidPresent)
		{
			SetWindowHumanoidPresent(FMath::FRand() <= WindowStayChanceAfterReturn);
		}
		bVisitedWindowSinceStart = false;
		bWindowAppearanceRolledThisCycle = false;
		UE_LOG(LogTemp, Log, TEXT("WorkshopEvents: retorno ao ponto inicial; humanoide presente=%d"), bWindowHumanoidPresent ? 1 : 0);
	}
}

void AWorkshopEventDirector::SetWindowHumanoidPresent(bool bPresent)
{
	if (!WindowHumanoid)
	{
		UE_LOG(LogTemp, Warning, TEXT("WorkshopEvents: configure um ator com a tag WindowHumanoid para a aparicao da janela"));
		bWindowHumanoidPresent = false;
		return;
	}

	bWindowHumanoidPresent = bPresent;
	WindowHumanoid->SetActorEnableCollision(false);
	WindowHumanoid->SetActorHiddenInGame(!bPresent);
	UE_LOG(LogTemp, Log, TEXT("WorkshopEvents: humanoide da janela %s"), bPresent ? TEXT("permanece visivel") : TEXT("foi ocultado"));
}

void AWorkshopEventDirector::StartManualLightFlicker(int32 LightIndex, float Duration)
{
	if (FlickerLights.IsValidIndex(LightIndex) && IsValid(FlickerLights[LightIndex]))
	{
		FlickerLights[LightIndex]->StartFlicker(Duration);
	}
}

void AWorkshopEventDirector::StopManualLightFlicker(int32 LightIndex)
{
	if (FlickerLights.IsValidIndex(LightIndex) && IsValid(FlickerLights[LightIndex]))
	{
		FlickerLights[LightIndex]->StopFlicker();
	}
}
