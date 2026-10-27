#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PeekerAnimInstance.generated.h"

class APeeker;

UCLASS(Blueprintable, BlueprintType)
class DEVIL_WORKSHOP_API UPeekerAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    UPeekerAnimInstance();

    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

    // Animation State
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Peeker|Animation")
    bool bIsPeeking = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Peeker|Animation")
    bool bIsHiding = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Peeker|Animation")
    bool bIsJumpscaring = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Peeker|Animation")
    float PeekAlpha = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Peeker|Animation")
    float Speed = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Peeker|Animation")
    float Direction = 0.0f;

    // Montage Playback
    UFUNCTION(BlueprintCallable, Category = "Peeker|Animation")
    void PlayMontage(UAnimMontage* Montage, float PlayRate = 1.0f, FName StartSection = NAME_None);

    UFUNCTION(BlueprintCallable, Category = "Peeker|Animation")
    void StopMontage(UAnimMontage* Montage);

    UFUNCTION(BlueprintPure, Category = "Peeker|Animation")
    bool IsMontagePlaying(UAnimMontage* Montage) const;

    // Animation Notifies (called from Animation Blueprint)
    UFUNCTION(BlueprintCallable, Category = "Peeker|Animation")
    void OnPeekStartFinished();

    UFUNCTION(BlueprintCallable, Category = "Peeker|Animation")
    void OnPeekEndFinished();

    UFUNCTION(BlueprintCallable, Category = "Peeker|Animation")
    void OnJumpscareFinished();

    UFUNCTION(BlueprintCallable, Category = "Peeker|Animation")
    void OnJumpscareImpact();

    // Peek Blend
    UFUNCTION(BlueprintCallable, Category = "Peeker|Animation")
    void SetPeekAlpha(float Alpha);

protected:
    UPROPERTY()
    TObjectPtr<APeeker> PeekerOwner;

    UPROPERTY()
    TObjectPtr<UAnimMontage> CurrentMontage;
};