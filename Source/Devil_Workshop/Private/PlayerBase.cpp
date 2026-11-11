#include "PlayerBase.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/ShapeComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Components/Widget.h"
#include "CollisionQueryParams.h"
#include "Curves/CurveFloat.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/GameUserSettings.h"
#include "InventoryComponent.h"
#include "InventoryWidget.h"
#include "ItemUseTarget.h"
#include "ItemPickup.h"
#include "BatteryCharger.h"
#include "RepairPart.h"
#include "RepairableComponent.h"
#include "RepairProgressWidget.h"
#include "RepairStep.h"
#include "Engine/EngineTypes.h"
#include "Engine/Scene.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Monsters/MonsterTypes.h"

static UCurveFloat* MakeDefaultCurve(UObject* Outer, float EndTime, float EndValue, bool bSmooth)
{
	UCurveFloat* NewCurve = NewObject<UCurveFloat>(Outer);

	const FKeyHandle StartKey = NewCurve->FloatCurve.AddKey(0.f, 0.f);
	const FKeyHandle EndKey = NewCurve->FloatCurve.AddKey(EndTime, EndValue);

	if (bSmooth)
	{
		NewCurve->FloatCurve.SetKeyInterpMode(StartKey, RCIM_Cubic);
		NewCurve->FloatCurve.SetKeyInterpMode(EndKey, RCIM_Cubic);
		NewCurve->FloatCurve.SetKeyTangentMode(StartKey, RCTM_Auto);
		NewCurve->FloatCurve.SetKeyTangentMode(EndKey, RCTM_Auto);
		NewCurve->FloatCurve.AutoSetTangents();
	}

	return NewCurve;
}

APlayerBase::APlayerBase()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Root);
	Camera->SetRelativeLocation(FVector::ZeroVector);
	Camera->SetRelativeRotation(FRotator::ZeroRotator);
	Camera->FieldOfView = 90.f;
	Camera->bUsePawnControlRotation = false;

	InspectionAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("InspectionAnchor"));
	InspectionAnchor->SetupAttachment(Camera);
	InspectionAnchor->SetRelativeLocation(FVector(120.0, 0.0, 0.0));
	InspectionAnchor->SetRelativeRotation(FRotator::ZeroRotator);

	Inventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));
}

void APlayerBase::InitTimeline(FTimeline& Timeline, TObjectPtr<UCurveFloat>& Curve, float Length,
	float DefaultEndTime, float DefaultEndValue, bool bSmooth, FName UpdateFn, FName FinishedFn)
{
	if (!Curve)
	{
		Curve = MakeDefaultCurve(this, DefaultEndTime, DefaultEndValue, bSmooth);
	}

	FOnTimelineFloat Update;
	Update.BindUFunction(this, UpdateFn);
	Timeline.AddInterpFloat(Curve, Update);

	Timeline.SetTimelineLengthMode(ETimelineLengthMode::TL_TimelineLength);
	Timeline.SetTimelineLength(Length);

	if (FinishedFn != NAME_None)
	{
		FOnTimelineEvent Finished;
		Finished.BindUFunction(this, FinishedFn);
		Timeline.SetTimelineFinishedFunc(Finished);
	}
}

void APlayerBase::EnforceMinimumQuality()
{
	if (!GEngine) return;

	UGameUserSettings* Settings = GEngine->GetGameUserSettings();
	if (!Settings) return;

	const int32 CurrentLevel = Settings->GetOverallScalabilityLevel();
	if (CurrentLevel >= MinimumScalabilityLevel) return;

	UE_LOG(LogTemp, Warning, TEXT("PlayerBase: qualidade geral = %d, abaixo do minimo %d. Ajustando."), CurrentLevel, MinimumScalabilityLevel);

	Settings->SetOverallScalabilityLevel(MinimumScalabilityLevel);
	Settings->ApplyNonResolutionSettings();
}

void APlayerBase::BeginPlay()
{
	Super::BeginPlay();

	EnforceMinimumQuality();

	DefaultCameraRelativeLocation = Camera->GetRelativeLocation();
	DefaultCameraRelativeRotation = Camera->GetRelativeRotation();

	InspectionAnchor->SetRelativeLocation(FVector(InspectionDistance, 0.0, 0.0));
	UE_LOG(LogTemp, Warning, TEXT("PlayerBase: versao com clique via Enhanced Input na inspecao carregada (margem de borda = %.0f px, blur FStop = %.2f)"), EdgeLookMargin, InspectionBlurFStop);

	InitTimeline(MoveTimeline, MoveCurve, MoveLength, 0.57f, 1.03f, true, "OnMoveUpdate", "OnMoveFinished");
	InitTimeline(CameraFocusTimeline, CameraFocusCurve, CameraFocusLength, 0.35f, 1.0f, false, "OnCameraFocusUpdate", "OnCameraFocusFinished");
	InitTimeline(InspectTimeline, InspectCurve, InspectLength, 0.5f, 1.0f, true, "OnInspectUpdate", "OnInspectFinished");

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		CachedPlayerController = PC;

		if (auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("PlayerBase: DefaultMappingContext nao foi atribuido no Class Defaults."));
			}
		}

		PC->bShowMouseCursor = true;
		UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(PC, nullptr, EMouseLockMode::LockAlways, false);
		UGameplayStatics::SetGamePaused(this, false);

		if (InventoryWidgetClass)
		{
			InventoryWidget = CreateWidget<UInventoryWidget>(PC, InventoryWidgetClass);
			if (InventoryWidget)
			{
				InventoryWidget->BindInventory(Inventory);
				InventoryWidget->AddToViewport(10);
				InventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("DEBUG: InventoryWidget É NULO!"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("DEBUG: InventoryWidgetClass NÃO SETADA no PlayerBase!"));
		}
	}
}

void APlayerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	MoveTimeline.TickTimeline(DeltaTime);
	CameraFocusTimeline.TickTimeline(DeltaTime);
	InspectTimeline.TickTimeline(DeltaTime);

	if (!CachedPlayerController) return;

	if (!IsTransitioning && !isInspecting && !bIsCameraFocused && !bInventoryOpen)
	{
		float MouseX = 0.f;
		float MouseY = 0.f;
		int32 SizeX = 0;
		int32 SizeY = 0;

		if (!CachedPlayerController->GetMousePosition(MouseX, MouseY)) return;
		CachedPlayerController->GetViewportSize(SizeX, SizeY);

		if (MouseX < EdgeLookMargin)
		{
			ApplyLookDelta(EdgeLookSpeed * -1.0 * DeltaTime);
		}
		else if (MouseX > SizeX - EdgeLookMargin)
		{
			ApplyLookDelta(EdgeLookSpeed * DeltaTime);
		}

		if (MouseY < EdgeLookMargin)
		{
			ApplyLookPitchDelta(DeltaTime * EdgeLookSpeed);
		}
		else if (MouseY > SizeY - EdgeLookMargin)
		{
			ApplyLookPitchDelta(DeltaTime * EdgeLookSpeed * -1.0);
		}
	}
	else if (isInspecting)
	{
		UpdateInspectionZoom(DeltaTime);
		UpdateInspectionRotation();

		// Atualiza progresso de reparo ativo (novo sistema data-driven)
		if (bIsActivelyRepairing && CurrentRepairableComponent)
		{
			UpdateActiveRepair(DeltaTime);
		}
	}
	else
	{
		bInspectDragging = false;
	}

	RotatePreviews(DeltaTime);
}

void APlayerBase::UpdateCameraRotation()
{
	Camera->SetRelativeRotation(FRotator(LookPitchOffset, LookYawOffset, 0.0));
}

void APlayerBase::ApplyLookDelta(double Delta)
{
	LookYawOffset = FMath::Clamp(LookYawOffset + Delta, -YawLimit, YawLimit);
	UpdateCameraRotation();
}

void APlayerBase::ApplyLookPitchDelta(double Delta)
{
	LookPitchOffset = FMath::Clamp(LookPitchOffset + Delta, PitchMin, PitchMax);
	UpdateCameraRotation();
}

void APlayerBase::OnLook(const FInputActionValue& Value)
{
	if (isInspecting || IsTransitioning || bIsCameraFocused || bInventoryOpen) return;

	const double Dt = GetWorld()->GetDeltaSeconds();
	LookYawOffset = FMath::Clamp(LookYawOffset + static_cast<double>(Value.Get<float>()) * Dt * 60.0, -YawLimit, YawLimit);
	UpdateCameraRotation();
}

bool APlayerBase::IsMouseNearScreenEdge()
{
	if (!CachedPlayerController) return false;

	float MouseX = 0.f;
	float MouseY = 0.f;
	int32 SizeX = 0;
	int32 SizeY = 0;

	if (!CachedPlayerController->GetMousePosition(MouseX, MouseY)) return false;
	CachedPlayerController->GetViewportSize(SizeX, SizeY);
	if (SizeX <= 0 || SizeY <= 0) return false;

	return MouseX < EdgeLookMargin || MouseX > SizeX - EdgeLookMargin
		|| MouseY < EdgeLookMargin || MouseY > SizeY - EdgeLookMargin;
}

void APlayerBase::OnClick()
{
	if (bInventoryOpen) return;

	bClickHeld = true;

	UE_LOG(LogTemp, Warning, TEXT("OnClick chamado | isInspecting = %d | IsTransitioning = %d"), isInspecting ? 1 : 0, IsTransitioning ? 1 : 0);

	if (!CachedPlayerController) return;

	if (isInspecting)
	{
		if (!IsTransitioning)
		{
			const bool bNearEdge = IsMouseNearScreenEdge();

			float ClickX = 0.f;
			float ClickY = 0.f;
			int32 ViewX = 0;
			int32 ViewY = 0;
			CachedPlayerController->GetMousePosition(ClickX, ClickY);
			CachedPlayerController->GetViewportSize(ViewX, ViewY);
			UE_LOG(LogTemp, Warning, TEXT("Inspecao: clique em (%.0f, %.0f) | viewport = (%d, %d) | perto da borda = %d"), ClickX, ClickY, ViewX, ViewY, bNearEdge ? 1 : 0);

			if (bNearEdge)
			{
				ExitInspection();
			}
			else
			{
				// Tenta primeiro o novo sistema de reparo (data-driven)
				if (bEnableNewRepairSystem)
				{
					TryUseItemOnRepairable();
				}
				// Fallback para o sistema antigo (ARepairPart actors)
				TryUseItemOnPart();
			}
		}
		return;
	}

	FVector WorldOrigin;
	FVector WorldDirection;
	if (!CachedPlayerController->DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
	{
		UE_LOG(LogTemp, Warning, TEXT("Click: falhou o Deproject do mouse"));
		return;
	}

	const FVector TraceEnd = WorldOrigin + WorldDirection * 100000.0;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(PlayerClickTrace), true);
	Params.AddIgnoredActor(this);

	AActor* FallbackClickable = nullptr;

	for (int32 i = 0; i < 8; ++i)
	{
		FHitResult Hit;
		if (!GetWorld()->LineTraceSingleByChannel(Hit, WorldOrigin, TraceEnd, ECC_Visibility, Params))
		{
			UE_LOG(LogTemp, Warning, TEXT("Click: trace %d não acertou nada"), i);
			break;
		}

		AActor* HitActor = Hit.GetActor();
		if (!HitActor) break;

		UE_LOG(LogTemp, Warning, TEXT("Click: trace %d acertou '%s'"), i, *HitActor->GetName());

		// Pegar item solto no cenario (ex.: bateria nova)
		if (AItemPickup* Pickup = Cast<AItemPickup>(HitActor))
		{
			if (Pickup->TryPickup(this)) return;
		}

		// Recolher a bateria carregada do carregador
		if (ABatteryCharger* Charger = Cast<ABatteryCharger>(HitActor))
		{
			if (Charger->TryCollect(this)) return;
		}

		if (Inventory && HitActor->GetClass()->ImplementsInterface(UItemUseTarget::StaticClass()))
		{
			FInventoryItem Equipped;
			if (Inventory->GetEquippedItem(Equipped) && IItemUseTarget::Execute_CanUseItem(HitActor, Equipped))
			{
				const bool bConsume = IItemUseTarget::Execute_UseItem(HitActor, Equipped, this);
				if (bConsume)
				{
					Inventory->RemoveItem(Equipped.InstanceId);
				}

				// Broadcast noise event if the item makes noise (Radio, Bell, etc.)
				if (Equipped.Data && (Equipped.Data->HasTag("Radio") || Equipped.Data->HasTag("NoiseMaker") || Equipped.Data->HasTag("Bell")))
				{
					FNoiseEvent NoiseEvent;
					NoiseEvent.NoiseTag = MonsterTags::Noise_Event_Radio_Active;
					NoiseEvent.Location = GetActorLocation();
					NoiseEvent.Loudness = 1.0f;
					NoiseEvent.HearRange = 2000.0f;
					NoiseEvent.Instigator = this;
					NoiseEvent.Timestamp = GetWorld()->GetTimeSeconds();
					OnPlayerMadeNoise.Broadcast(NoiseEvent);
				}

				return;
			}
		}

		if (HitActor->ActorHasTag("Inspectable"))
		{
			StartInspection(HitActor);
			return;
		}
		if (HitActor->ActorHasTag("CameraFocus"))
		{
			StartCameraFocus(HitActor);
			return;
		}
		if (HitActor->ActorHasTag("Clickable"))
		{
			if (!FallbackClickable) FallbackClickable = HitActor;
			Params.AddIgnoredActor(HitActor);
			continue;
		}

		// Check for Door interaction
		if (HitActor->ActorHasTag("Door"))
		{
			OnPlayerClosedDoor.Broadcast(HitActor);
			return;
		}

		break;
	}

	if (FallbackClickable && FallbackClickable != CurrentAreaActor)
	{
		CurrentAreaActor = FallbackClickable;
		PositionHistory.Add(GetActorTransform());
		MoveToPosition(FallbackClickable->GetActorTransform());
	}
}

void APlayerBase::TryUseItemOnPart()
{
	if (!Inventory || !CachedPlayerController) return;

	FInventoryItem Equipped;
	if (!Inventory->GetEquippedItem(Equipped)) return;

	FVector WorldOrigin;
	FVector WorldDirection;
	if (!CachedPlayerController->DeprojectMousePositionToWorld(WorldOrigin, WorldDirection)) return;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(PartClickTrace), true);
	Params.AddIgnoredActor(this);

	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, WorldOrigin, WorldOrigin + WorldDirection * 100000.0, ECC_Visibility, Params)) return;

	AActor* HitActor = Hit.GetActor();
	if (!HitActor) return;

	// 1. Tenta o novo sistema de reparo (URepairableComponent no actor ou no parent)
	if (bEnableNewRepairSystem)
	{
		URepairableComponent* Repairable = HitActor->FindComponentByClass<URepairableComponent>();
		if (!Repairable && HitActor->GetAttachParentActor())
		{
			Repairable = HitActor->GetAttachParentActor()->FindComponentByClass<URepairableComponent>();
		}

		if (Repairable && IItemUseTarget::Execute_CanUseItem(Repairable, Equipped))
		{
			IItemUseTarget::Execute_UseItem(Repairable, Equipped, this);
			return;
		}
	}

	// 2. Fallback para o sistema antigo (ARepairPart actors implementando IItemUseTarget)
	if (!HitActor->GetClass()->ImplementsInterface(UItemUseTarget::StaticClass())) return;
	if (!IItemUseTarget::Execute_CanUseItem(HitActor, Equipped)) return;

	if (IItemUseTarget::Execute_UseItem(HitActor, Equipped, this))
	{
		Inventory->RemoveItem(Equipped.InstanceId);
	}
}

void APlayerBase::SetPartsInteractable(AActor* Parent, bool bInteractable)
{
	if (!Parent) return;

	TArray<AActor*> Attached;
	Parent->GetAttachedActors(Attached);

	for (AActor* Child : Attached)
	{
		if (ARepairPart* Part = Cast<ARepairPart>(Child))
		{
			Part->SetInteractable(bInteractable);
		}
	}
}

void APlayerBase::OnClickReleased()
{
	UE_LOG(LogTemp, Warning, TEXT("Click solto"));

	bClickHeld = false;
	bInspectDragging = false;

	// Para reparo ativo se houver
	if (bIsActivelyRepairing)
	{
		StopRepairInteraction();
	}
}

void APlayerBase::OnInventoryOpen()
{
	if (bInventoryOpen || !Inventory) return;

	bInventoryOpen = true;
	bClickHeld = false;
	bInspectDragging = false;

	Inventory->BeginSelection();

	if (InventoryWidget)
	{
		InventoryWidget->Open();
	}
}

void APlayerBase::OnInventoryClose()
{
	if (!bInventoryOpen) return;

	bInventoryOpen = false;

	if (Inventory)
	{
		Inventory->ConfirmSelection();
	}

	if (InventoryWidget)
	{
		InventoryWidget->Close();
	}
}

void APlayerBase::OnInventoryScroll(const FInputActionValue& Value)
{
	const float Axis = Value.Get<float>();

	if (isInspecting && !bInventoryOpen)
	{
		if (!IsTransitioning && !bWorkingOnPart && !FMath::IsNearlyZero(Axis))
		{
			InspectionZoomTarget = FMath::Clamp(InspectionZoomTarget - Axis * InspectionZoomStep, InspectionZoomMin, InspectionZoomMax);
		}
		return;
	}

	if (!bInventoryOpen || !Inventory) return;
	if (FMath::IsNearlyZero(Axis)) return;

	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastInventoryScrollTime < InventoryScrollInterval) return;

	LastInventoryScrollTime = Now;
	Inventory->ScrollSelection(Axis > 0.f ? 1 : -1);
}

void APlayerBase::OnGoBack()
{
	UE_LOG(LogTemp, Warning, TEXT("OnGoBack chamado | isInspecting = %d | IsTransitioning = %d"), isInspecting ? 1 : 0, IsTransitioning ? 1 : 0);

	if (IsTransitioning || bInventoryOpen) return;

	if (isInspecting)
	{
		ExitInspection();
		return;
	}

	if (bIsCameraFocused)
	{
		ExitCameraFocus();
		return;
	}

	if (PositionHistory.Num() <= 0) return;

	const FTransform Last = PositionHistory.Last();
	PositionHistory.RemoveAt(PositionHistory.Num() - 1);
	CurrentAreaActor = nullptr;
	MoveToPosition(Last);
}

void APlayerBase::MoveToPosition(const FTransform& Target)
{
	CameraFocusTimeline.Stop();
	bIsCameraFocused = false;
	bExitingCameraFocus = false;

	LookPitchOffset = 0.0;
	LookYawOffset = 0.0;
	Camera->SetRelativeLocation(DefaultCameraRelativeLocation);
	Camera->SetRelativeRotation(FRotator::ZeroRotator);

	StartLocation = GetActorLocation();
	StartRotation = GetActorRotation();
	TargetLocation = Target.GetLocation();
	TargetRotation = Target.Rotator();

	IsTransitioning = true;
	MoveTimeline.PlayFromStart();
}

void APlayerBase::OnMoveUpdate(float Alpha)
{
	const float ClampedAlpha = FMath::Clamp(Alpha, 0.f, 1.f);

	SetActorLocationAndRotation(
		FMath::Lerp(StartLocation, TargetLocation, ClampedAlpha),
		UKismetMathLibrary::RLerp(StartRotation, TargetRotation, ClampedAlpha, false));

	const double Z = FMath::Sin(ClampedAlpha * BobFrequency * UE_DOUBLE_PI) * BobAmplitude;
	Camera->SetRelativeLocation(DefaultCameraRelativeLocation + FVector(0.0, 0.0, Z));
}

void APlayerBase::OnMoveFinished()
{
	SetActorLocationAndRotation(TargetLocation, TargetRotation);
	Camera->SetRelativeLocation(DefaultCameraRelativeLocation);

	IsTransitioning = false;
}

void APlayerBase::StartCameraFocus(AActor* FocusActor)
{
	if (!FocusActor || bIsCameraFocused) return;

	FocusStartLocation = Camera->GetRelativeLocation();
	FocusStartRotation = Camera->GetRelativeRotation();

	const FTransform Rel = FocusActor->GetActorTransform().GetRelativeTransform(GetActorTransform());
	FocusTargetLocation = Rel.GetLocation();
	FocusTargetRotation = Rel.Rotator();

	bIsCameraFocused = true;
	bExitingCameraFocus = false;
	IsTransitioning = true;

	CameraFocusTimeline.PlayFromStart();
}

void APlayerBase::ExitCameraFocus()
{
	if (!bIsCameraFocused || IsTransitioning) return;

	bExitingCameraFocus = true;
	IsTransitioning = true;

	CameraFocusTimeline.ReverseFromEnd();
}

void APlayerBase::OnCameraFocusUpdate(float Alpha)
{
	Camera->SetRelativeLocationAndRotation(
		FMath::Lerp(FocusStartLocation, FocusTargetLocation, Alpha),
		UKismetMathLibrary::RLerp(FocusStartRotation, FocusTargetRotation, Alpha, true));
}

void APlayerBase::OnCameraFocusFinished()
{
	IsTransitioning = false;

	if (bExitingCameraFocus)
	{
		bExitingCameraFocus = false;
		bIsCameraFocused = false;
	}
	else
	{
		// Player finished focusing on something - check if it was a security camera
		if (InspectedObject && InspectedObject->ActorHasTag("SecurityCamera"))
		{
			OnPlayerUsedSecurityCamera.Broadcast(InspectedObject);
		}
	}
}

void APlayerBase::StartInspection(AActor* TargetObject)
{
	if (!TargetObject || isInspecting) return;

	if (const USceneComponent* TargetRoot = TargetObject->GetRootComponent())
	{
		if (TargetRoot->Mobility != EComponentMobility::Movable)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("PlayerBase: '%s' é Inspectable mas não está como Movable. Mude a Mobility para Movable no Details."),
				*TargetObject->GetName());
		}
	}

	InspectionZoomTarget = InspectionDistance;

	InspectedObject = TargetObject;
	isInspecting = true;
	IsTransitioning = true;
	bExitingInspection = false;
	bInspectDragging = false;
	UE_LOG(LogTemp, Warning, TEXT("Inspecao iniciada: '%s' | blur = %d | FStop = %.2f | distancia = %.0f"), *TargetObject->GetName(), bBlurBackground ? 1 : 0, InspectionBlurFStop, InspectionDistance);

	InspectStartLocation = TargetObject->GetActorLocation();
	InspectStartRotation = TargetObject->GetActorRotation();
	InspectStartScale = TargetObject->GetActorScale3D();

	FVector BoundsOrigin = FVector::ZeroVector;
	FVector BoundsExtent = FVector::ZeroVector;

	// Mede só os meshes visíveis. Para Skeletal Mesh, usa o bounds do asset
	// (e não o do componente, que pode ser afetado pelo Physics Asset).
	FBox VisualBox(ForceInit);
	TArray<UPrimitiveComponent*> BoundsPrimitives;
	TargetObject->GetComponents<UPrimitiveComponent>(BoundsPrimitives);
	for (UPrimitiveComponent* Prim : BoundsPrimitives)
	{
		if (!Prim || !Prim->IsRegistered() || Cast<UShapeComponent>(Prim)) continue;

		if (const USkeletalMeshComponent* Skel = Cast<USkeletalMeshComponent>(Prim))
		{
			if (const USkeletalMesh* MeshAsset = Skel->GetSkeletalMeshAsset())
			{
				VisualBox += MeshAsset->GetBounds().TransformBy(Skel->GetComponentTransform()).GetBox();
			}
			// Skeletal Mesh Component sem mesh (vazio) nao entra na medida
			continue;
		}
		VisualBox += Prim->Bounds.GetBox();
	}

	if (VisualBox.IsValid)
	{
		VisualBox.GetCenterAndExtents(BoundsOrigin, BoundsExtent);
	}
	else
	{
		TargetObject->GetActorBounds(false, BoundsOrigin, BoundsExtent);
	}

	UE_LOG(LogTemp, Warning, TEXT("Inspecao: bounds origem = %s | extensao = %s | escala do ator = %s"), *BoundsOrigin.ToString(), *BoundsExtent.ToString(), *TargetObject->GetActorScale3D().ToString());

	InspectStartCenter = BoundsOrigin;
	InspectLocalCenterOffset = TargetObject->GetActorQuat().UnrotateVector(BoundsOrigin - InspectStartLocation);

	InspectScaleRatio = 1.0;
	if (bAutoFitInspectedObject)
	{
		const double MaxSize = FMath::Max3(BoundsExtent.X, BoundsExtent.Y, BoundsExtent.Z) * 2.0;
		if (MaxSize > KINDA_SMALL_NUMBER)
		{
			InspectScaleRatio = InspectionTargetSize / MaxSize;
		}
	}

	bInspectedObjectHadCollision = TargetObject->GetActorEnableCollision();
	TargetObject->SetActorEnableCollision(false);

	InspectedShadowStates.Reset();
	if (bDisableShadowDuringInspection)
	{
		TArray<UPrimitiveComponent*> Primitives;
		TargetObject->GetComponents<UPrimitiveComponent>(Primitives);

		for (UPrimitiveComponent* Primitive : Primitives)
		{
			InspectedShadowStates.Add(Primitive, Primitive->CastShadow);
			Primitive->SetCastShadow(false);
		}
	}

	InspectTimeline.PlayFromStart();

	SetPartsInteractable(TargetObject, true);
}

void APlayerBase::ExitInspection()
{
	if (!isInspecting || IsTransitioning) return;

	if (!InspectedObject)
	{
		isInspecting = false;
		ClearInspectionBlur();
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("ExitInspection chamado"));

	// Para reparo ativo ao sair da inspeção
	if (bIsActivelyRepairing)
	{
		StopRepairInteraction();
	}

	InspectExitRotation = InspectedObject->GetActorRotation();
	bExitingInspection = true;
	bInspectDragging = false;
	IsTransitioning = true;

	InspectTimeline.ReverseFromEnd();

	SetPartsInteractable(InspectedObject, false);
}

void APlayerBase::PlaceInspectedObject(const FVector& CenterWorld, double ScaleRatio)
{
	if (!InspectedObject) return;

	InspectedObject->SetActorScale3D(InspectStartScale * ScaleRatio);

	const FQuat Q = InspectedObject->GetActorQuat();
	InspectedObject->SetActorLocation(CenterWorld - Q.RotateVector(InspectLocalCenterOffset * ScaleRatio));
}

void APlayerBase::UpdateInspectionZoom(float DeltaTime)
{
	if (!InspectedObject || IsTransitioning) return;

	const double Current = InspectionAnchor->GetRelativeLocation().X;
	if (FMath::IsNearlyEqual(Current, InspectionZoomTarget, 0.01)) return;

	const double NewDistance = FMath::FInterpTo(Current, InspectionZoomTarget, static_cast<double>(DeltaTime), InspectionZoomSpeed);
	InspectionAnchor->SetRelativeLocation(FVector(NewDistance, 0.0, 0.0));
	PlaceInspectedObject(InspectionAnchor->GetComponentLocation(), InspectScaleRatio);

	if (bBlurBackground)
	{
		Camera->PostProcessSettings.DepthOfFieldFocalDistance = static_cast<float>(NewDistance);
	}
}

void APlayerBase::UpdateInspectionRotation()
{
	const bool bHolding = !IsTransitioning && InspectedObject && bClickHeld && !bInventoryOpen && !bWorkingOnPart;

	if (!bHolding)
	{
		bInspectDragging = false;
		return;
	}

	float MouseX = 0.f;
	float MouseY = 0.f;
	if (!CachedPlayerController->GetMousePosition(MouseX, MouseY))
	{
		bInspectDragging = false;
		return;
	}

	const FVector2D Current(MouseX, MouseY);

	if (bInspectDragging)
	{
		const FVector2D Delta = Current - LastInspectMousePos;

		const double DX = Delta.X;
		const double DY = -Delta.Y;

		const FQuat YawQuat(Camera->GetUpVector(), FMath::DegreesToRadians(DX * InspectRotateSpeed * -1.0));
		const FQuat PitchQuat(Camera->GetRightVector(), FMath::DegreesToRadians(DY * InspectRotateSpeed));

		InspectedObject->SetActorRotation(PitchQuat * YawQuat * InspectedObject->GetActorQuat());
		PlaceInspectedObject(InspectionAnchor->GetComponentLocation(), InspectScaleRatio);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Inspecao: arraste comecou em (%.0f, %.0f)"), MouseX, MouseY);
	}

	LastInspectMousePos = Current;
	bInspectDragging = true;
}

void APlayerBase::OnInspectUpdate(float Alpha)
{
	if (!InspectedObject) return;

	if (bExitingInspection)
	{
		InspectedObject->SetActorRotation(
			UKismetMathLibrary::RLerp(InspectStartRotation, InspectExitRotation, Alpha, true));
	}

	const double Ratio = FMath::Lerp(1.0, InspectScaleRatio, static_cast<double>(Alpha));
	const FVector Center = FMath::Lerp(InspectStartCenter, InspectionAnchor->GetComponentLocation(), Alpha);
	PlaceInspectedObject(Center, Ratio);

	UpdateInspectionBlur(Alpha);
}

void APlayerBase::OnInspectFinished()
{
	UE_LOG(LogTemp, Warning, TEXT("OnInspectFinished | saindo = %d"), bExitingInspection ? 1 : 0);
	IsTransitioning = false;

	if (bExitingInspection)
	{
		bExitingInspection = false;

		if (InspectedObject)
		{
			InspectedObject->SetActorScale3D(InspectStartScale);
			InspectedObject->SetActorLocationAndRotation(InspectStartLocation, InspectStartRotation);
			InspectedObject->SetActorEnableCollision(bInspectedObjectHadCollision);
		}

		for (const TPair<TWeakObjectPtr<UPrimitiveComponent>, bool>& Entry : InspectedShadowStates)
		{
			if (UPrimitiveComponent* Primitive = Entry.Key.Get())
			{
				Primitive->SetCastShadow(Entry.Value);
			}
		}
		InspectedShadowStates.Reset();

		InspectedObject = nullptr;
		isInspecting = false;
		bInspectDragging = false;

		InspectionAnchor->SetRelativeLocation(FVector(InspectionDistance, 0.0, 0.0));
		InspectionZoomTarget = InspectionDistance;

		ClearInspectionBlur();
	}
}

void APlayerBase::UpdateInspectionBlur(float Alpha)
{
	if (!bBlurBackground || !Camera) return;

	FPostProcessSettings& PP = Camera->PostProcessSettings;
	PP.bOverride_DepthOfFieldFocalDistance = true;
	PP.bOverride_DepthOfFieldFstop = true;

	PP.DepthOfFieldFocalDistance = static_cast<float>(InspectionAnchor->GetRelativeLocation().X);

	const float InvFStop = FMath::Lerp(1.f / 32.f, 1.f / FMath::Max(InspectionBlurFStop, 0.05f), Alpha);
	PP.DepthOfFieldFstop = 1.f / InvFStop;

	Camera->PostProcessBlendWeight = 1.f;
}

void APlayerBase::ClearInspectionBlur()
{
	if (!Camera) return;

	FPostProcessSettings& PP = Camera->PostProcessSettings;
	PP.bOverride_DepthOfFieldFocalDistance = false;
	PP.bOverride_DepthOfFieldFstop = false;
}

void APlayerBase::OnPause()
{
	if (bInventoryOpen)
	{
		OnInventoryClose();
	}

	if (!CachedPlayerController) return;

	if (PauseMenuClass)
	{
		if (UUserWidget* Widget = CreateWidget<UUserWidget>(CachedPlayerController, PauseMenuClass))
		{
			Widget->AddToViewport();
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerBase: PauseMenuClass não foi atribuída no Class Defaults."));
	}

	CachedPlayerController->bShowMouseCursor = true;
	UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(CachedPlayerController, nullptr, EMouseLockMode::DoNotLock, false);
	UGameplayStatics::SetGamePaused(this, true);
}

void APlayerBase::RotatePreviews(float DeltaTime)
{
	float YawDelta = PreviewRotationSpeed * DeltaTime;
	for (AActor* Actor : PreviewActors)
	{
		if (Actor && Actor->IsValidLowLevel())
		{
			Actor->AddActorWorldRotation(FRotator(0.0f, YawDelta, 0.0f));
		}
	}
}

// --- Novo Sistema de Reparo Data-Driven ---

void APlayerBase::TryUseItemOnRepairable()
{
	if (!Inventory || !CachedPlayerController || !InspectedObject) return;

	// Mao livre tambem vale: passos sem ferramenta (Required Tool Tag = None) nao exigem item equipado.
	// Sem item equipado, Equipped fica vazio (Data nula).
	FInventoryItem Equipped;
	Inventory->GetEquippedItem(Equipped);

	// Procura URepairableComponent no objeto inspecionado
	URepairableComponent* Repairable = InspectedObject->FindComponentByClass<URepairableComponent>();
	if (!Repairable)
	{
		return;
	}

	// Verifica se pode usar o item equipado
	if (!IItemUseTarget::Execute_CanUseItem(Repairable, Equipped))
	{
		return;
	}

	// Inicia interação de reparo
	StartRepairInteraction(Repairable);

	// Tenta usar o item (isso vai iniciar o passo apropriado)
	IItemUseTarget::Execute_UseItem(Repairable, Equipped, this);
}

void APlayerBase::UpdateActiveRepair(float DeltaTime)
{
	if (!CurrentRepairableComponent || !bIsActivelyRepairing) return;

	// Atualiza progresso do componente de reparo
	CurrentRepairableComponent->UpdateRepairProgress(DeltaTime);

	// Atualiza widget de progresso
	if (RepairProgressWidget)
	{
		const int32 ActiveStepIndex = CurrentRepairableComponent->GetActiveStepIndex();
		if (ActiveStepIndex != INDEX_NONE)
		{
			const FRepairStep& ActiveStep = CurrentRepairableComponent->GetActiveStep();
			RepairProgressWidget->UpdateProgress(ActiveStep.CurrentProgress);
		}
	}

	// Verifica se o jogador soltou o clique ou trocou de ferramenta
	if (!IsClickHeld())
	{
		StopRepairInteraction();
	}
	else if (Inventory)
	{
		const int32 ActiveStepIndex = CurrentRepairableComponent->GetActiveStepIndex();
		if (ActiveStepIndex != INDEX_NONE)
		{
			const FRepairStep& ActiveStep = CurrentRepairableComponent->GetActiveStep();

			// Passo sem ferramenta (Required Tool Tag = None): nao precisa de item equipado
			if (!ActiveStep.RequiredToolTag.IsNone())
			{
				FInventoryItem Equipped;
				if (!Inventory->GetEquippedItem(Equipped) || !Equipped.Data || !Equipped.Data->HasTag(ActiveStep.RequiredToolTag))
				{
					StopRepairInteraction();
				}
			}
		}
	}
}

void APlayerBase::StartRepairInteraction(URepairableComponent* Repairable)
{
	if (!Repairable) return;

	CurrentRepairableComponent = Repairable;
	bIsActivelyRepairing = true;

	// Cria/mostra widget de progresso
	if (RepairProgressWidgetClass && !RepairProgressWidget)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			RepairProgressWidget = CreateWidget<URepairProgressWidget>(PC, RepairProgressWidgetClass);
			if (RepairProgressWidget)
			{
				RepairProgressWidget->AddToViewport(5); // Acima do inventário
			}
		}
	}

	// Bind delegates
	if (CurrentRepairableComponent)
	{
		CurrentRepairableComponent->OnStepStarted.AddDynamic(this, &APlayerBase::OnRepairStepStarted);
		CurrentRepairableComponent->OnStepProgress.AddDynamic(this, &APlayerBase::OnRepairStepProgress);
		CurrentRepairableComponent->OnStepCompleted.AddDynamic(this, &APlayerBase::OnRepairStepCompleted);
		CurrentRepairableComponent->OnAllRepairsCompleted.AddDynamic(this, &APlayerBase::OnAllRepairsCompleted);
	}

	UE_LOG(LogTemp, Log, TEXT("PlayerBase: Iniciado reparo em '%s'"), *InspectedObject->GetName());
}

void APlayerBase::StopRepairInteraction()
{
	if (!bIsActivelyRepairing) return;

	bIsActivelyRepairing = false;

	// Para o componente de reparo
	if (CurrentRepairableComponent)
	{
		CurrentRepairableComponent->StopRepair();

		CurrentRepairableComponent->OnStepStarted.RemoveDynamic(this, &APlayerBase::OnRepairStepStarted);
		CurrentRepairableComponent->OnStepProgress.RemoveDynamic(this, &APlayerBase::OnRepairStepProgress);
		CurrentRepairableComponent->OnStepCompleted.RemoveDynamic(this, &APlayerBase::OnRepairStepCompleted);
		CurrentRepairableComponent->OnAllRepairsCompleted.RemoveDynamic(this, &APlayerBase::OnAllRepairsCompleted);
	}

	// Esconde widget
	if (RepairProgressWidget)
	{
		RepairProgressWidget->Hide();
		RepairProgressWidget = nullptr;
	}

	CurrentRepairableComponent = nullptr;

	UE_LOG(LogTemp, Log, TEXT("PlayerBase: Reparo interrompido"));
}

void APlayerBase::OnRepairStepStarted(const FRepairStep& Step)
{
	if (RepairProgressWidget)
	{
		RepairProgressWidget->SetStep(Step);
	}

	UE_LOG(LogTemp, Log, TEXT("PlayerBase: Passo de reparo iniciado: %s"), *Step.StepId.ToString());
}

void APlayerBase::OnRepairStepProgress(const FRepairStep& Step, float Progress)
{
	if (RepairProgressWidget)
	{
		RepairProgressWidget->UpdateProgress(Progress);
	}
}

void APlayerBase::OnRepairStepCompleted(const FRepairStep& Step)
{
	UE_LOG(LogTemp, Log, TEXT("PlayerBase: Passo de reparo completado: %s"), *Step.StepId.ToString());

	// O widget continua mostrando o progresso até o próximo passo ser iniciado
	// O próximo passo só inicia quando o jogador clica novamente com a ferramenta certa
}

void APlayerBase::OnAllRepairsCompleted()
{
	UE_LOG(LogTemp, Log, TEXT("PlayerBase: TODOS OS REPAROS COMPLETOS em '%s'!"), *InspectedObject->GetName());

	// Esconde widget após um delay
	if (RepairProgressWidget)
	{
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, [this]()
			{
				if (RepairProgressWidget)
				{
					RepairProgressWidget->Hide();
				}
			}, 2.0f, false);
	}

	StopRepairInteraction();

	// Aqui você pode adicionar lógica extra:
	// - Trocar mesh do objeto para versão "reparada"
	// - Habilitar funcionalidade do objeto (ex.: rádio toca)
	// - Dar item ao jogador
	// - Tocar som de sucesso
	// - Trigger evento de quest
}

void APlayerBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerBase: o Input Component não é um EnhancedInputComponent (Project Settings > Input > Default Classes)."));
		return;
	}

	if (ClickAction)
	{
		EIC->BindAction(ClickAction, ETriggerEvent::Started, this, &APlayerBase::OnClick);
		EIC->BindAction(ClickAction, ETriggerEvent::Completed, this, &APlayerBase::OnClickReleased);
		EIC->BindAction(ClickAction, ETriggerEvent::Canceled, this, &APlayerBase::OnClickReleased);
	}
	else { UE_LOG(LogTemp, Warning, TEXT("PlayerBase: ClickAction não atribuída.")); }

	if (GoBackAction) { EIC->BindAction(GoBackAction, ETriggerEvent::Triggered, this, &APlayerBase::OnGoBack); }
	else { UE_LOG(LogTemp, Warning, TEXT("PlayerBase: GoBackAction não atribuída.")); }

	if (LookAction) { EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerBase::OnLook); }
	else { UE_LOG(LogTemp, Warning, TEXT("PlayerBase: LookAction não atribuída.")); }

	if (PauseAction) { EIC->BindAction(PauseAction, ETriggerEvent::Started, this, &APlayerBase::OnPause); }
	else { UE_LOG(LogTemp, Warning, TEXT("PlayerBase: PauseAction não atribuída.")); }

	if (InventoryAction)
	{
		EIC->BindAction(InventoryAction, ETriggerEvent::Started, this, &APlayerBase::OnInventoryOpen);
		EIC->BindAction(InventoryAction, ETriggerEvent::Completed, this, &APlayerBase::OnInventoryClose);
		EIC->BindAction(InventoryAction, ETriggerEvent::Canceled, this, &APlayerBase::OnInventoryClose);
	}
	else { UE_LOG(LogTemp, Warning, TEXT("PlayerBase: InventoryAction não atribuída.")); }

	if (InventoryScrollAction) { EIC->BindAction(InventoryScrollAction, ETriggerEvent::Triggered, this, &APlayerBase::OnInventoryScroll); }
	else { UE_LOG(LogTemp, Warning, TEXT("PlayerBase: InventoryScrollAction não atribuída.")); }
}