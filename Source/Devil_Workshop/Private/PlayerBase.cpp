#include "PlayerBase.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Curves/CurveFloat.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/LocalPlayer.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

APlayerBase::APlayerBase()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Root);
	InspectionAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("InspectionAnchor"));
	InspectionAnchor->SetupAttachment(Camera);
}

void APlayerBase::InitTimeline(FTimeline& Timeline, TObjectPtr<UCurveFloat>& Curve, FName UpdateFn, FName FinishedFn)
{
	if (!Curve)
	{
		Curve = NewObject<UCurveFloat>(this);
		Curve->FloatCurve.AddKey(0.f, 0.f);
		Curve->FloatCurve.AddKey(DefaultTimelineDuration, 1.f);
	}
	FOnTimelineFloat Update;
	Update.BindUFunction(this, UpdateFn);
	Timeline.AddInterpFloat(Curve, Update);
	Timeline.SetTimelineLengthMode(ETimelineLengthMode::TL_LastKeyFrame);

	if (FinishedFn != NAME_None)
	{
		FOnTimelineEvent Finished;
		Finished.BindUFunction(this, FinishedFn);
		Timeline.SetTimelineFinishedFunc(Finished);
	}
}

void APlayerBase::BeginPlay()
{
	Super::BeginPlay();

	DefaultCameraRelativeLocation = Camera->GetRelativeLocation();
	DefaultCameraRelativeRotation = Camera->GetRelativeRotation();

	InitTimeline(MoveTimeline, MoveCurve, "OnMoveUpdate", "OnMoveFinished");
	InitTimeline(CameraFocusTimeline, CameraFocusCurve, "OnCameraFocusUpdate", NAME_None);
	InitTimeline(InspectTimeline, InspectCurve, "OnInspectUpdate", "OnInspectFinished");

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		CachedPlayerController = PC;
		if (auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
		PC->bShowMouseCursor = true;
		UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC, false);
		UGameplayStatics::SetGamePaused(this, false);
	}
}

void APlayerBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (auto* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EIC->BindAction(ClickAction, ETriggerEvent::Started, this, &APlayerBase::OnClick);
		EIC->BindAction(GoBackAction, ETriggerEvent::Triggered, this, &APlayerBase::OnGoBack);
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerBase::OnLook);
		EIC->BindAction(PauseAction, ETriggerEvent::Started, this, &APlayerBase::OnPause);
	}
}

// ===================== TICK =====================
void APlayerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	MoveTimeline.TickTimeline(DeltaTime);
	CameraFocusTimeline.TickTimeline(DeltaTime);
	InspectTimeline.TickTimeline(DeltaTime);

	if (!CachedPlayerController) return;

	if (!IsTransitioning && !bIsInspecting)
	{
		// Look pelas bordas da tela
		float MouseX, MouseY; int32 SizeX, SizeY;
		CachedPlayerController->GetMousePosition(MouseX, MouseY);
		CachedPlayerController->GetViewportSize(SizeX, SizeY);

		if (MouseX < EdgeLookMargin)               ApplyLookDelta(-EdgeLookSpeed * DeltaTime);
		else if (MouseX > SizeX - EdgeLookMargin)  ApplyLookDelta(EdgeLookSpeed * DeltaTime);

		if (MouseY < EdgeLookMargin)               ApplyLookPitchDelta(DeltaTime * EdgeLookSpeed);
		else if (MouseY > SizeY - EdgeLookMargin)  ApplyLookPitchDelta(-DeltaTime * EdgeLookSpeed);
	}
	else if (bIsInspecting && !IsTransitioning &&
		CachedPlayerController->IsInputKeyDown(EKeys::LeftMouseButton))
	{
		// Rotacionar o objeto inspecionado com o mouse
		float DX, DY;
		CachedPlayerController->GetInputMouseDelta(DX, DY);
		if (InspectedObject)
		{
			InspectedObject->AddActorWorldRotation(
				UKismetMathLibrary::RotatorFromAxisAndAngle(Camera->GetUpVector(), -DX * InspectRotateSpeed));
			InspectedObject->AddActorWorldRotation(
				UKismetMathLibrary::RotatorFromAxisAndAngle(Camera->GetRightVector(), DY * InspectRotateSpeed));
		}
	}
}

// ===================== LOOK =====================
void APlayerBase::UpdateCameraRotation()
{
	Camera->SetRelativeRotation(FRotator(LookPitchOffset, LookYawOffset, 0.0));
}

// ATENÇÃO: reconstruí estas duas funções por dedução (não estavam no grafo que você enviou).
void APlayerBase::ApplyLookDelta(double Delta)
{
	LookYawOffset = FMath::Clamp(LookYawOffset + Delta, -30.0, 30.0);
	UpdateCameraRotation();
}
void APlayerBase::ApplyLookPitchDelta(double Delta)
{
	LookPitchOffset = FMath::Clamp(LookPitchOffset + Delta, -30.0, 30.0);
	UpdateCameraRotation();
}

void APlayerBase::OnLook(const FInputActionValue& Value)
{
	if (bIsInspecting || IsTransitioning) return;
	const double Dt = GetWorld()->GetDeltaSeconds();
	LookYawOffset = FMath::Clamp(LookYawOffset + Value.Get<float>() * Dt * 60.0, -30.0, 30.0);
	UpdateCameraRotation();
}

// ===================== CLICK =====================
void APlayerBase::OnClick()
{
	if (bIsInspecting || !CachedPlayerController) return;

	FHitResult Hit;
	const bool bHit = CachedPlayerController->GetHitResultUnderCursorByChannel(
		UEngineTypes::ConvertToTraceType(ECC_Visibility), true, Hit);

	AActor* HitActor = Hit.GetActor();

	if (bHit)
	{
		if (HitActor && HitActor->ActorHasTag("Inspectable"))
		{
			StartInspection(HitActor);
		}
		else if (HitActor && HitActor->ActorHasTag("Clickable"))
		{
			if (HitActor != CurrentAreaActor)
			{
				CurrentAreaActor = HitActor;
				PositionHistory.Add(GetActorTransform());
				MoveToPosition(HitActor->GetActorTransform());
			}
		}
	}
	else if (HitActor && HitActor->ActorHasTag("CameraFocus"))
	{
		// Mesma lógica do grafo (ver observação sobre este ramo abaixo)
		FocusStartLocation = Camera->GetRelativeLocation();
		FocusStartRotation = Camera->GetRelativeRotation();
		const FTransform Rel = HitActor->GetActorTransform().GetRelativeTransform(GetActorTransform());
		FocusTargetLocation = Rel.GetLocation();
		FocusTargetRotation = Rel.Rotator();
		bIsCameraFocused = true;
		CameraFocusTimeline.PlayFromStart();
	}
}

// ===================== GO BACK =====================
void APlayerBase::OnGoBack()
{
	if (IsTransitioning || PositionHistory.Num() <= 0) return;

	const FTransform Last = PositionHistory.Last();
	PositionHistory.RemoveAt(PositionHistory.Num() - 1);
	CurrentAreaActor = nullptr;
	MoveToPosition(Last);
}

// ===================== MOVE =====================
void APlayerBase::MoveToPosition(const FTransform& Target)
{
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
	SetActorLocationAndRotation(
		FMath::Lerp(StartLocation, TargetLocation, Alpha),
		UKismetMathLibrary::RLerp(StartRotation, TargetRotation, Alpha, false));

	// Head bob (mesmo cálculo do grafo)
	const double Z = FMath::Sin(Alpha * BobFrequency * PI) * BobAmplitude;
	Camera->SetRelativeLocation(FVector(0.0, 0.0, Z));
}

void APlayerBase::OnMoveFinished() { IsTransitioning = false; }

void APlayerBase::OnCameraFocusUpdate(float Alpha)
{
	Camera->SetRelativeLocationAndRotation(
		FMath::Lerp(FocusStartLocation, FocusTargetLocation, Alpha),
		UKismetMathLibrary::RLerp(FocusStartRotation, FocusTargetRotation, Alpha, true));
}

// ===================== INSPEÇÃO =====================
void APlayerBase::StartInspection(AActor* TargetObject)
{
	if (!TargetObject) return;
	InspectedObject = TargetObject;
	bIsInspecting = true;
	IsTransitioning = true;
	InspectStartLocation = TargetObject->GetActorLocation();
	InspectStartRotation = TargetObject->GetActorRotation();
	TargetObject->SetActorEnableCollision(false);
	InspectTimeline.PlayFromStart();
}

void APlayerBase::OnInspectUpdate(float Alpha)
{
	if (InspectedObject)
	{
		InspectedObject->SetActorLocation(
			FMath::Lerp(InspectStartLocation, InspectionAnchor->GetComponentLocation(), Alpha));
	}
}

void APlayerBase::OnInspectFinished() { IsTransitioning = false; }

// ===================== PAUSE =====================
void APlayerBase::OnPause()
{
	if (!CachedPlayerController) return;
	if (UUserWidget* W = CreateWidget<UUserWidget>(CachedPlayerController, PauseMenuClass))
	{
		W->AddToViewport();
	}
	CachedPlayerController->bShowMouseCursor = true;
	UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(CachedPlayerController, nullptr, EMouseLockMode::DoNotLock, false);
	UGameplayStatics::SetGamePaused(this, true);
}