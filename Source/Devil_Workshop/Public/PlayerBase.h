#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Components/TimelineComponent.h"
#include "PlayerBase.generated.h"

class UCameraComponent;
class USceneComponent;
class UInputMappingContext;
class UInputAction;
class UCurveFloat;
class UUserWidget;
struct FInputActionValue;

UCLASS()
class APlayerBase : public APawn
{
	GENERATED_BODY()

public:
	APlayerBase();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// Equivalente ao Custom Event "MoveToPosition"
	UFUNCTION(BlueprintCallable) void MoveToPosition(const FTransform& Target);
	// Equivalente ao Custom Event "StartInspection"
	UFUNCTION(BlueprintCallable) void StartInspection(AActor* TargetObject);

	UFUNCTION(BlueprintCallable) void ApplyLookDelta(double Delta);
	UFUNCTION(BlueprintCallable) void ApplyLookPitchDelta(double Delta);

protected:
	virtual void BeginPlay() override;

	// ---- Componentes ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UCameraComponent> Camera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USceneComponent> InspectionAnchor;

	// ---- Input (atribuir no BP filho) ----
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputMappingContext> DefaultMappingContext;
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> ClickAction;
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> GoBackAction;
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> LookAction;
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> PauseAction;

	// ---- Config ----
	UPROPERTY(EditDefaultsOnly, Category = "UI") TSubclassOf<UUserWidget> PauseMenuClass;
	UPROPERTY(EditAnywhere, Category = "Look") double EdgeLookMargin = 50.0;
	UPROPERTY(EditAnywhere, Category = "Look") double EdgeLookSpeed = 30.0;
	UPROPERTY(EditAnywhere, Category = "Look") double InspectRotateSpeed = 1.0;
	UPROPERTY(EditAnywhere, Category = "Bob") double BobFrequency = 1.0;
	UPROPERTY(EditAnywhere, Category = "Bob") double BobAmplitude = 2.0;

	// Curvas (se vazias, cria uma linear 0->1)
	UPROPERTY(EditDefaultsOnly, Category = "Timelines") TObjectPtr<UCurveFloat> MoveCurve;
	UPROPERTY(EditDefaultsOnly, Category = "Timelines") TObjectPtr<UCurveFloat> CameraFocusCurve;
	UPROPERTY(EditDefaultsOnly, Category = "Timelines") TObjectPtr<UCurveFloat> InspectCurve;
	UPROPERTY(EditDefaultsOnly, Category = "Timelines") float DefaultTimelineDuration = 1.f;

	// ---- Estado ----
	UPROPERTY(BlueprintReadOnly) bool IsTransitioning = false;
	UPROPERTY(BlueprintReadOnly) bool bIsInspecting = false;
	UPROPERTY(BlueprintReadOnly) bool bIsCameraFocused = false;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<AActor> CurrentAreaActor;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<AActor> InspectedObject;
	UPROPERTY() TObjectPtr<APlayerController> CachedPlayerController;
	UPROPERTY() TArray<FTransform> PositionHistory;

	double LookYawOffset = 0.0;
	double LookPitchOffset = 0.0;

	FVector DefaultCameraRelativeLocation; FRotator DefaultCameraRelativeRotation;
	FVector StartLocation, TargetLocation;  FRotator StartRotation, TargetRotation;
	FVector FocusStartLocation, FocusTargetLocation; FRotator FocusStartRotation, FocusTargetRotation;
	FVector InspectStartLocation; FRotator InspectStartRotation;

	// ---- Timelines ----
	FTimeline MoveTimeline, CameraFocusTimeline, InspectTimeline;
	void InitTimeline(FTimeline& Timeline, TObjectPtr<UCurveFloat>& Curve, FName UpdateFn, FName FinishedFn);

	UFUNCTION() void OnMoveUpdate(float Alpha);
	UFUNCTION() void OnMoveFinished();
	UFUNCTION() void OnCameraFocusUpdate(float Alpha);
	UFUNCTION() void OnInspectUpdate(float Alpha);
	UFUNCTION() void OnInspectFinished();

	// ---- Handlers de input ----
	void OnClick();
	void OnGoBack();
	void OnLook(const FInputActionValue& Value);
	void OnPause();

	void UpdateCameraRotation();
};