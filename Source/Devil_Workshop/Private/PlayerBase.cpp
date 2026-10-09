#include "PlayerBase.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/Widget.h"
#include "Curves/CurveFloat.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/LocalPlayer.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

// Cria uma curva com 2 pontos: (0, 0) e (EndTime, EndValue).
// bSmooth = true  -> interpolação Cubic com tangentes Auto (começa e termina suave)
// bSmooth = false -> interpolação linear
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
	InspectionAnchor->SetRelativeLocation(FVector(80.0, 0.0, 0.0));
	InspectionAnchor->SetRelativeRotation(FRotator::ZeroRotator);
}

void APlayerBase::InitTimeline(FTimeline& Timeline, TObjectPtr<UCurveFloat>& Curve, float Length,
	float DefaultEndTime, float DefaultEndValue, bool bSmooth, FName UpdateFn, FName FinishedFn)
{
	// Sem curva atribuída no BP: cria a curva padrão
	if (!Curve)
	{
		Curve = MakeDefaultCurve(this, DefaultEndTime, DefaultEndValue, bSmooth);
	}

	FOnTimelineFloat Update;
	Update.BindUFunction(this, UpdateFn);
	Timeline.AddInterpFloat(Curve, Update);

	// Usa o Length exato da timeline (igual ao BP), e não o último ponto da curva
	Timeline.SetTimelineLengthMode(ETimelineLengthMode::TL_TimelineLength);
	Timeline.SetTimelineLength(Length);

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

	// Move:        (0,0) -> (0.57, 1.03), suave, length 0.65
	InitTimeline(MoveTimeline, MoveCurve, MoveLength, 0.57f, 1.03f, true, "OnMoveUpdate", "OnMoveFinished");
	// CameraFocus: (0,0) -> (0.35, 1),    linear, length 0.35
	InitTimeline(CameraFocusTimeline, CameraFocusCurve, CameraFocusLength, 0.35f, 1.0f, false, "OnCameraFocusUpdate", NAME_None);
	// Inspect:     (0,0) -> (0.5, 1),     suave,  length 0.5
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
				UE_LOG(LogTemp, Warning, TEXT("PlayerBase: DefaultMappingContext não foi atribuído no Class Defaults."));
			}
		}

		PC->bShowMouseCursor = true;
		UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC, false);
		UGameplayStatics::SetGamePaused(this, false);
	}
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

	if (ClickAction) { EIC->BindAction(ClickAction, ETriggerEvent::Started, this, &APlayerBase::OnClick); }
	else { UE_LOG(LogTemp, Warning, TEXT("PlayerBase: ClickAction não atribuída.")); }

	if (GoBackAction) { EIC->BindAction(GoBackAction, ETriggerEvent::Triggered, this, &APlayerBase::OnGoBack); }
	else { UE_LOG(LogTemp, Warning, TEXT("PlayerBase: GoBackAction não atribuída.")); }

	if (LookAction) { EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerBase::OnLook); }
	else { UE_LOG(LogTemp, Warning, TEXT("PlayerBase: LookAction não atribuída.")); }

	if (PauseAction) { EIC->BindAction(PauseAction, ETriggerEvent::Started, this, &APlayerBase::OnPause); }
	else { UE_LOG(LogTemp, Warning, TEXT("PlayerBase: PauseAction não atribuída.")); }
}

// ===================== TICK =====================
void APlayerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	MoveTimeline.TickTimeline(DeltaTime);
	CameraFocusTimeline.TickTimeline(DeltaTime);
	InspectTimeline.TickTimeline(DeltaTime);

	if (!CachedPlayerController) return;

	if (!IsTransitioning && !isInspecting)
	{
		// Olhar pelas bordas da tela
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
	else if (isInspecting && !IsTransitioning &&
		CachedPlayerController->IsInputKeyDown(EKeys::LeftMouseButton))
	{
		// Girar o objeto inspecionado com o mouse
		float DX = 0.f;
		float DY = 0.f;
		CachedPlayerController->GetInputMouseDelta(DX, DY);

		if (InspectedObject)
		{
			InspectedObject->AddActorWorldRotation(
				UKismetMathLibrary::RotatorFromAxisAndAngle(Camera->GetUpVector(), DX * InspectRotateSpeed * -1.0));
			InspectedObject->AddActorWorldRotation(
				UKismetMathLibrary::RotatorFromAxisAndAngle(Camera->GetRightVector(), DY * InspectRotateSpeed));
		}
	}
}

// ===================== LOOK =====================
void APlayerBase::UpdateCameraRotation()
{
	// Pitch = LookPitchOffset, Yaw = LookYawOffset, Roll = 0 (igual ao K2_SetRelativeRotation do BP)
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
	if (isInspecting || IsTransitioning) return;

	const double Dt = GetWorld()->GetDeltaSeconds();
	LookYawOffset = FMath::Clamp(LookYawOffset + static_cast<double>(Value.Get<float>()) * Dt * 60.0, -YawLimit, YawLimit);
	UpdateCameraRotation();
}

// ===================== CLICK =====================
void APlayerBase::OnClick()
{
	if (isInspecting || !CachedPlayerController) return;

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
		// Mantido igual ao grafo original: este ramo só roda quando o trace NÃO acerta nada,
		// então HitActor é nulo e ele nunca dispara (o mesmo acontece no Blueprint).
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

	// Head bob: Z = Sin(Alpha * BobFrequency * Pi) * BobAmplitude (igual ao grafo)
	const double Z = FMath::Sin(Alpha * BobFrequency * UE_DOUBLE_PI) * BobAmplitude;
	Camera->SetRelativeLocation(FVector(0.0, 0.0, Z));
}

void APlayerBase::OnMoveFinished()
{
	IsTransitioning = false;
}

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
	isInspecting = true;
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

void APlayerBase::OnInspectFinished()
{
	IsTransitioning = false;
}

// ===================== PAUSE =====================
void APlayerBase::OnPause()
{
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