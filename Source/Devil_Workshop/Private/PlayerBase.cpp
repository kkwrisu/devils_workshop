#include "PlayerBase.h"

#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
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
#include "RepairStep.h"
#include "TaskListBoard.h"
#include "TaskListWidget.h"
#include "PrologueCompletedWidget.h"
#include "CorridorReticleWidget.h"
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

	CorridorCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CorridorCollision"));
	CorridorCollision->SetupAttachment(Root);
	CorridorCollision->SetCapsuleSize(26.0f, 65.0f);
	CorridorCollision->SetRelativeLocation(CorridorCollisionRelativeLocation);
	CorridorCollision->SetCollisionProfileName(TEXT("Pawn"));
	CorridorCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CorridorCollision->SetGenerateOverlapEvents(false);

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

	InspectionLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("InspectionLight"));
	InspectionLight->SetupAttachment(InspectionAnchor);
	InspectionLight->SetMobility(EComponentMobility::Movable);
	InspectionLight->SetRelativeLocation(FVector(-50.0, 0.0, 25.0));
	InspectionLight->SetLightingChannels(false, true, false);
	InspectionLight->SetVisibility(false);

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
	CorridorStandCameraRelativeLocation = Camera->GetRelativeLocation() - FVector(0.0f, 0.0f, CorridorCameraHeightDrop);
	if (CorridorCollision)
	{
		CorridorStandCollisionRelativeLocation = CorridorCollision->GetRelativeLocation();
		CorridorStandCapsuleHalfHeight = CorridorCollision->GetUnscaledCapsuleHalfHeight();
		CorridorStandCapsuleRadius = CorridorCollision->GetUnscaledCapsuleRadius();
	}
	InspectionLight->SetIntensity(InspectionLightIntensity);
	InspectionLight->SetAttenuationRadius(InspectionLightRadius);
	InspectionLight->SetSourceRadius(InspectionLightSourceRadius);
	InspectionLight->SetLightColor(InspectionLightColor);
	InspectionLight->SetLightingChannels(false, true, false);
	InspectionLight->SetVisibility(false);

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

		CorridorReticleWidget = CreateWidget<UCorridorReticleWidget>(PC, UCorridorReticleWidget::StaticClass());
		if (CorridorReticleWidget)
		{
			CorridorReticleWidget->AddToViewport(50);
			CorridorReticleWidget->SetVisibility(ESlateVisibility::Collapsed);
		}

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
	if (bPrologueFadeActive && PrologueCompletedWidget)
	{
		PrologueFadeElapsed += DeltaTime;
		const float BackgroundAlpha = FMath::Clamp(PrologueFadeElapsed / PrologueFadeDuration, 0.0f, 1.0f);
		const float TextAlpha = FMath::Clamp(
			(PrologueFadeElapsed - PrologueTextFadeDelay) / PrologueTextFadeDuration,
			0.0f,
			1.0f);
		PrologueCompletedWidget->SetFadeProgress(BackgroundAlpha, TextAlpha);

		if (BackgroundAlpha >= 1.0f && TextAlpha >= 1.0f)
		{
			bPrologueFadeActive = false;
			UE_LOG(LogTemp, Warning, TEXT("PlayerBase: fade de conclusao terminou; jogo segue em execucao."));
		}
	}

	// A lista bloqueia o controle do jogador sem pausar o restante do mundo.
	if (bTaskListOpen)
	{
		if (CachedPlayerController && TaskListWidget)
		{
			if (CachedPlayerController->WasInputKeyJustPressed(EKeys::E))
			{
				TaskListWidget->ShowControlsPage();
			}
			else if (CachedPlayerController->WasInputKeyJustPressed(EKeys::Q))
			{
				TaskListWidget->ShowRadioPage();
			}
		}
		bInspectDragging = false;
		return;
	}

	MoveTimeline.TickTimeline(DeltaTime);
	CameraFocusTimeline.TickTimeline(DeltaTime);
	InspectTimeline.TickTimeline(DeltaTime);

	if (!CachedPlayerController) return;

	if (bIsInCorridor && !IsTransitioning && !isInspecting && !bIsCameraFocused && !bInventoryOpen && !bTaskListOpen)
	{
		UpdateCorridorControls(DeltaTime);
	}
	else if (!bIsInCorridor && !IsTransitioning && !isInspecting && !bIsCameraFocused && !bInventoryOpen)
	{
		float MouseX = 0.f;
		float MouseY = 0.f;
		int32 SizeX = 0;
		int32 SizeY = 0;

		if (!CachedPlayerController->GetMousePosition(MouseX, MouseY)) return;
		CachedPlayerController->GetViewportSize(SizeX, SizeY);

		if (MouseX < EdgeLookMargin)
		{
			ApplyLookDelta(LookRotationSpeed * -1.0 * DeltaTime);
		}
		else if (MouseX > SizeX - EdgeLookMargin)
		{
			ApplyLookDelta(LookRotationSpeed * DeltaTime);
		}

		if (MouseY < EdgeLookMargin)
		{
			ApplyLookPitchDelta(DeltaTime * EdgePitchSpeed);
		}
		else if (MouseY > SizeY - EdgeLookMargin)
		{
			ApplyLookPitchDelta(DeltaTime * EdgePitchSpeed * -1.0);
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
	LookYawOffset += Delta;
	UpdateCameraRotation();
}

void APlayerBase::ApplyLookPitchDelta(double Delta)
{
	LookPitchOffset = FMath::Clamp(LookPitchOffset + Delta, PitchMin, PitchMax);
	UpdateCameraRotation();
}

void APlayerBase::OnLook(const FInputActionValue& Value)
{
	if (bIsInCorridor || isInspecting || IsTransitioning || bIsCameraFocused || bInventoryOpen || bTaskListOpen) return;

	const double Dt = GetWorld()->GetDeltaSeconds();
	LookYawOffset += static_cast<double>(Value.Get<float>()) * Dt * LookRotationSpeed;
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
	if (bInventoryOpen || bTaskListOpen) return;

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
	if (bIsInCorridor)
	{
		WorldOrigin = Camera->GetComponentLocation();
		WorldDirection = Camera->GetForwardVector();
	}
	else if (!CachedPlayerController->DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
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

		if (HitActor->ActorHasTag(TEXT("CorridorDoor")))
		{
			TryUseCorridorDoor(HitActor);
			return;
		}

		if (Cast<ATaskListBoard>(HitActor))
		{
			ToggleTaskList();
			return;
		}

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
		ReportGameplayNoise(FName(TEXT("Noise.Event.Footstep.Walk")), 0.7f, 2000.0f);
		CurrentAreaActor = FallbackClickable;
		PositionHistory.Add(GetActorTransform());
		PositionLookHistory.Add(FRotator(LookPitchOffset, LookYawOffset, 0.0));
		MoveToPosition(FallbackClickable->GetActorTransform());
	}
}

void APlayerBase::ReportGameplayNoise(FName NoiseTagName, float Loudness, float HearRange)
{
	if (NoiseTagName.IsNone() || !GetWorld()) return;

	FNoiseEvent NoiseEvent;
	NoiseEvent.NoiseTag = FGameplayTag::RequestGameplayTag(NoiseTagName, false);
	NoiseEvent.Location = GetActorLocation();
	NoiseEvent.Loudness = FMath::Max(0.0f, Loudness);
	NoiseEvent.HearRange = FMath::Max(0.0f, HearRange);
	NoiseEvent.Instigator = this;
	NoiseEvent.Timestamp = GetWorld()->GetTimeSeconds();
	OnPlayerMadeNoise.Broadcast(NoiseEvent);
}

void APlayerBase::UpdateCorridorControls(float DeltaTime)
{
	if (!CachedPlayerController || DeltaTime <= 0.0f) return;

	float MouseDeltaX = 0.0f;
	float MouseDeltaY = 0.0f;
	CachedPlayerController->GetInputMouseDelta(MouseDeltaX, MouseDeltaY);
	if (bIgnoreNextCorridorMouseDelta)
	{
		bIgnoreNextCorridorMouseDelta = false;
		MouseDeltaX = 0.0f;
		MouseDeltaY = 0.0f;
	}

	// Accumulate raw mouse motion, then ease the view toward it for smooth aiming.
	CorridorTargetYawOffset += MouseDeltaX * CorridorMouseSensitivity;
	const float PitchSign = bInvertCorridorMouseY ? -1.0f : 1.0f;
	CorridorTargetPitchOffset = FMath::Clamp(
		CorridorTargetPitchOffset + MouseDeltaY * CorridorMouseSensitivity * PitchSign,
		-CorridorPitchLimit,
		CorridorPitchLimit);
	LookYawOffset = FMath::FInterpTo(
		static_cast<float>(LookYawOffset), CorridorTargetYawOffset, DeltaTime, CorridorLookSmoothSpeed);
	LookPitchOffset = FMath::FInterpTo(
		static_cast<float>(LookPitchOffset), CorridorTargetPitchOffset, DeltaTime, CorridorLookSmoothSpeed);
	UpdateCameraRotation();
	UpdateCorridorReticle();

	const bool bCrouchPressed = CachedPlayerController->IsInputKeyDown(EKeys::LeftControl)
		|| CachedPlayerController->IsInputKeyDown(EKeys::RightControl);
	SetCorridorCrouched(bCrouchPressed);

	float ForwardInput = 0.0f;
	float RightInput = 0.0f;
	ForwardInput += CachedPlayerController->IsInputKeyDown(EKeys::W) ? 1.0f : 0.0f;
	ForwardInput -= CachedPlayerController->IsInputKeyDown(EKeys::S) ? 1.0f : 0.0f;
	RightInput += CachedPlayerController->IsInputKeyDown(EKeys::D) ? 1.0f : 0.0f;
	RightInput -= CachedPlayerController->IsInputKeyDown(EKeys::A) ? 1.0f : 0.0f;

	FVector Forward = Camera->GetForwardVector();
	FVector Right = Camera->GetRightVector();
	Forward.Z = 0.0f;
	Right.Z = 0.0f;
	Forward.Normalize();
	Right.Normalize();
	FVector MoveDirection = Forward * ForwardInput + Right * RightInput;
	MoveDirection = MoveDirection.GetClampedToMaxSize(1.0f);

	const bool bSprintPressed = CachedPlayerController->IsInputKeyDown(EKeys::LeftShift)
		|| CachedPlayerController->IsInputKeyDown(EKeys::RightShift);
	const float CurrentMoveSpeed = bIsCorridorCrouched
		? CorridorCrouchSpeed
		: (bSprintPressed ? CorridorSprintSpeed : CorridorMoveSpeed);
	const FVector DesiredVelocity = MoveDirection * CurrentMoveSpeed;
	const float MovementResponse = MoveDirection.IsNearlyZero()
		? CorridorMoveBraking
		: CorridorMoveAcceleration;
	CorridorVelocity = FMath::VInterpTo(CorridorVelocity, DesiredVelocity, DeltaTime, MovementResponse);
	const FVector MoveDelta = CorridorVelocity * DeltaTime;

	FVector AppliedDelta = MoveDelta;
	if (CorridorCollision)
	{
		const FVector CapsuleStartLocation = CorridorCollision->GetComponentLocation();
		const FVector SavedRelativeLocation = CorridorCollision->GetRelativeLocation();
		const FQuat SweepRotation = CorridorCollision->GetComponentQuat();
		FHitResult Hit;
		CorridorCollision->MoveComponent(MoveDelta, SweepRotation, true, &Hit);

		if (Hit.IsValidBlockingHit())
		{
			const FVector RemainingDelta = MoveDelta * (1.0f - Hit.Time);
			const FVector SlideDelta = FVector::VectorPlaneProject(RemainingDelta, Hit.ImpactNormal);
			if (!SlideDelta.IsNearlyZero())
			{
				FHitResult SlideHit;
				CorridorCollision->MoveComponent(SlideDelta, SweepRotation, true, &SlideHit);
			}
		}

		AppliedDelta = CorridorCollision->GetComponentLocation() - CapsuleStartLocation;
		CorridorCollision->SetRelativeLocation(SavedRelativeLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}

	AddActorWorldOffset(AppliedDelta, false);

	if (!AppliedDelta.IsNearlyZero())
	{
		CorridorFootstepNoiseTimer += DeltaTime;
		if (CorridorFootstepNoiseTimer >= CorridorFootstepNoiseInterval)
		{
			ReportGameplayNoise(FName(TEXT("Noise.Event.Footstep.Walk")), 0.7f, 2000.0f);
			CorridorFootstepNoiseTimer = 0.0f;
		}
	}
}

void APlayerBase::SetCorridorCrouched(bool bCrouched)
{
	if (bIsCorridorCrouched == bCrouched || !Camera)
	{
		return;
	}

	bIsCorridorCrouched = bCrouched;
	if (bCrouched)
	{
		const float NewHalfHeight = FMath::Clamp(
			CorridorCrouchCapsuleHalfHeight,
			CorridorCollision ? CorridorCollision->GetUnscaledCapsuleRadius() : 1.0f,
			CorridorStandCapsuleHalfHeight);
		const float HeightDifference = CorridorStandCapsuleHalfHeight - NewHalfHeight;

		if (CorridorCollision)
		{
			CorridorCollision->SetCapsuleSize(
				CorridorCollision->GetUnscaledCapsuleRadius(), NewHalfHeight, true);
			CorridorCollision->SetRelativeLocation(
				CorridorStandCollisionRelativeLocation - FVector(0.0f, 0.0f, HeightDifference));
		}

		Camera->SetRelativeLocation(
			CorridorStandCameraRelativeLocation - FVector(0.0f, 0.0f, CorridorCrouchCameraDrop));
	}
	else
	{
		if (CorridorCollision)
		{
			CorridorCollision->SetCapsuleSize(
				CorridorCollision->GetUnscaledCapsuleRadius(), CorridorStandCapsuleHalfHeight, true);
			CorridorCollision->SetRelativeLocation(CorridorStandCollisionRelativeLocation);
		}
		Camera->SetRelativeLocation(CorridorStandCameraRelativeLocation);
	}
}

bool APlayerBase::IsCorridorInteractable(AActor* Actor) const
{
	if (!IsValid(Actor)) return false;

	if (Cast<AItemPickup>(Actor)
		|| Cast<ABatteryCharger>(Actor)
		|| Cast<ATaskListBoard>(Actor)
		|| Actor->ActorHasTag(TEXT("CorridorDoor"))
		|| Actor->ActorHasTag(TEXT("Inspectable"))
		|| Actor->ActorHasTag(TEXT("CameraFocus"))
		|| Actor->ActorHasTag(TEXT("Door")))
	{
		return true;
	}

	if (Actor->GetClass()->ImplementsInterface(UItemUseTarget::StaticClass()) && Inventory)
	{
		FInventoryItem Equipped;
		if (Inventory->GetEquippedItem(Equipped)
			&& IItemUseTarget::Execute_CanUseItem(Actor, Equipped))
		{
			return true;
		}
	}

	return false;
}

void APlayerBase::UpdateCorridorReticle()
{
	if (!CorridorReticleWidget || !GetWorld() || !Camera) return;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CorridorReticleTrace), true);
	Params.AddIgnoredActor(this);

	const FVector Start = Camera->GetComponentLocation();
	const FVector End = Start + Camera->GetForwardVector() * 100000.0f;
	AActor* TargetActor = nullptr;

	// Match the click trace: clickable navigation markers can be skipped, while
	// solid non-interactive objects still block targets behind them.
	for (int32 Attempt = 0; Attempt < 8; ++Attempt)
	{
		if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
		{
			break;
		}

		AActor* HitActor = Hit.GetActor();
		if (!HitActor) break;

		if (IsCorridorInteractable(HitActor))
		{
			TargetActor = HitActor;
			break;
		}

		if (HitActor->ActorHasTag(TEXT("Clickable")))
		{
			Params.AddIgnoredActor(HitActor);
			continue;
		}

		break;
	}

	CorridorReticleWidget->SetHasInteractableTarget(TargetActor != nullptr);
}

void APlayerBase::SetCorridorMode(bool bEnable)
{
	SetCorridorCrouched(false);
	bIsInCorridor = bEnable;
	CorridorFootstepNoiseTimer = 0.0f;
	CorridorVelocity = FVector::ZeroVector;
	CorridorTargetYawOffset = static_cast<float>(LookYawOffset);
	CorridorTargetPitchOffset = static_cast<float>(LookPitchOffset);
	bIgnoreNextCorridorMouseDelta = bEnable;
	if (Camera)
	{
		Camera->SetRelativeLocation(bEnable ? CorridorStandCameraRelativeLocation : DefaultCameraRelativeLocation);
	}
	if (CorridorCollision)
	{
		CorridorCollision->SetCapsuleSize(
			CorridorStandCapsuleRadius, CorridorStandCapsuleHalfHeight, true);
		CorridorCollision->SetRelativeLocation(CorridorStandCollisionRelativeLocation);
		CorridorCollision->SetCollisionEnabled(
			bEnable ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}
	if (!CachedPlayerController) return;
	if (CorridorReticleWidget)
	{
		CorridorReticleWidget->SetVisibility(
			bEnable ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		CorridorReticleWidget->SetHasInteractableTarget(false);
	}

	CachedPlayerController->bShowMouseCursor = !bEnable;
	if (bEnable)
	{
		UWidgetBlueprintLibrary::SetInputMode_GameOnly(CachedPlayerController);
		int32 ViewportWidth = 0;
		int32 ViewportHeight = 0;
		CachedPlayerController->GetViewportSize(ViewportWidth, ViewportHeight);
		if (ViewportWidth > 0 && ViewportHeight > 0)
		{
			CachedPlayerController->SetMouseLocation(ViewportWidth / 2, ViewportHeight / 2);
		}
	}
	else
	{
		UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(
			CachedPlayerController, nullptr, EMouseLockMode::LockAlways, false);
	}
}

bool APlayerBase::TryUseCorridorDoor(AActor* DoorActor)
{
	if (!DoorActor || IsTransitioning) return false;

	const bool bEnteringCorridor = !bIsInCorridor;
	const FName DestinationTag = bEnteringCorridor
		? FName(TEXT("CorridorEntryPosition"))
		: FName(TEXT("WorkshopStartPosition"));
	TArray<AActor*> Destinations;
	UGameplayStatics::GetAllActorsWithTag(this, DestinationTag, Destinations);
	if (Destinations.IsEmpty())
	{
		UE_LOG(LogTemp, Error,
			TEXT("PlayerBase: porta '%s' sem destino. Adicione a tag '%s' ao ator de posição correspondente."),
			*DoorActor->GetName(), *DestinationTag.ToString());
		return false;
	}

	OnPlayerClosedDoor.Broadcast(DoorActor);
	PositionHistory.Reset();
	PositionLookHistory.Reset();
	CurrentAreaActor = nullptr;
	bPendingCorridorMode = bEnteringCorridor;
	bChangeCorridorModeAfterMove = true;
	if (!bEnteringCorridor && CorridorReticleWidget)
	{
		CorridorReticleWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	MoveToPosition(Destinations[0]->GetActorTransform());
	return true;
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
	if (bInventoryOpen || bTaskListOpen || !Inventory) return;

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

	if (bIgnoreGoBackUntilReleased) return;

	if (bTaskListOpen)
	{
		bIgnoreGoBackUntilReleased = true;
		ToggleTaskList();
		return;
	}
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
	const FRotator LastLookOffset = PositionLookHistory.IsEmpty()
		? FRotator::ZeroRotator
		: PositionLookHistory.Pop();
	CurrentAreaActor = nullptr;
	MoveToPosition(Last);
	PendingLookOffset = LastLookOffset;
	bRestoreLookOffsetOnMoveFinished = true;
}

void APlayerBase::OnGoBackReleased()
{
	bIgnoreGoBackUntilReleased = false;
}

void APlayerBase::MoveToPosition(const FTransform& Target)
{
	bRestoreLookOffsetOnMoveFinished = false;
	PendingLookOffset = FRotator::ZeroRotator;

	CameraFocusTimeline.Stop();
	bIsCameraFocused = false;
	bExitingCameraFocus = false;

	// Preserve the view direction the player currently sees. The camera is attached
	// to the pawn, so cache world rotation before the pawn starts turning.
	StartCameraWorldRotation = Camera->GetComponentRotation();
	LookPitchOffset = 0.0;
	LookYawOffset = 0.0;
	Camera->SetRelativeLocation(DefaultCameraRelativeLocation);

	StartLocation = GetActorLocation();
	StartRotation = GetActorRotation();
	TargetLocation = Target.GetLocation();
	TargetRotation = Target.Rotator();
	++CameraMoveDebugId;
	const float MoveDistance = FVector::Distance(StartLocation, TargetLocation);
	const float TurnAngleDegrees = FMath::RadiansToDegrees(
		StartRotation.Quaternion().AngularDistance(TargetRotation.Quaternion()));
	const float DistanceDuration = MoveDistance / FMath::Max(MoveSpeedUnitsPerSecond, 1.0f);
	const float RotationDuration = TurnAngleDegrees / FMath::Max(MoveTurnDegreesPerSecond, 1.0f);
	const float EffectiveMoveDuration = FMath::Max(MoveLength, FMath::Max(DistanceDuration, RotationDuration));
	const float TimelinePlayRate = MoveLength / EffectiveMoveDuration;
	MoveTimeline.SetPlayRate(TimelinePlayRate);
	UE_LOG(LogTemp, Warning,
		TEXT("[CameraMove %d] START | Area=%s | Distance=%.1f | Turn=%.1fdeg | Duration=%.2fs | PlayRate=%.3f | StartLoc=%s | TargetLoc=%s | StartRot=%s | TargetRot=%s | CameraWorldRot=%s"),
		CameraMoveDebugId,
		CurrentAreaActor ? *CurrentAreaActor->GetName() : TEXT("HistoryOrManual"),
		MoveDistance,
		TurnAngleDegrees,
		EffectiveMoveDuration,
		TimelinePlayRate,
		*StartLocation.ToString(),
		*TargetLocation.ToString(),
		*StartRotation.ToString(),
		*TargetRotation.ToString(),
		*StartCameraWorldRotation.ToString());

	IsTransitioning = true;
	MoveTimeline.PlayFromStart();
}

void APlayerBase::OnMoveUpdate(float Alpha)
{
	const float ClampedAlpha = FMath::Clamp(Alpha, 0.f, 1.f);
	const FRotator InterpolatedRotation = UKismetMathLibrary::RLerp(StartRotation, TargetRotation, ClampedAlpha, true);
	FRotator TargetCameraWorldRotation = TargetRotation;
	if (bRestoreLookOffsetOnMoveFinished)
	{
		TargetCameraWorldRotation += PendingLookOffset;
	}
	const FRotator InterpolatedCameraWorldRotation = UKismetMathLibrary::RLerp(
		StartCameraWorldRotation, TargetCameraWorldRotation, ClampedAlpha, true);

	SetActorLocationAndRotation(
		FMath::Lerp(StartLocation, TargetLocation, ClampedAlpha),
		InterpolatedRotation);
	Camera->SetWorldRotation(InterpolatedCameraWorldRotation);

	const double Z = FMath::Sin(ClampedAlpha * BobFrequency * UE_DOUBLE_PI) * BobAmplitude;
	Camera->SetRelativeLocation(DefaultCameraRelativeLocation + FVector(0.0, 0.0, Z));

	UE_LOG(LogTemp, Warning,
		TEXT("[CameraMove %d] UPDATE | RawAlpha=%.4f | Alpha=%.4f | ActorLoc=%s | ActorRot=%s | InterpRot=%s | CameraRelLoc=%s | CameraWorldRot=%s | InterpCameraWorldRot=%s | BobZ=%.2f"),
		CameraMoveDebugId,
		Alpha,
		ClampedAlpha,
		*GetActorLocation().ToString(),
		*GetActorRotation().ToString(),
		*InterpolatedRotation.ToString(),
		*Camera->GetRelativeLocation().ToString(),
		*Camera->GetComponentRotation().ToString(),
		*InterpolatedCameraWorldRotation.ToString(),
		Z);
}

void APlayerBase::OnMoveFinished()
{
	SetActorLocationAndRotation(TargetLocation, TargetRotation);
	Camera->SetRelativeLocation(DefaultCameraRelativeLocation);
	Camera->SetWorldRotation(TargetRotation);
	if (bRestoreLookOffsetOnMoveFinished)
	{
		LookPitchOffset = PendingLookOffset.Pitch;
		LookYawOffset = PendingLookOffset.Yaw;
		UpdateCameraRotation();
		bRestoreLookOffsetOnMoveFinished = false;
	}
	if (bChangeCorridorModeAfterMove)
	{
		SetCorridorMode(bPendingCorridorMode);
		bChangeCorridorModeAfterMove = false;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[CameraMove %d] FINISHED | ActorLoc=%s | ActorRot=%s | CameraRelLoc=%s | CameraWorldRot=%s"),
		CameraMoveDebugId,
		*GetActorLocation().ToString(),
		*GetActorRotation().ToString(),
		*Camera->GetRelativeLocation().ToString(),
		*Camera->GetComponentRotation().ToString());

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
	InspectionLight->SetVisibility(true);
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

	InspectedLightingChannelStates.Reset();
	TArray<UPrimitiveComponent*> LightingPrimitives;
	TargetObject->GetComponents<UPrimitiveComponent>(LightingPrimitives);
	for (UPrimitiveComponent* Primitive : LightingPrimitives)
	{
		if (!Primitive || !Primitive->IsRegistered()) continue;

		const FLightingChannels PreviousChannels = Primitive->LightingChannels;
		InspectedLightingChannelStates.Add(Primitive, PreviousChannels);
		Primitive->SetLightingChannels(PreviousChannels.bChannel0, true, PreviousChannels.bChannel2);
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
		InspectionLight->SetVisibility(false);
		RestoreInspectionLightingChannels();
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
		RestoreInspectionLightingChannels();

		InspectedObject = nullptr;
		isInspecting = false;
		InspectionLight->SetVisibility(false);
		bInspectDragging = false;

		InspectionAnchor->SetRelativeLocation(FVector(InspectionDistance, 0.0, 0.0));
		InspectionZoomTarget = InspectionDistance;

		ClearInspectionBlur();
	}
}

void APlayerBase::RestoreInspectionLightingChannels()
{
	for (const TPair<TWeakObjectPtr<UPrimitiveComponent>, FLightingChannels>& Entry : InspectedLightingChannelStates)
	{
		if (UPrimitiveComponent* Primitive = Entry.Key.Get())
		{
			const FLightingChannels& Channels = Entry.Value;
			Primitive->SetLightingChannels(Channels.bChannel0, Channels.bChannel1, Channels.bChannel2);
		}
	}
	InspectedLightingChannelStates.Reset();
}

void APlayerBase::UpdateInspectionBlur(float Alpha)
{
	if (!bBlurBackground || !Camera) return;

	FPostProcessSettings& PP = Camera->PostProcessSettings;
	PP.bOverride_DepthOfFieldFocalDistance = true;
	PP.bOverride_DepthOfFieldFstop = true;

	PP.DepthOfFieldFocalDistance = static_cast<float>(InspectionAnchor->GetRelativeLocation().X);

	// Keep a deeper focus range so the inspected object's near and far surfaces stay legible.
	const float EffectiveFStop = FMath::Max(InspectionBlurFStop, 5.6f);
	const float InvFStop = FMath::Lerp(1.f / 32.f, 1.f / EffectiveFStop, Alpha);
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
	if (bTaskListOpen) return;

	if (bInventoryOpen)
	{
		OnInventoryClose();
	}
	if (bTaskListOpen)
	{
		ToggleTaskList();
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

void APlayerBase::ToggleTaskList()
{
	if (!CachedPlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("TaskList: nao abriu; CachedPlayerController esta nulo."));
		return;
	}

	const bool bOpening = !bTaskListOpen;
	TArray<FRepairStep> ChecklistSteps;
	if (bOpening)
	{
		TArray<AActor*> Actors;
		UGameplayStatics::GetAllActorsOfClass(this, AActor::StaticClass(), Actors);
		for (AActor* Actor : Actors)
		{
			if (!Actor) continue;
			URepairableComponent* Repairable = Actor->FindComponentByClass<URepairableComponent>();
			if (!Repairable || Repairable->RepairSteps.Num() == 0) continue;

			const bool bLooksLikeRadioRepair = Repairable->RepairSteps.ContainsByPredicate([](const FRepairStep& Step)
			{
				return Step.StepId.ToString().Contains(TEXT("Screw"))
					|| Step.StepId.ToString().Contains(TEXT("Battery"));
			});
			if (bLooksLikeRadioRepair)
			{
				ChecklistSteps = Repairable->RepairSteps;
				break;
			}
		}
	}

	if (!TaskListWidget)
	{
		TaskListWidget = CreateWidget<UTaskListWidget>(CachedPlayerController, UTaskListWidget::StaticClass());
		if (!TaskListWidget)
		{
			UE_LOG(LogTemp, Error, TEXT("TaskList: CreateWidget falhou."));
			return;
		}
		TaskListWidget->InitializeChecklist(ChecklistSteps);
		TaskListWidget->OnCloseRequested.AddDynamic(this, &APlayerBase::OnTaskListCloseRequested);
		UE_LOG(LogTemp, Warning, TEXT("TaskList: widget criado: %s"), *GetNameSafe(TaskListWidget));
		TaskListWidget->AddToViewport(100);
		UE_LOG(LogTemp, Warning, TEXT("TaskList: widget adicionado ao viewport."));
	}
	else if (bOpening)
	{
		TaskListWidget->InitializeChecklist(ChecklistSteps);
	}

	bTaskListOpen = !bTaskListOpen;
	TaskListWidget->SetVisibility(bTaskListOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	TaskListWidget->SetRenderOpacity(1.0f);
	UE_LOG(LogTemp, Warning, TEXT("TaskList: %s; visibilidade=%s; viewport=%d"),
		bTaskListOpen ? TEXT("aberta") : TEXT("fechada"),
		TaskListWidget->GetVisibility() == ESlateVisibility::Visible ? TEXT("Visible") : TEXT("Collapsed"),
		TaskListWidget->IsInViewport() ? 1 : 0);
	if (bTaskListOpen)
	{
		bClickHeld = false;
	}
}

void APlayerBase::OnTaskListCloseRequested()
{
	if (bTaskListOpen)
	{
		ToggleTaskList();
	}
}

void APlayerBase::ShowPrologueCompleted()
{
	if (bPrologueCompletionStarted) return;
	if (!CachedPlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerBase: nao foi possivel mostrar a conclusao; PlayerController nulo."));
		return;
	}

	PrologueCompletedWidget = CreateWidget<UPrologueCompletedWidget>(CachedPlayerController, UPrologueCompletedWidget::StaticClass());
	if (!PrologueCompletedWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerBase: CreateWidget falhou para a tela de conclusao."));
		return;
	}

	PrologueCompletedWidget->AddToViewport(1000);
	PrologueCompletedWidget->FillViewport();
	PrologueCompletedWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	PrologueCompletedWidget->SetCompletionText(FText::FromString(TEXT("Dia 1 conclu\u00EDdo!")));
	PrologueCompletedWidget->SetFadeProgress(0.0f, 0.0f);

	bPrologueCompletionStarted = true;
	bPrologueFadeActive = true;
	PrologueFadeElapsed = 0.0f;
	bClickHeld = false;
	IsTransitioning = true;
	CachedPlayerController->bShowMouseCursor = false;
	UWidgetBlueprintLibrary::SetInputMode_GameOnly(CachedPlayerController);
	UE_LOG(LogTemp, Warning, TEXT("PlayerBase: tela 'Dia 1 concluido!' adicionada; iniciando fade preto."));
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

	// Define o jogador para que o componente possa validar peças exigidas no inventário.
	Repairable->StartRepair(this);

	// Verifica se pode usar o item equipado e as peças necessárias no inventário.
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

	// Guardamos o componente atual: UpdateRepairProgress pode concluir a última etapa e
	// disparar OnAllRepairsCompleted, cujo callback encerra a interação e limpa o membro.
	URepairableComponent* RepairableBeingUpdated = CurrentRepairableComponent;

	// Atualiza progresso do componente de reparo
	RepairableBeingUpdated->UpdateRepairProgress(DeltaTime);

	// A conclusão pode ter encerrado a interação de forma síncrona dentro da chamada acima.
	if (!bIsActivelyRepairing || CurrentRepairableComponent != RepairableBeingUpdated)
	{
		return;
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

			// Valida a ferramenta e as flags exigidas pelo passo (ex.: bateria Carregada).
			if (!ActiveStep.RequiredToolTag.IsNone()
				|| !ActiveStep.RequiredItemFlag.IsNone()
				|| !ActiveStep.RequiredSelectedItemId.IsNone())
			{
				FInventoryItem Equipped;
				const bool bHasEquippedItem = Inventory->GetEquippedItem(Equipped);
				const bool bHasRequiredTag = ActiveStep.RequiredToolTag.IsNone()
					|| (bHasEquippedItem && Equipped.Data && Equipped.Data->HasTag(ActiveStep.RequiredToolTag));
				const bool bHasRequiredFlag = ActiveStep.RequiredItemFlag.IsNone()
					|| (bHasEquippedItem && Equipped.Flags.Contains(ActiveStep.RequiredItemFlag));
				const bool bHasRequiredSelectedItem = ActiveStep.RequiredSelectedItemId.IsNone()
					|| (bHasEquippedItem && Equipped.Data && Equipped.Data->ItemId == ActiveStep.RequiredSelectedItemId);
				if (!bHasEquippedItem || !bHasRequiredTag || !bHasRequiredFlag || !bHasRequiredSelectedItem)
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

	// Bind delegates
	if (CurrentRepairableComponent)
	{
		CurrentRepairableComponent->OnStepCompleted.AddUniqueDynamic(this, &APlayerBase::OnRepairStepCompleted);
		CurrentRepairableComponent->OnAllRepairsCompleted.AddUniqueDynamic(this, &APlayerBase::OnAllRepairsCompleted);
		RepairableCompletionDelegateSource = CurrentRepairableComponent;
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

		CurrentRepairableComponent->OnStepCompleted.RemoveDynamic(this, &APlayerBase::OnRepairStepCompleted);
		// Keep the completion delegate subscribed: UpdateRepairProgress can broadcast
		// completion immediately after the input release stops the active repair.
	}

	CurrentRepairableComponent = nullptr;

	UE_LOG(LogTemp, Log, TEXT("PlayerBase: Reparo interrompido"));
}

void APlayerBase::OnRepairStepCompleted(const FRepairStep& Step)
{
	UE_LOG(LogTemp, Log, TEXT("PlayerBase: Passo de reparo completado: %s"), *Step.StepId.ToString());
	if (TaskListWidget)
	{
		TaskListWidget->MarkStepCompleted(Step.StepId);
	}

	// O widget continua mostrando o progresso até o próximo passo ser iniciado
	// O próximo passo só inicia quando o jogador clica novamente com a ferramenta certa
}

void APlayerBase::OnAllRepairsCompleted()
{
	if (URepairableComponent* CompletionSource = RepairableCompletionDelegateSource.Get())
	{
		CompletionSource->OnAllRepairsCompleted.RemoveDynamic(this, &APlayerBase::OnAllRepairsCompleted);
	}
	RepairableCompletionDelegateSource.Reset();

	UE_LOG(LogTemp, Warning, TEXT("PlayerBase: TODOS OS REPAROS COMPLETOS em '%s'!"), *GetNameSafe(InspectedObject));

	StopRepairInteraction();
	ShowPrologueCompleted();

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

	if (GoBackAction)
	{
		EIC->BindAction(GoBackAction, ETriggerEvent::Triggered, this, &APlayerBase::OnGoBack);
		EIC->BindAction(GoBackAction, ETriggerEvent::Completed, this, &APlayerBase::OnGoBackReleased);
		EIC->BindAction(GoBackAction, ETriggerEvent::Canceled, this, &APlayerBase::OnGoBackReleased);
	}
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
