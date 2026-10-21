#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Components/TimelineComponent.h"
#include "PlayerBase.generated.h"

class UCameraComponent;
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
struct FInputActionValue;

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

	UFUNCTION(BlueprintCallable, Category = "Player")
	void ExitCameraFocus();

	UFUNCTION(BlueprintCallable, Category = "Player")
	void ApplyLookDelta(double Delta);

	UFUNCTION(BlueprintCallable, Category = "Player")
	void ApplyLookPitchDelta(double Delta);

	bool IsClickHeld() const { return bClickHeld; }
	void SetWorkingOnPart(bool bWorking) { bWorkingOnPart = bWorking; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> InspectionAnchor;

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
	double EdgeLookSpeed = 40.0;

	UPROPERTY(EditAnywhere, Category = "Look")
	double InspectRotateSpeed = 0.4;

	UPROPERTY(EditAnywhere, Category = "Look")
	double YawLimit = 30.0;

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

	UPROPERTY(EditAnywhere, Category = "Inspeção", meta = (EditCondition = "bBlurBackground", ClampMin = "0.05"))
	float InspectionBlurFStop = 0.3f;

	UPROPERTY(EditAnywhere, Category = "Inspeção")
	bool bDisableShadowDuringInspection = true;

	UPROPERTY(EditDefaultsOnly, Category = "Qualidade", meta = (ClampMin = "0", ClampMax = "4"))
	int32 MinimumScalabilityLevel = 3;

	UPROPERTY(EditAnywhere, Category = "Bob")
	double BobFrequency = 2.0;

	UPROPERTY(EditAnywhere, Category = "Bob")
	double BobAmplitude = 40.0;

	UPROPERTY(EditDefaultsOnly, Category = "Timelines")
	TObjectPtr<UCurveFloat> MoveCurve;

	UPROPERTY(EditDefaultsOnly, Category = "Timelines")
	TObjectPtr<UCurveFloat> CameraFocusCurve;

	UPROPERTY(EditDefaultsOnly, Category = "Timelines")
	TObjectPtr<UCurveFloat> InspectCurve;

	UPROPERTY(EditDefaultsOnly, Category = "Timelines")
	float MoveLength = 0.65f;

	UPROPERTY(EditDefaultsOnly, Category = "Timelines")
	float CameraFocusLength = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "Timelines")
	float InspectLength = 0.5f;

	UPROPERTY()
	TObjectPtr<APlayerController> CachedPlayerController;

	UPROPERTY()
	TObjectPtr<UInventoryWidget> InventoryWidget;

	double LastInventoryScrollTime = -1000.0;

	FVector DefaultCameraRelativeLocation = FVector::ZeroVector;
	FRotator DefaultCameraRelativeRotation = FRotator::ZeroRotator;

	FVector StartLocation = FVector::ZeroVector;
	FVector TargetLocation = FVector::ZeroVector;
	FRotator StartRotation = FRotator::ZeroRotator;
	FRotator TargetRotation = FRotator::ZeroRotator;

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
	FVector2D LastInspectMousePos = FVector2D::ZeroVector;

	bool bExitingInspection = false;
	bool bExitingCameraFocus = false;
	bool bInspectedObjectHadCollision = true;

	FTimeline MoveTimeline;
	FTimeline CameraFocusTimeline;
	FTimeline InspectTimeline;

	void InitTimeline(FTimeline& Timeline, TObjectPtr<UCurveFloat>& Curve, float Length,
		float DefaultEndTime, float DefaultEndValue, bool bSmooth, FName UpdateFn, FName FinishedFn);

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
	void OnInventoryOpen();
	void OnInventoryClose();
	void OnInventoryScroll(const FInputActionValue& Value);
	void OnGoBack();
	void OnLook(const FInputActionValue& Value);
	void OnPause();

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

private:
	// Preview 3D system
	float PreviewRotationSpeed = 30.0f;
	int32 MaxPreviewSlots = 20;
	TArray<TObjectPtr<AActor>> PreviewActors;
	TArray<TObjectPtr<USceneCaptureComponent2D>> PreviewCaptures;
	TArray<TObjectPtr<UTextureRenderTarget2D>> PreviewRenderTargets;
	TArray<TObjectPtr<UMaterialInstanceDynamic>> PreviewMaterials;
};