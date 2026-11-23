#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Components/TimelineComponent.h"
#include "Engine/EngineTypes.h"
#include "Monsters/MonsterTypes.h"
#include "RepairStep.h"
#include "PlayerBase.generated.h"

class UCameraComponent;
class UCapsuleComponent;
class UPointLightComponent;
class USceneComponent;
class UPrimitiveComponent;
class UInventoryComponent;
class UInventoryWidget;
class UInputMappingContext;
class UInputAction;
class UCurveFloat;
class UUserWidget;
class APlayerController;
class UTextureRenderTarget2D;
class UMaterialInstanceDynamic;
class USceneCaptureComponent2D;
class URepairableComponent;
class UTaskListWidget;
class UPrologueCompletedWidget;
class UCorridorReticleWidget;
class AMonsterBase;
class USoundBase;
class USoundAttenuation;
class UAudioComponent;
struct FInputActionValue;

struct FCorridorCabinetFallState
{
	TWeakObjectPtr<AActor> Actor;
	FVector Pivot = FVector::ZeroVector;
	float FloorZ = 0.0f;
	FVector StartLocation = FVector::ZeroVector;
	FRotator StartRotation = FRotator::ZeroRotator;
	FRotator TargetRotation = FRotator::ZeroRotator;
	float Elapsed = 0.0f;
	bool bMonsterEnteredTriggerRange = false;
	bool bTriggered = false;
};

UCLASS(Blueprintable)
class APlayerBase : public APawn
{
	GENERATED_BODY()

public:
	APlayerBase();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable, Category = "Player")
	void MoveToPosition(const FTransform& Target);

	UFUNCTION(BlueprintCallable, Category = "Player")
	void StartInspection(AActor* TargetObject);

	UFUNCTION(BlueprintCallable, Category = "Player")
	void ExitInspection();

	UFUNCTION(BlueprintCallable, Category = "Player")
	void StartCameraFocus(AActor* FocusActor);
	void StartMonsterReveal(AMonsterBase* Monster);
	void ClosePursuitDoor(AActor* DoorActor, USoundBase* DoorSound);
	void CheckMonsterProximityToCabinets(const FVector& MonsterLocation);

	UFUNCTION(BlueprintCallable, Category = "Player")
	void ExitCameraFocus();

	UFUNCTION(BlueprintCallable, Category = "Player")
	void ApplyLookDelta(double Delta);

	UFUNCTION(BlueprintCallable, Category = "Player")
	void ApplyLookPitchDelta(double Delta);

	UFUNCTION(BlueprintCallable, Category = "Monsters|Noise")
	void ReportGameplayNoise(FName NoiseTagName, float Loudness = 1.0f, float HearRange = 2000.0f);

	void PlayPickupSound(USoundBase* SoundOverride, const FVector& Location);

	bool IsClickHeld() const { return bClickHeld; }
    bool IsInCorridor() const { return bIsInCorridor; }
	void SetWorkingOnPart(bool bWorking) { bWorkingOnPart = bWorking; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCapsuleComponent> CorridorCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> InspectionAnchor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> InspectionLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInventoryComponent> Inventory;

	UPROPERTY(BlueprintReadWrite, Category = "Estado")
	bool IsTransitioning = false;

	UPROPERTY(BlueprintReadWrite, Category = "Estado")
	bool isInspecting = false;

	UPROPERTY(BlueprintReadWrite, Category = "Estado")
	bool bIsCameraFocused = false;

	UPROPERTY(BlueprintReadOnly, Category = "Estado")
	bool bInventoryOpen = false;

	UPROPERTY(BlueprintReadWrite, Category = "Estado")
	TObjectPtr<AActor> CurrentAreaActor;

	UPROPERTY(BlueprintReadWrite, Category = "Estado")
	TObjectPtr<AActor> InspectedObject;

	UPROPERTY(BlueprintReadWrite, Category = "Estado")
	TArray<FTransform> PositionHistory;

	UPROPERTY(BlueprintReadWrite, Category = "Estado")
	double LookYawOffset = 0.0;

	UPROPERTY(BlueprintReadWrite, Category = "Estado")
	double LookPitchOffset = 0.0;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ClickAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> GoBackAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> PauseAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> InventoryAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> InventoryScrollAction;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> PauseMenuClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UInventoryWidget> InventoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Inventário")
	double InventoryScrollInterval = 0.12;

	UPROPERTY(EditAnywhere, Category = "Look")
	double EdgeLookMargin = 80.0;

	UPROPERTY(EditAnywhere, Category = "Look")
	double EdgePitchSpeed = 40.0;

	UPROPERTY(EditAnywhere, Category = "Look")
	double InspectRotateSpeed = 0.4;

	UPROPERTY(EditAnywhere, Category = "Look")
	double LookRotationSpeed = 80.0;

	UPROPERTY(EditAnywhere, Category = "Look")
	double PitchMin = -40.0;

	UPROPERTY(EditAnywhere, Category = "Look")
	double PitchMax = 60.0;

	UPROPERTY(EditAnywhere, Category = "Inspeção")
	double InspectionDistance = 120.0;

	UPROPERTY(EditAnywhere, Category = "Inspeção|Zoom")
	double InspectionZoomMin = 50.0;

	UPROPERTY(EditAnywhere, Category = "Inspeção|Zoom")
	double InspectionZoomMax = 200.0;

	// Distancia percorrida por tick de scroll (use valor negativo para inverter o sentido)
	UPROPERTY(EditAnywhere, Category = "Inspeção|Zoom")
	double InspectionZoomStep = 15.0;

	UPROPERTY(EditAnywhere, Category = "Inspeção|Zoom")
	double InspectionZoomSpeed = 10.0;

	UPROPERTY(EditAnywhere, Category = "Inspeção")
	bool bAutoFitInspectedObject = true;

	UPROPERTY(EditAnywhere, Category = "Inspeção", meta = (EditCondition = "bAutoFitInspectedObject"))
	double InspectionTargetSize = 40.0;

	UPROPERTY(EditAnywhere, Category = "Inspeção")
	bool bBlurBackground = true;

	UPROPERTY()
	TObjectPtr<URepairableComponent> CurrentRepairableComponent;

	UPROPERTY(EditAnywhere, Category = "Reparo")
	bool bEnableNewRepairSystem = true;

	bool bIsActivelyRepairing = false;

	UPROPERTY(EditAnywhere, Category = "Inspeção", meta = (EditCondition = "bBlurBackground", ClampMin = "0.05"))
	float InspectionBlurFStop = 5.6f;

	UPROPERTY(EditAnywhere, Category = "Inspeção")
	bool bDisableShadowDuringInspection = true;

	UPROPERTY(EditDefaultsOnly, Category = "Qualidade", meta = (ClampMin = "0", ClampMax = "4"))
	int32 MinimumScalabilityLevel = 3;

	UPROPERTY(EditAnywhere, Category = "Bob")
	double BobFrequency = 2.0;

	UPROPERTY(EditAnywhere, Category = "Bob")
	double BobAmplitude = 40.0;

	UPROPERTY(EditAnywhere, Category = "Corridor|Controls", meta = (ClampMin = "0.0"))
	float CorridorMoveSpeed = 300.0f;

	UPROPERTY(EditAnywhere, Category = "Corridor|Controls", meta = (ClampMin = "0.001"))
	float CorridorMouseSensitivity = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Corridor|Controls", meta = (ClampMin = "0.1"))
	float CorridorLookSmoothSpeed = 18.0f;

	UPROPERTY(EditAnywhere, Category = "Corridor|Controls", meta = (ClampMin = "0.1"))
	float CorridorMoveAcceleration = 9.0f;

	UPROPERTY(EditAnywhere, Category = "Corridor|Controls", meta = (ClampMin = "0.1"))
	float CorridorMoveBraking = 12.0f;

	UPROPERTY(EditAnywhere, Category = "Corridor|Controls", meta = (ClampMin = "0.0"))
	float CorridorCameraHeightDrop = 25.0f;

	UPROPERTY(EditAnywhere, Category = "Corridor|Controls")
	bool bInvertCorridorMouseY = false;

	UPROPERTY(EditAnywhere, Category = "Corridor|Controls", meta = (ClampMin = "0.0", ClampMax = "89.0"))
	float CorridorPitchLimit = 85.0f;

	UPROPERTY(EditAnywhere, Category = "Corridor|Controls", meta = (ClampMin = "0.0"))
	float CorridorSprintSpeed = 520.0f;

	UPROPERTY(EditAnywhere, Category = "Corridor|Controls", meta = (ClampMin = "0.0"))
	float CorridorCrouchSpeed = 150.0f;

	UPROPERTY(EditAnywhere, Category = "Corridor|Controls", meta = (ClampMin = "0.0"))
	float CorridorCrouchCameraDrop = 35.0f;

	UPROPERTY(EditAnywhere, Category = "Corridor|Controls", meta = (ClampMin = "1.0"))
	float CorridorCrouchCapsuleHalfHeight = 45.0f;

	UPROPERTY(EditAnywhere, Category = "Corridor|Controls", meta = (ClampMin = "0.05"))
	float CorridorFootstepNoiseInterval = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Audio|Workshop")
	TObjectPtr<USoundBase> WorkshopMovementSound;

	UPROPERTY(EditDefaultsOnly, Category = "Audio|Pickups")
	TObjectPtr<USoundBase> DefaultPickupSound;

	UPROPERTY(EditDefaultsOnly, Category = "Audio|Pickups", meta = (ClampMin = "0.0", ClampMax = "0.15"))
	float PickupSoundPitchVariation = 0.04f;

	UPROPERTY(EditDefaultsOnly, Category = "Audio|Corridor")
	TObjectPtr<USoundBase> CorridorMovementSound;

	UPROPERTY(EditAnywhere, Category = "Audio|Corridor", meta = (ClampMin = "1.0", ClampMax = "2.0"))
	float CorridorRunSoundPitch = 1.12f;

	UPROPERTY(EditAnywhere, Category = "Audio|Corridor", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float CorridorCrouchSoundVolume = 0.45f;

	UPROPERTY(EditAnywhere, Category = "Audio|Corridor", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float CorridorCrouchSoundPitch = 0.9f;

	UPROPERTY(EditAnywhere, Category = "Audio|Corridor", meta = (ClampMin = "200.0", ClampMax = "20000.0"))
	float CorridorCrouchLowPassFrequency = 1600.0f;

	UPROPERTY(EditAnywhere, Category = "Corridor|Encontro", meta = (ClampMin = "1.0"))
	float MonsterRevealDuration = 1.25f;

	UPROPERTY(EditAnywhere, Category = "Corridor|Perigos", meta = (ClampMin = "1.0"))
	float CabinetFallTriggerDistance = 220.0f;

	UPROPERTY(EditAnywhere, Category = "Corridor|Perigos", meta = (ClampMin = "0.0"))
	float CabinetFallClearanceDistance = 120.0f;

	UPROPERTY(EditAnywhere, Category = "Corridor|Perigos", meta = (ClampMin = "1.0"))
	float CabinetFallDuration = 0.65f;

	UPROPERTY(EditAnywhere, Category = "Corridor|Perigos", meta = (ClampMin = "1.0", ClampMax = "89.0"))
	float CabinetFallAngle = 82.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Corridor|Perigos|Audio")
	TObjectPtr<USoundBase> CabinetFallSound;

	UPROPERTY(EditDefaultsOnly, Category = "Corridor|Perigos|Audio")
	TObjectPtr<USoundAttenuation> CabinetFallAttenuation;

public:
	// Monster Reaction Delegates
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerUsedSecurityCamera, AActor*, CameraActor);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerClosedDoor, AActor*, DoorActor);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerMadeNoise, const FNoiseEvent&, NoiseEvent);

	UPROPERTY(BlueprintAssignable, Category = "Monsters")
	FOnPlayerUsedSecurityCamera OnPlayerUsedSecurityCamera;

	UPROPERTY(BlueprintAssignable, Category = "Monsters")
	FOnPlayerClosedDoor OnPlayerClosedDoor;

	UPROPERTY(BlueprintAssignable, Category = "Monsters")
	FOnPlayerMadeNoise OnPlayerMadeNoise;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Timelines")
	TObjectPtr<UCurveFloat> MoveCurve;

	UPROPERTY(EditDefaultsOnly, Category = "Timelines")
	TObjectPtr<UCurveFloat> CameraFocusCurve;

	UPROPERTY(EditDefaultsOnly, Category = "Timelines")
	TObjectPtr<UCurveFloat> InspectCurve;

	UPROPERTY(EditDefaultsOnly, Category = "Timelines")
	float MoveLength = 0.65f;

	UPROPERTY(EditDefaultsOnly, Category = "Timelines|Movimento", meta = (ClampMin = "1.0"))
	float MoveSpeedUnitsPerSecond = 900.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Timelines|Movimento", meta = (ClampMin = "1.0"))
	float MoveTurnDegreesPerSecond = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Timelines")
	float CameraFocusLength = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "Timelines")
	float InspectLength = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Inspection|Lighting", meta = (ClampMin = "0.0"))
	float InspectionLightIntensity = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Inspection|Lighting", meta = (ClampMin = "0.0"))
	float InspectionLightRadius = 180.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Inspection|Lighting", meta = (ClampMin = "0.0"))
	float InspectionLightSourceRadius = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Inspection|Lighting")
	FLinearColor InspectionLightColor = FLinearColor::White;

	UPROPERTY()
	TObjectPtr<APlayerController> CachedPlayerController;

	UPROPERTY()
	TObjectPtr<UInventoryWidget> InventoryWidget;

	UPROPERTY()
	TObjectPtr<UTaskListWidget> TaskListWidget;

	UPROPERTY()
	TObjectPtr<UPrologueCompletedWidget> PrologueCompletedWidget;

	TWeakObjectPtr<URepairableComponent> RepairableCompletionDelegateSource;

	UPROPERTY()
	TObjectPtr<UCorridorReticleWidget> CorridorReticleWidget;

	bool bTaskListOpen = false;
	bool bIgnoreGoBackUntilReleased = false;
	bool bPrologueCompletionStarted = false;
	bool bPrologueFadeActive = false;
	float PrologueFadeElapsed = 0.0f;
	float PrologueFadeDuration = 2.0f;
	float PrologueTextFadeDelay = 0.8f;
	float PrologueTextFadeDuration = 0.6f;

	double LastInventoryScrollTime = -1000.0;

	FVector DefaultCameraRelativeLocation = FVector::ZeroVector;
	FRotator DefaultCameraRelativeRotation = FRotator::ZeroRotator;

	FVector StartLocation = FVector::ZeroVector;
	FVector TargetLocation = FVector::ZeroVector;
	FRotator StartRotation = FRotator::ZeroRotator;
	FRotator TargetRotation = FRotator::ZeroRotator;
	FRotator StartCameraWorldRotation = FRotator::ZeroRotator;
	int32 CameraMoveDebugId = 0;
	FRotator PendingLookOffset = FRotator::ZeroRotator;
	bool bRestoreLookOffsetOnMoveFinished = false;
	TArray<FRotator> PositionLookHistory;
	bool bIsInCorridor = false;
	bool bChangeCorridorModeAfterMove = false;
	bool bPendingCorridorMode = false;
	bool bIgnoreNextCorridorMouseDelta = false;
	bool bIsCorridorCrouched = false;
	float CorridorFootstepNoiseTimer = 0.0f;
	UPROPERTY()
	TObjectPtr<UAudioComponent> WorkshopMovementAudio;
	UPROPERTY()
	TObjectPtr<UAudioComponent> CorridorMovementAudio;
	FVector CorridorCollisionRelativeLocation = FVector(0.0f, 0.0f, -69.0f);
	FVector CorridorStandCameraRelativeLocation = FVector::ZeroVector;
	FVector CorridorStandCollisionRelativeLocation = FVector(0.0f, 0.0f, -69.0f);
	FVector CorridorVelocity = FVector::ZeroVector;
	float CorridorTargetYawOffset = 0.0f;
	float CorridorTargetPitchOffset = 0.0f;
	float CorridorStandCapsuleHalfHeight = 65.0f;
	float CorridorStandCapsuleRadius = 26.0f;
	TArray<FCorridorCabinetFallState> CorridorCabinets;
	TWeakObjectPtr<AMonsterBase> PendingCorridorRevealMonster;
	TWeakObjectPtr<AActor> PursuitDoorToClose;
	FVector PursuitDoorPivot = FVector::ZeroVector;
	FVector PursuitDoorStartLocation = FVector::ZeroVector;
	FVector PursuitDoorTargetLocation = FVector::ZeroVector;
	FRotator PursuitDoorStartRotation = FRotator::ZeroRotator;
	FRotator PursuitDoorTargetRotation = FRotator::ZeroRotator;
	float PursuitDoorCloseElapsed = 0.0f;
	float PursuitDoorCloseDuration = 0.7f;
	bool bPursuitDoorClosing = false;

	FVector FocusStartLocation = FVector::ZeroVector;
	FVector FocusTargetLocation = FVector::ZeroVector;
	FRotator FocusStartRotation = FRotator::ZeroRotator;
	FRotator FocusTargetRotation = FRotator::ZeroRotator;

	FVector InspectStartLocation = FVector::ZeroVector;
	FRotator InspectStartRotation = FRotator::ZeroRotator;
	FRotator InspectExitRotation = FRotator::ZeroRotator;
	FVector InspectStartScale = FVector::OneVector;
	FVector InspectStartCenter = FVector::ZeroVector;
	FVector InspectLocalCenterOffset = FVector::ZeroVector;
	double InspectScaleRatio = 1.0;

	bool bClickHeld = false;
	double InspectionZoomTarget = 120.0;
	bool bWorkingOnPart = false;
	bool bInspectDragging = false;
	TMap<TWeakObjectPtr<UPrimitiveComponent>, bool> InspectedShadowStates;
	TMap<TWeakObjectPtr<UPrimitiveComponent>, FLightingChannels> InspectedLightingChannelStates;
	FVector2D LastInspectMousePos = FVector2D::ZeroVector;

	bool bExitingInspection = false;
	bool bExitingCameraFocus = false;
	bool bInspectedObjectHadCollision = true;

	FTimeline MoveTimeline;
	FTimeline CameraFocusTimeline;
	FTimeline InspectTimeline;

	void InitTimeline(FTimeline& Timeline, TObjectPtr<UCurveFloat>& Curve, float Length,
		float DefaultEndTime, float DefaultEndValue, bool bSmooth, FName UpdateFn, FName FinishedFn);
	void RestoreInspectionLightingChannels();

	UFUNCTION()
	void OnMoveUpdate(float Alpha);

	UFUNCTION()
	void OnMoveFinished();

	UFUNCTION()
	void OnCameraFocusUpdate(float Alpha);

	UFUNCTION()
	void OnCameraFocusFinished();

	UFUNCTION()
	void OnInspectUpdate(float Alpha);

	UFUNCTION()
	void OnInspectFinished();

	void OnClick();
	void TryUseItemOnPart();
	void SetPartsInteractable(AActor* Parent, bool bInteractable);
	void OnClickReleased();

	// --- Novo Sistema de Reparo ---
	void TryUseItemOnRepairable();
	void UpdateActiveRepair(float DeltaTime);
	void StartRepairInteraction(URepairableComponent* Repairable);
	void StopRepairInteraction();
	UFUNCTION()
	void OnRepairStepCompleted(const FRepairStep& Step);
	UFUNCTION()
	void OnAllRepairsCompleted();
	void OnInventoryOpen();
	void OnInventoryClose();
	void OnInventoryScroll(const FInputActionValue& Value);
	void OnGoBack();
	void OnGoBackReleased();
	void OnLook(const FInputActionValue& Value);
	void OnPause();
	void UpdateCorridorControls(float DeltaTime);
	void UpdateWorkshopMovementAudio(bool bMoving);
	void UpdateCorridorMovementAudio(bool bMoving, bool bSprinting, bool bCrouching);
	void PauseMovementAudio();
	void UpdateCorridorHazards(float DeltaTime);
	void UpdatePursuitDoor(float DeltaTime);
	void SetCorridorCrouched(bool bCrouched);
	void UpdateCorridorReticle();
	bool IsCorridorInteractable(AActor* Actor) const;
	void SetCorridorMode(bool bEnable);
	bool TryUseCorridorDoor(AActor* DoorActor);

	void UpdateCameraRotation();
	void EnforceMinimumQuality();

	void PlaceInspectedObject(const FVector& CenterWorld, double ScaleRatio);
	void UpdateInspectionRotation();
	void UpdateInspectionZoom(float DeltaTime);
	bool IsMouseNearScreenEdge();
	void UpdateInspectionBlur(float Alpha);
	void ClearInspectionBlur();

	// Preview 3D system
	void RotatePreviews(float DeltaTime);
	void ToggleTaskList();
	UFUNCTION()
	void OnTaskListCloseRequested();
	void ShowPrologueCompleted();

private:
	// Preview 3D system
	float PreviewRotationSpeed = 30.0f;
	int32 MaxPreviewSlots = 20;
	TArray<TObjectPtr<AActor>> PreviewActors;
	TArray<TObjectPtr<USceneCaptureComponent2D>> PreviewCaptures;
	TArray<TObjectPtr<UTextureRenderTarget2D>> PreviewRenderTargets;
	TArray<TObjectPtr<UMaterialInstanceDynamic>> PreviewMaterials;
};
