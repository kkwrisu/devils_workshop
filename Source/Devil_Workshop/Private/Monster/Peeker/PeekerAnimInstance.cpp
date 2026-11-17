#include "Monster/Peeker/PeekerAnimInstance.h"
#include "Monster/Peeker/Peeker.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraShakeBase.h"

UPeekerAnimInstance::UPeekerAnimInstance()
{
    PeekerOwner = nullptr;
}

void UPeekerAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    PeekerOwner = Cast<APeeker>(TryGetPawnOwner());
}

void UPeekerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    if (!PeekerOwner) return;

    // Update speed/direction for locomotion
    FVector Velocity = PeekerOwner->GetVelocity();
    Velocity.Z = 0.0f;
    Speed = Velocity.Size();

    if (Speed > 1.0f)
    {
        FVector Forward = PeekerOwner->GetActorForwardVector();
        FVector Right = PeekerOwner->GetActorRightVector();
        Direction = FMath::Atan2(FVector::DotProduct(Velocity.GetSafeNormal(), Right), FVector::DotProduct(Velocity.GetSafeNormal(), Forward));
        Direction = FMath::RadiansToDegrees(Direction);
    }
    else
    {
        Direction = 0.0f;
    }

    // Update peek state
    bIsPeeking = PeekerOwner->IsPeeking();

    // Smooth peek alpha
    float TargetAlpha = bIsPeeking ? 1.0f : 0.0f;
    PeekAlpha = FMath::FInterpTo(PeekAlpha, TargetAlpha, DeltaSeconds, 5.0f);
}

void UPeekerAnimInstance::PlayMontage(UAnimMontage* Montage, float PlayRate, FName StartSection)
{
    if (!Montage) return;

    Montage_Play(Montage, PlayRate);
    if (StartSection != NAME_None)
    {
        Montage_JumpToSection(StartSection, Montage);
    }
    CurrentMontage = Montage;
}

void UPeekerAnimInstance::StopMontage(UAnimMontage* Montage)
{
    if (!Montage) return;

    Montage_Stop(0.2f, Montage);
    if (CurrentMontage == Montage)
    {
        CurrentMontage = nullptr;
    }
}

bool UPeekerAnimInstance::IsMontagePlaying(UAnimMontage* Montage) const
{
    if (!Montage) return false;
    return Montage_IsPlaying(Montage);
}

void UPeekerAnimInstance::OnPeekStartFinished()
{
    if (PeekerOwner)
    {
        PeekerOwner->OnPeekTransitionComplete();
    }
}

void UPeekerAnimInstance::OnPeekEndFinished()
{
    // Animation finished hiding
    bIsHiding = false;
}

void UPeekerAnimInstance::OnJumpscareFinished()
{
    if (PeekerOwner)
    {
        PeekerOwner->OnJumpscareComplete();
    }
}

void UPeekerAnimInstance::OnJumpscareImpact()
{
    // Moment of jumpscare impact - could trigger camera shake, sound, etc.
    if (PeekerOwner)
    {
        APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
        if (PC && PC->PlayerCameraManager)
        {
            PC->PlayerCameraManager->StartCameraShake(UCameraShakeBase::StaticClass(), 1.0f);
        }
    }
}

void UPeekerAnimInstance::SetPeekAlpha(float Alpha)
{
    PeekAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
}