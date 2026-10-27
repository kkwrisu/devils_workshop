#pragma once

#include "CoreMinimal.h"
#include "Monster/MonsterBase.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Sound/SoundBase.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"
#include "Peeker.generated.h"

class UPeekerAnimInstance;
class USceneComponent;
class APlayerBase;

UCLASS(Blueprintable)
class DEVIL_WORKSHOP_API APeeker : public AMonsterBase
{
    GENERATED_BODY()

public:
    APeeker();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // Peeking Logic
    UFUNCTION(BlueprintCallable, Category = "Peeker|Peeking")
    void StartPeeking(AActor* PlayerNode);

    UFUNCTION(BlueprintCallable, Category = "Peeker|Peeking")
    void StopPeeking();

    UFUNCTION(BlueprintCallable, Category = "Peeker|Peeking")
    void UpdatePeekVisual();

    UFUNCTION(BlueprintPure, Category = "Peeker|Peeking")
    bool IsPeeking() const { return bIsPeeking; }

    UFUNCTION(BlueprintPure, Category = "Peeker|Peeking")
    FName GetCurrentPeekSocket() const { return CurrentPeekSocket; }

    // Socket Management
    UFUNCTION(BlueprintCallable, Category = "Peeker|Sockets")
    void FindAvailablePeekSocket(AActor* PlayerNode);

    UFUNCTION(BlueprintCallable, Category = "Peeker|Sockets")
    TArray<FName> GetPeekSocketsForNode(AActor* Node) const;

    // Jumpscare
    virtual void PerformJumpscare_Implementation() override;

    // Reactions
    virtual void OnPlayerUsedCamera_Implementation(AActor* CameraActor) override;
    virtual void OnPlayerClosedDoor_Implementation(AActor* DoorActor) override;
    virtual void OnPlayerMadeNoise_Implementation(const FNoiseEvent& NoiseEvent) override;
    virtual void OnPlayerStaredAtPeeker_Implementation() override;

    // Animation
    UFUNCTION(BlueprintCallable, Category = "Peeker|Animation")
    void PlayPeekMontage();

    UFUNCTION(BlueprintCallable, Category = "Peeker|Animation")
    void PlayJumpscareMontage();

    UFUNCTION(BlueprintCallable, Category = "Peeker|Animation")
    void PlayHideMontage();

protected:
    // Peek Socket Configuration
    UPROPERTY(EditAnywhere, Category = "Peeker|Peeking", meta = (Categories = "Peek"))
    TArray<FName> PeekSocketNames;

    UPROPERTY(EditAnywhere, Category = "Peeker|Peeking")
    float PeekTransitionTime = 0.5f;

    UPROPERTY(EditAnywhere, Category = "Peeker|Peeking")
    float PeekBobAmount = 10.0f;

    UPROPERTY(EditAnywhere, Category = "Peeker|Peeking")
    float PeekBobSpeed = 2.0f;

    // State
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Peeker|State", Replicated)
    bool bIsPeeking = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Peeker|State", Replicated)
    FName CurrentPeekSocket = NAME_None;

    UPROPERTY()
    TObjectPtr<AActor> PeekTargetNode;

    UPROPERTY()
    FVector PeekStartLocation;

    UPROPERTY()
    FQuat PeekStartRotation;

    // Animation
    UPROPERTY(EditAnywhere, Category = "Peeker|Animation")
    TObjectPtr<UAnimMontage> PeekStartMontage;

    UPROPERTY(EditAnywhere, Category = "Peeker|Animation")
    TObjectPtr<UAnimMontage> PeekLoopMontage;

    UPROPERTY(EditAnywhere, Category = "Peeker|Animation")
    TObjectPtr<UAnimMontage> PeekEndMontage;

    UPROPERTY(EditAnywhere, Category = "Peeker|Animation")
    TObjectPtr<UAnimMontage> JumpscareMontage;

    UPROPERTY(EditAnywhere, Category = "Peeker|Animation")
    TObjectPtr<UAnimMontage> HideMontage;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Peeker|Animation", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UPeekerAnimInstance> PeekerAnimInstance;

    // Jumpscare
    UPROPERTY(EditAnywhere, Category = "Peeker|Jumpscare")
    float JumpscareDuration = 2.0f;

    UPROPERTY(EditAnywhere, Category = "Peeker|Jumpscare")
    float JumpscareCameraFOV = 120.0f;

    UPROPERTY(EditAnywhere, Category = "Peeker|Jumpscare")
    TObjectPtr<USoundBase> JumpscareSound;

    UPROPERTY(EditAnywhere, Category = "Peeker|Jumpscare")
    TSubclassOf<UUserWidget> JumpscareWidgetClass;

    // Timers
    UPROPERTY()
    FTimerHandle PeekTimerHandle;

    UPROPERTY()
    FTimerHandle PeekTransitionTimerHandle;

    // Internal - made public for access from anim instance
public:
    UFUNCTION(BlueprintCallable, Category = "Peeker|Internal")
    void OnPeekTransitionComplete();

    UFUNCTION(BlueprintCallable, Category = "Peeker|Internal")
    void OnPeekTimerElapsed();

    UFUNCTION(BlueprintCallable, Category = "Peeker|Internal")
    void OnJumpscareComplete();

    UFUNCTION(BlueprintCallable, Category = "Peeker|Internal")
    void UpdatePeekBob(float DeltaTime);

    // Player Detection via Edge Look
    UFUNCTION(BlueprintCallable, Category = "Peeker|Detection")
    bool IsPlayerLookingAtPeek() const;

    // Audio
    UPROPERTY(EditAnywhere, Category = "Peeker|Audio")
    TObjectPtr<USoundBase> PeekStartSound;

    UPROPERTY(EditAnywhere, Category = "Peeker|Audio")
    TObjectPtr<USoundBase> PeekLoopSound;

    UPROPERTY(EditAnywhere, Category = "Peeker|Audio")
    TObjectPtr<USoundBase> HideSound;
};