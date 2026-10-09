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
class APlayerController;
struct FInputActionValue;

UCLASS(Blueprintable)
class APlayerBase : public APawn
{
	GENERATED_BODY()

public:
	APlayerBase();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// Equivalente ao Custom Event "MoveToPosition" do BP
	UFUNCTION(BlueprintCallable, Category = "Player")
	void MoveToPosition(const FTransform& Target);

	// Equivalente ao Custom Event "StartInspection" do BP
	UFUNCTION(BlueprintCallable, Category = "Player")
	void StartInspection(AActor* TargetObject);

	// Equivalentes às funções "ApplyLookDelta" e "ApplyLookPitchDelta" do BP
	UFUNCTION(BlueprintCallable, Category = "Player")
	void ApplyLookDelta(double Delta);

	UFUNCTION(BlueprintCallable, Category = "Player")
	void ApplyLookPitchDelta(double Delta);

	// ---- Componentes ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> InspectionAnchor;

	// ---- Estado (mesmos nomes das variáveis do BP, acessíveis por outros BPs) ----
	UPROPERTY(BlueprintReadWrite, Category = "Estado")
	bool IsTransitioning = false;

	UPROPERTY(BlueprintReadWrite, Category = "Estado")
	bool isInspecting = false;

	UPROPERTY(BlueprintReadWrite, Category = "Estado")
	bool bIsCameraFocused = false;

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

	// ---- Input (atribuir no Class Defaults do BP filho) ----
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

	// ---- UI ----
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> PauseMenuClass;

	// ---- Look ----
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

	// ---- Head bob ----
	UPROPERTY(EditAnywhere, Category = "Bob")
	double BobFrequency = 2.0;

	UPROPERTY(EditAnywhere, Category = "Bob")
	double BobAmplitude = 40.0;

	// ---- Timelines ----
	// Curvas opcionais: se ficarem vazias, o código cria as curvas padrão (ver BeginPlay no .cpp).
	UPROPERTY(EditDefaultsOnly, Category = "Timelines")
	TObjectPtr<UCurveFloat> MoveCurve;

	UPROPERTY(EditDefaultsOnly, Category = "Timelines")
	TObjectPtr<UCurveFloat> CameraFocusCurve;

	UPROPERTY(EditDefaultsOnly, Category = "Timelines")
	TObjectPtr<UCurveFloat> InspectCurve;

	// Duração (Length) de cada timeline, igual ao BP
	UPROPERTY(EditDefaultsOnly, Category = "Timelines")
	float MoveLength = 0.65f;

	UPROPERTY(EditDefaultsOnly, Category = "Timelines")
	float CameraFocusLength = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "Timelines")
	float InspectLength = 0.5f;

	// ---- Internos ----
	UPROPERTY()
	TObjectPtr<APlayerController> CachedPlayerController;

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
	void OnInspectUpdate(float Alpha);

	UFUNCTION()
	void OnInspectFinished();

	// ---- Handlers de input ----
	void OnClick();
	void OnGoBack();
	void OnLook(const FInputActionValue& Value);
	void OnPause();

	void UpdateCameraRotation();
};