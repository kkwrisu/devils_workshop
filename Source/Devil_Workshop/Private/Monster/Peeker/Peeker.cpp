#include "Monster/Peeker/Peeker.h"
#include "Monster/Peeker/PeekerAnimInstance.h"
#include "PlayerBase.h"
#include "Monster/MonsterAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Net/UnrealNetwork.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "Sound/SoundBase.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Camera/CameraComponent.h"
#include "AIController.h"

APeeker::APeeker()
{
    CreatureType = ECreatureType::Peeker;

    // Default peek sockets (configured per level in Blueprints)
    PeekSocketNames = {
        FName("Peek_Window_Left"),
        FName("Peek_Window_Right"),
        FName("Peek_Door_Center"),
        FName("Peek_Vent"),
        FName("Peek_Corner")
    };

    // Movement config for Peeker
    GetCharacterMovement()->MaxWalkSpeed = 250.0f; // Slower, stealthier
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 360.0f, 0.0f);

    // Capsule - smaller for hiding
    GetCapsuleComponent()->SetCapsuleHalfHeight(88.0f);
    GetCapsuleComponent()->SetCapsuleRadius(34.0f);

    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.0f;
}

void APeeker::BeginPlay()
{
    Super::BeginPlay();

    // Cache anim instance
    PeekerAnimInstance = Cast<UPeekerAnimInstance>(GetMesh()->GetAnimInstance());

    // Start hidden
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    GetMesh()->SetVisibility(false);

    // Find initial node if not set
    if (!CurrentNode && NavigationNodes.Num() > 0)
    {
        CurrentNode = NavigationNodes[0];
        SetActorLocation(CurrentNode->GetActorLocation());
    }
}

void APeeker::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsPeeking)
    {
        UpdatePeekBob(DeltaTime);
        UpdatePeekVisual();

        // Check if player is looking at us (Edge Look detection)
        if (IsPlayerLookingAtPeek())
        {
            OnPlayerStaredAtPeeker();
        }
    }
}

void APeeker::StartPeeking(AActor* PlayerNode)
{
    if (!PlayerNode || bIsPeeking) return;

    PeekTargetNode = PlayerNode;
    bIsPeeking = true;

    FindAvailablePeekSocket(PlayerNode);

    if (CurrentPeekSocket == NAME_None)
    {
        UE_LOG(LogTemp, Warning, TEXT("Peeker: No valid peek socket found on node %s"), *PlayerNode->GetName());
        StopPeeking();
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Peeker: Starting peek at socket %s on node %s"), *CurrentPeekSocket.ToString(), *PlayerNode->GetName());

    // Teleport to peek position
    USceneComponent* SocketComponent = PlayerNode->FindComponentByClass<USceneComponent>();
    if (SocketComponent)
    {
        // Find the specific socket on the mesh
        UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(SocketComponent);
        if (MeshComp && MeshComp->DoesSocketExist(CurrentPeekSocket))
        {
            PeekStartLocation = MeshComp->GetSocketLocation(CurrentPeekSocket);
            PeekStartRotation = MeshComp->GetSocketQuaternion(CurrentPeekSocket);
        }
        else
        {
            // Fallback: use actor location + offset
            PeekStartLocation = PlayerNode->GetActorLocation() + FVector(0, 0, 100);
            PeekStartRotation = PlayerNode->GetActorQuat();
        }
    }

    // Show mesh at peek position
    SetActorHiddenInGame(false);
    SetActorEnableCollision(true);
    GetMesh()->SetVisibility(true);
    SetActorLocationAndRotation(PeekStartLocation, PeekStartRotation);

    // Play peek start animation/sound
    PlayPeekMontage();
    if (PeekStartSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, PeekStartSound, GetActorLocation());
    }

    // Start peek loop sound
    if (PeekLoopSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, PeekLoopSound, GetActorLocation());
    }

    // Set timer for peek duration
    GetWorldTimerManager().SetTimer(PeekTimerHandle, this, &APeeker::OnPeekTimerElapsed, PeekDuration, false);

    // Update Blackboard
    if (AMonsterAIController* AIController = Cast<AMonsterAIController>(GetController()))
    {
        if (UBlackboardComponent* BB = AIController->GetBlackboardComponent())
        {
            BB->SetValueAsBool("IsPeeking", true);
            BB->SetValueAsName("PeekSocketName", CurrentPeekSocket);
        }
    }
}

void APeeker::StopPeeking()
{
    if (!bIsPeeking) return;

    bIsPeeking = false;
    CurrentPeekSocket = NAME_None;
    PeekTargetNode = nullptr;

    GetWorldTimerManager().ClearTimer(PeekTimerHandle);
    GetWorldTimerManager().ClearTimer(PeekTransitionTimerHandle);

    // Play hide animation
    PlayHideMontage();
    if (HideSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, HideSound, GetActorLocation());
    }

    // Hide after animation
    FTimerHandle HideTimer;
    GetWorldTimerManager().SetTimer(HideTimer, [this]()
    {
        SetActorHiddenInGame(true);
        SetActorEnableCollision(false);
        GetMesh()->SetVisibility(false);
    }, 1.0f, false);

    // Update Blackboard
    if (AMonsterAIController* AIController = Cast<AMonsterAIController>(GetController()))
    {
        if (UBlackboardComponent* BB = AIController->GetBlackboardComponent())
        {
            BB->SetValueAsBool("IsPeeking", false);
        }
    }
}

void APeeker::UpdatePeekVisual()
{
    if (!bIsPeeking || !PeekTargetNode) return;

    // Smooth bob animation while peeking
    // (Handled in UpdatePeekBob for location, here we ensure we stay at socket)
    USceneComponent* SocketComponent = PeekTargetNode->FindComponentByClass<USceneComponent>();
    if (SocketComponent)
    {
        UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(SocketComponent);
        if (MeshComp && MeshComp->DoesSocketExist(CurrentPeekSocket))
        {
            FVector SocketLocation = MeshComp->GetSocketLocation(CurrentPeekSocket);
            FQuat SocketRotation = MeshComp->GetSocketQuaternion(CurrentPeekSocket);

            // Smooth interpolation to handle door/window movement
            SetActorLocationAndRotation(
                FMath::VInterpTo(GetActorLocation(), SocketLocation, GetWorld()->GetDeltaSeconds(), 10.0f),
                FMath::QInterpTo(GetActorQuat(), SocketRotation, GetWorld()->GetDeltaSeconds(), 10.0f)
            );
        }
    }
}

void APeeker::FindAvailablePeekSocket(AActor* PlayerNode)
{
    CurrentPeekSocket = NAME_None;

    if (!PlayerNode) return;

    UStaticMeshComponent* MeshComp = PlayerNode->FindComponentByClass<UStaticMeshComponent>();
    if (!MeshComp) return;

    // Check configured sockets in order of priority
    for (FName SocketName : PeekSocketNames)
    {
        if (MeshComp->DoesSocketExist(SocketName))
        {
            // Check if socket is visible from player camera (not occluded)
            FVector SocketLocation = MeshComp->GetSocketLocation(SocketName);
            APlayerBase* Player = GetPlayerPawn();

            if (Player && Player->Camera)
            {
                FVector CameraLocation = Player->Camera->GetComponentLocation();
                FVector Direction = (SocketLocation - CameraLocation).GetSafeNormal();

                FCollisionQueryParams Params(SCENE_QUERY_STAT(PeekVisibility), true);
                Params.AddIgnoredActor(this);
                Params.AddIgnoredActor(Player);

                FHitResult Hit;
                if (!GetWorld()->LineTraceSingleByChannel(Hit, CameraLocation, SocketLocation, ECC_Visibility, Params))
                {
                    // Socket is visible - use it!
                    CurrentPeekSocket = SocketName;
                    return;
                }
                else if (Hit.GetActor() == PlayerNode)
                {
                    // Hit the node itself - socket is on surface, visible
                    CurrentPeekSocket = SocketName;
                    return;
                }
            }
            else
            {
                // No player reference, just use first available
                CurrentPeekSocket = SocketName;
                return;
            }
        }
    }
}

TArray<FName> APeeker::GetPeekSocketsForNode(AActor* Node) const
{
    TArray<FName> AvailableSockets;

    if (!Node) return AvailableSockets;

    UStaticMeshComponent* MeshComp = Node->FindComponentByClass<UStaticMeshComponent>();
    if (!MeshComp) return AvailableSockets;

    for (FName SocketName : PeekSocketNames)
    {
        if (MeshComp->DoesSocketExist(SocketName))
        {
            AvailableSockets.Add(SocketName);
        }
    }

    return AvailableSockets;
}

void APeeker::OnPeekTimerElapsed()
{
    if (bIsPeeking)
    {
        // Peek duration expired - JUMPSCARE!
        SetMonsterState(EMonsterState::Attack);
    }
}

void APeeker::OnPeekTransitionComplete()
{
    // Called after peek start montage finishes
    if (PeekLoopMontage && PeekerAnimInstance)
    {
        PeekerAnimInstance->Montage_Play(PeekLoopMontage);
    }
}

void APeeker::OnJumpscareComplete()
{
    // Restart level or game over
    UGameplayStatics::OpenLevel(GetWorld(), FName(*GetWorld()->GetName()));
}

void APeeker::UpdatePeekBob(float DeltaTime)
{
    if (!bIsPeeking) return;

    float Time = GetWorld()->GetTimeSeconds();
    float BobOffset = FMath::Sin(Time * PeekBobSpeed) * PeekBobAmount;

    FVector CurrentLocation = GetActorLocation();
    CurrentLocation.Z = PeekStartLocation.Z + BobOffset;
    SetActorLocation(CurrentLocation);
}

bool APeeker::IsPlayerLookingAtPeek() const
{
    APlayerBase* Player = GetPlayerPawn();
    if (!Player || !Player->Camera || !PeekTargetNode) return false;

    // Check if player is in CameraFocus mode looking at a "SecurityCamera"
    // OR if edge-look is pointing at our peek socket
    if (Player->bIsCameraFocused)
    {
        AActor* FocusActor = Player->InspectedObject; // Reused for camera focus target
        if (FocusActor && FocusActor->ActorHasTag("SecurityCamera"))
        {
            return true;
        }
    }

    // Check edge-look direction towards peek socket
    if (Player->Camera)
    {
        FVector CameraLocation = Player->Camera->GetComponentLocation();
        FVector CameraForward = Player->Camera->GetForwardVector();

        USceneComponent* SocketComponent = PeekTargetNode->FindComponentByClass<USceneComponent>();
        if (SocketComponent)
        {
            UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(SocketComponent);
            if (MeshComp && MeshComp->DoesSocketExist(CurrentPeekSocket))
            {
                FVector SocketLocation = MeshComp->GetSocketLocation(CurrentPeekSocket);
                FVector ToSocket = (SocketLocation - CameraLocation).GetSafeNormal();

                float Dot = FVector::DotProduct(CameraForward, ToSocket);
                if (Dot > 0.95f) // ~18 degrees
                {
                    return true;
                }
            }
        }
    }

    return false;
}

void APeeker::PlayPeekMontage()
{
    if (PeekStartMontage && PeekerAnimInstance)
    {
        PeekerAnimInstance->Montage_Play(PeekStartMontage);
        FTimerHandle MontageTimer;
        GetWorldTimerManager().SetTimer(MontageTimer, this, &APeeker::OnPeekTransitionComplete, PeekStartMontage->GetPlayLength(), false);
    }
}

void APeeker::PlayJumpscareMontage()
{
    if (JumpscareMontage && PeekerAnimInstance)
    {
        PeekerAnimInstance->Montage_Play(JumpscareMontage);
    }

    if (JumpscareSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, JumpscareSound, GetActorLocation());
    }

    // Show jumpscare widget
    if (JumpscareWidgetClass && GetWorld())
    {
        APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
        if (PC)
        {
            UUserWidget* Widget = CreateWidget<UUserWidget>(PC, JumpscareWidgetClass);
            if (Widget)
            {
                Widget->AddToViewport(1000);
            }
        }
    }

    FTimerHandle JumpscareTimer;
    GetWorldTimerManager().SetTimer(JumpscareTimer, this, &APeeker::OnJumpscareComplete, JumpscareDuration, false);
}

void APeeker::PlayHideMontage()
{
    if (PeekEndMontage && PeekerAnimInstance)
    {
        PeekerAnimInstance->Montage_Play(PeekEndMontage);
    }
    else if (HideMontage && PeekerAnimInstance)
    {
        PeekerAnimInstance->Montage_Play(HideMontage);
    }
}

void APeeker::PerformJumpscare_Implementation()
{
    UE_LOG(LogTemp, Warning, TEXT("Peeker: JUMPSCARE!"));

    OnJumpscare.Broadcast(this);

    // Teleport to player camera for jumpscare
    APlayerBase* Player = GetPlayerPawn();
    if (Player && Player->Camera)
    {
        FVector JumpscareLocation = Player->Camera->GetComponentLocation() + Player->Camera->GetForwardVector() * 50.0f;
        SetActorLocationAndRotation(JumpscareLocation, Player->Camera->GetComponentRotation().Quaternion());
    }

    PlayJumpscareMontage();
}

void APeeker::OnPlayerUsedCamera_Implementation(AActor* CameraActor)
{
    UE_LOG(LogTemp, Log, TEXT("Peeker: Player used security camera %s"), CameraActor ? *CameraActor->GetName() : TEXT("NULL"));

    if (bIsPeeking || CurrentState == EMonsterState::Investigate)
    {
        // Caught on camera - retreat!
        StopPeeking();
        SetMonsterState(EMonsterState::Hiding);

        if (PreviousNode)
        {
            MoveToNode(PreviousNode);
        }
        else
        {
            ChooseNextPatrolNode();
        }
    }
}

void APeeker::OnPlayerClosedDoor_Implementation(AActor* DoorActor)
{
    UE_LOG(LogTemp, Log, TEXT("Peeker: Player closed door %s"), DoorActor ? *DoorActor->GetName() : TEXT("NULL"));

    if (bIsPeeking && PeekTargetNode == DoorActor)
    {
        // Door blocked our peek spot
        StopPeeking();
        SetMonsterState(EMonsterState::Hiding);

        if (PreviousNode)
        {
            MoveToNode(PreviousNode);
        }
    }
}

void APeeker::OnPlayerMadeNoise_Implementation(const FNoiseEvent& NoiseEvent)
{
    UE_LOG(LogTemp, Log, TEXT("Peeker: Heard noise %s"), *NoiseEvent.NoiseTag.ToString());

    if (CurrentState == EMonsterState::Patrol || CurrentState == EMonsterState::Hiding)
    {
        // Distracted by noise - investigate
        StopPeeking();
        SetMonsterState(EMonsterState::Investigate);

        if (AMonsterAIController* AIController = Cast<AMonsterAIController>(GetController()))
        {
            AIController->SetBlackboardVector(FName("NoiseLocation"), NoiseEvent.Location);
            AIController->MoveToLocation(NoiseEvent.Location, 50.0f);
        }
    }
}

void APeeker::OnPlayerStaredAtPeeker_Implementation()
{
    UE_LOG(LogTemp, Log, TEXT("Peeker: Player stared at us!"));

    if (bIsPeeking)
    {
        // Weeping Angel style - player looking at us forces us to hide
        StopPeeking();
        SetMonsterState(EMonsterState::Hiding);

        if (PreviousNode)
        {
            MoveToNode(PreviousNode);
        }
    }
}

void APeeker::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(APeeker, bIsPeeking);
    DOREPLIFETIME(APeeker, CurrentPeekSocket);
}