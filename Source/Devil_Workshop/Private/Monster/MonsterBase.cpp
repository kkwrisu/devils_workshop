#include "Monster/MonsterBase.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Sound/SoundBase.h"
#include "Monster/MonsterAIController.h"
#include "PlayerBase.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Net/UnrealNetwork.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Monsters/MonsterTypes.h"

namespace
{
	const FName MonsterPatrolNodeTag(TEXT("MonsterPatrolPoint"));
}

AMonsterBase::AMonsterBase()
{
    PrimaryActorTick.bCanEverTick = true;
    AIControllerClass = AMonsterAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    // AI Perception - created on the AI Controller, not here
    // The perception component will be created in the AI Controller

    // Movement
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 480.0f, 0.0f);
    GetCharacterMovement()->MaxWalkSpeed = 300.0f;
    GetCharacterMovement()->bUseControllerDesiredRotation = false;

    // Capsule
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    // Mesh
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetMesh()->SetCollisionResponseToAllChannels(ECR_Ignore);
    GetMesh()->SetGenerateOverlapEvents(false);

    // Replication
    bReplicates = true;
    SetReplicateMovement(true);
}

void AMonsterBase::BeginPlay()
{
    Super::BeginPlay();

    // Initialize gameplay tags (only once)
    static bool bTagsInitialized = false;
    if (!bTagsInitialized)
    {
        MonsterTags::InitializeTags();
        bTagsInitialized = true;
    }

    InitializePerception();
    BuildNavigationGraph();
	LastMonsterFootstepLocation = GetActorLocation();
	bHasLastMonsterFootstepLocation = true;

    if (APlayerBase* Player = GetPlayerPawn())
    {
        Player->OnPlayerMadeNoise.AddUniqueDynamic(this, &AMonsterBase::HandlePlayerNoiseEvent);
    }

    // Start in Patrol state after a short delay
    FTimerHandle InitialTimer;
    GetWorldTimerManager().SetTimer(InitialTimer, [this]()
    {
        if (HasAuthority())
        {
            if (!bCorridorChaseMode)
            {
                SetMonsterState(EMonsterState::Patrol);
            }
        }
    }, 1.0f, false);
}

void AMonsterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (APlayerBase* Player = GetPlayerPawn())
    {
        Player->OnPlayerMadeNoise.RemoveDynamic(this, &AMonsterBase::HandlePlayerNoiseEvent);
    }
    GetWorldTimerManager().ClearTimer(NoiseSilenceTimerHandle);
    if (CorridorChaseAudio)
    {
        CorridorChaseAudio->Stop();
        CorridorChaseAudio = nullptr;
    }
    if (CorridorChaseMusicAudio)
    {
        CorridorChaseMusicAudio->Stop();
        CorridorChaseMusicAudio = nullptr;
    }
    Super::EndPlay(EndPlayReason);
}

void AMonsterBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

	if (bHasLastMonsterFootstepLocation)
	{
		const FVector CurrentLocation = GetActorLocation();
		const float DistanceMoved = FVector::Dist2D(CurrentLocation, LastMonsterFootstepLocation);
		LastMonsterFootstepLocation = CurrentLocation;

		if (MonsterFootstepSound && GetVelocity().Size2D() > 20.0f && DistanceMoved > 0.0f)
		{
			MonsterFootstepDistanceAccumulator += DistanceMoved;
			const float StrideDistance = FMath::Max(MonsterFootstepStrideDistance, 50.0f);
			while (MonsterFootstepDistanceAccumulator >= StrideDistance)
			{
				MonsterFootstepDistanceAccumulator -= StrideDistance;
				const FVector FootstepLocation = GetActorLocation()
					- FVector(0.0f, 0.0f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
				UGameplayStatics::PlaySoundAtLocation(
					this, MonsterFootstepSound, FootstepLocation, 1.0f, 1.0f, 0.0f, MonsterFootstepAttenuation);
			}
		}
	}

	if (bCorridorChaseMode)
	{
		if (bCorridorChaseActive)
		{
			if (CorridorMovementDebugTimeRemaining > 0.0f)
			{
				CorridorMovementDebugElapsed += DeltaTime;
				if (CorridorMovementDebugElapsed >= 1.0f)
				{
					CorridorMovementDebugElapsed = 0.0f;
					CorridorMovementDebugTimeRemaining -= 1.0f;
					const AMonsterAIController* ChaseAIController = Cast<AMonsterAIController>(GetController());
					const UCharacterMovementComponent* Movement = GetCharacterMovement();
					UE_LOG(LogTemp, Warning,
						TEXT("MonsterChaseDiag: pos=%s vel=%s speed=%.1f mode=%d pathStatus=%d target=%s"),
						*GetActorLocation().ToString(), *GetVelocity().ToString(), GetVelocity().Size(),
						Movement ? static_cast<int32>(Movement->MovementMode) : -1,
						ChaseAIController ? static_cast<int32>(ChaseAIController->GetMoveStatus()) : -1,
						*GetNameSafe(PerceivedTarget));
				}
			}

			CorridorHazardCheckElapsed += DeltaTime;
			if (CorridorHazardCheckElapsed >= 0.15f)
			{
				CorridorHazardCheckElapsed = 0.0f;
				if (APlayerBase* Player = Cast<APlayerBase>(PerceivedTarget))
				{
					Player->CheckMonsterProximityToCabinets(GetActorLocation());
				}
			}
		}
		return;
	}

    // Update blackboard with current state info
    if (AMonsterAIController* AIController = Cast<AMonsterAIController>(GetController()))
    {
        if (UBlackboardComponent* BB = AIController->GetBlackboardComponent())
        {
            BB->SetValueAsEnum("MonsterState", static_cast<uint8>(CurrentState));
            BB->SetValueAsObject("CurrentNode", CurrentNode);
            BB->SetValueAsObject("PreviousNode", PreviousNode);
            BB->SetValueAsObject("PerceivedTarget", PerceivedTarget);
        }
    }
}

void AMonsterBase::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    if (AMonsterAIController* AIController = Cast<AMonsterAIController>(NewController))
    {
        if (BehaviorTree && BlackboardAsset)
        {
            AIController->RunBehaviorTree(BehaviorTree);
        }
    }
}

void AMonsterBase::UnPossessed()
{
    Super::UnPossessed();
    GetWorldTimerManager().ClearAllTimersForObject(this);
}

void AMonsterBase::InitializePerception()
{
    // Perception is initialized in the AI Controller
    // We just bind to the perception events here
    if (AMonsterAIController* AIController = Cast<AMonsterAIController>(GetController()))
    {
        if (UAIPerceptionComponent* Perception = AIController->GetAIPerceptionComponent())
        {
            Perception->OnTargetPerceptionUpdated.AddDynamic(this, &AMonsterBase::OnTargetPerceived);
        }
    }
}

void AMonsterBase::BuildNavigationGraph()
{
    NavigationNodes.Empty();

    TArray<AActor*> PatrolPointActors;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), MonsterPatrolNodeTag, PatrolPointActors);

    for (AActor* Actor : PatrolPointActors)
    {
        if (Actor && Actor != this)
        {
            NavigationNodes.Add(Actor);
        }
    }

    if (!CurrentNode && NavigationNodes.Num() > 0)
    {
        float BestDistanceSquared = TNumericLimits<float>::Max();
        for (AActor* Node : NavigationNodes)
        {
            const float DistanceSquared = FVector::DistSquared(GetActorLocation(), Node->GetActorLocation());
            if (DistanceSquared < BestDistanceSquared)
            {
                BestDistanceSquared = DistanceSquared;
                CurrentNode = Node;
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("MonsterBase: Built patrol graph from tag '%s' with %d points"),
        *MonsterPatrolNodeTag.ToString(), NavigationNodes.Num());
}

void AMonsterBase::PrepareCorridorReveal()
{
	bCorridorChaseMode = true;
	bCorridorChaseActive = false;
	GetWorldTimerManager().ClearAllTimersForObject(this);

	if (AMonsterAIController* MonsterController = Cast<AMonsterAIController>(GetController()))
	{
		MonsterController->StopBehaviorTree();
		MonsterController->StopMovement();
	}

	GetCharacterMovement()->DisableMovement();
	SetMonsterState(EMonsterState::Idle);
	UE_LOG(LogTemp, Warning, TEXT("MonsterBase '%s': aguardando fim da revelacao antes de perseguir"), *GetName());
}

void AMonsterBase::StartCorridorChase(AActor* TargetActor)
{
	if (!IsValid(TargetActor)) return;

	bCorridorChaseMode = true;
	bCorridorChaseActive = true;
	CorridorHazardCheckElapsed = 0.15f;
	CorridorMovementDebugElapsed = 0.0f;
	CorridorMovementDebugTimeRemaining = 6.0f;
	PerceivedTarget = TargetActor;
	LastMonsterFootstepLocation = GetActorLocation();
	MonsterFootstepDistanceAccumulator = 0.0f;
	bHasLastMonsterFootstepLocation = true;
	GetWorldTimerManager().ClearAllTimersForObject(this);
	GetCharacterMovement()->MaxWalkSpeed = 350.0f;
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	SetMonsterState(EMonsterState::Chase);

	if (CorridorChaseSound && !CorridorChaseAudio)
	{
		CorridorChaseAudio = UGameplayStatics::SpawnSoundAttached(
			CorridorChaseSound, GetRootComponent(), NAME_None, FVector::ZeroVector,
			EAttachLocation::KeepRelativeOffset, true);
	}
	if (CorridorChaseMusic && !CorridorChaseMusicAudio)
	{
		CorridorChaseMusicAudio = UGameplayStatics::SpawnSound2D(this, CorridorChaseMusic);
	}

	if (AMonsterAIController* MonsterController = Cast<AMonsterAIController>(GetController()))
	{
		MonsterController->StopBehaviorTree();
		MonsterController->StopMovement();
		const EPathFollowingRequestResult::Type MoveResult =
			MonsterController->MoveToActor(TargetActor, 70.0f, true, true, false, nullptr, true);
		const TCHAR* MoveResultName = MoveResult == EPathFollowingRequestResult::RequestSuccessful ? TEXT("Accepted")
			: MoveResult == EPathFollowingRequestResult::AlreadyAtGoal ? TEXT("AlreadyAtGoal") : TEXT("Failed");

		UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		FNavLocation ProjectedMonsterLocation;
		FNavLocation ProjectedTargetLocation;
		const FVector ProjectionExtent(300.0f, 300.0f, 500.0f);
		const bool bMonsterProjectsToNav = NavSystem && NavSystem->ProjectPointToNavigation(GetActorLocation(), ProjectedMonsterLocation, ProjectionExtent);
		const bool bTargetProjectsToNav = NavSystem && NavSystem->ProjectPointToNavigation(TargetActor->GetActorLocation(), ProjectedTargetLocation, ProjectionExtent);
		UE_LOG(LogTemp, Warning,
			TEXT("MonsterChaseDiag: MoveToActor=%s | monsterNav=%d (%s) | targetNav=%d (%s) | mode=%d speed=%.1f"),
			MoveResultName,
			bMonsterProjectsToNav ? 1 : 0, *GetActorLocation().ToString(),
			bTargetProjectsToNav ? 1 : 0, *TargetActor->GetActorLocation().ToString(),
			static_cast<int32>(GetCharacterMovement()->MovementMode), GetCharacterMovement()->MaxWalkSpeed);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MonsterBase '%s': sem MonsterAIController para iniciar perseguição"), *GetName());
	}
}

void AMonsterBase::FadeOutCorridorChaseMusic(float FadeDuration)
{
	if (CorridorChaseMusicAudio && CorridorChaseMusicAudio->IsPlaying())
	{
		CorridorChaseMusicAudio->FadeOut(FMath::Max(FadeDuration, 0.0f), 0.0f);
	}
}

TArray<AActor*> AMonsterBase::GetAdjacentNodes() const
{
    TArray<AActor*> AdjacentNodes;

    if (!CurrentNode) return AdjacentNodes;

    FVector CurrentLocation = CurrentNode->GetActorLocation();

    for (AActor* Node : NavigationNodes)
    {
        if (Node && Node != CurrentNode)
        {
            float Distance = FVector::Dist(CurrentLocation, Node->GetActorLocation());
            if (Distance <= NodeAdjacencyDistance)
            {
                AdjacentNodes.Add(Node);
            }
        }
    }

    return AdjacentNodes;
}

bool AMonsterBase::IsNodeAdjacentToPlayer(AActor* Node) const
{
    APlayerBase* Player = GetPlayerPawn();
    if (!Player || !Player->CurrentAreaActor || !Node) return false;

    const float DistanceSquared = FVector::DistSquared(
        Node->GetActorLocation(), Player->CurrentAreaActor->GetActorLocation());
    return DistanceSquared <= FMath::Square(NodeAdjacencyDistance);
}

APlayerBase* AMonsterBase::GetPlayerPawn() const
{
    return Cast<APlayerBase>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
}

void AMonsterBase::SetMonsterState(EMonsterState NewState)
{
    if (CurrentState == NewState) return;

    EMonsterState OldState = CurrentState;
    CurrentState = NewState;

    HandleStateTransition(OldState, NewState);

    OnStateChanged.Broadcast(NewState);

    // Update Blackboard
    if (AMonsterAIController* AIController = Cast<AMonsterAIController>(GetController()))
    {
        if (UBlackboardComponent* BB = AIController->GetBlackboardComponent())
        {
            BB->SetValueAsEnum("MonsterState", static_cast<uint8>(NewState));
        }
    }

    UE_LOG(LogTemp, Log, TEXT("MonsterBase: State changed %s -> %s"),
        *UEnum::GetValueAsString(OldState), *UEnum::GetValueAsString(NewState));
}

void AMonsterBase::HandleStateTransition(EMonsterState FromState, EMonsterState ToState)
{
    GetWorldTimerManager().ClearTimer(StateTimerHandle);
    GetWorldTimerManager().ClearTimer(PatrolTimerHandle);

    switch (ToState)
    {
        case EMonsterState::Patrol:
            ChooseNextPatrolNode();
            break;

        case EMonsterState::Investigate:
            GetWorldTimerManager().SetTimer(StateTimerHandle, this, &AMonsterBase::OnStateTimerElapsed, InvestigationDuration, false);
            break;

        case EMonsterState::Peeking:
            GetWorldTimerManager().SetTimer(StateTimerHandle, this, &AMonsterBase::OnStateTimerElapsed, PeekDuration, false);
            break;

        case EMonsterState::Hiding:
            GetWorldTimerManager().ClearTimer(NoiseSilenceTimerHandle);
            NoiseEventsSinceQuiet = 0;
            bInvestigatingNoise = false;
            GetWorldTimerManager().SetTimer(StateTimerHandle, this, &AMonsterBase::OnStateTimerElapsed, HidingCooldown, false);
            break;

        case EMonsterState::Chase:
            // Handled by BT
            break;

        case EMonsterState::Attack:
            PerformJumpscare();
            break;

        default:
            break;
    }
}

void AMonsterBase::OnStateTimerElapsed()
{
    switch (CurrentState)
    {
        case EMonsterState::Investigate:
            if (bInvestigatingNoise) return;
            // Lost target during investigation, return to patrol
            if (!PerceivedTarget)
            {
                SetMonsterState(EMonsterState::Patrol);
            }
            break;

        case EMonsterState::Peeking:
            // Peek timeout - attack!
            SetMonsterState(EMonsterState::Attack);
            break;

        case EMonsterState::Hiding:
            SetMonsterState(EMonsterState::Patrol);
            break;

        default:
            break;
    }
}

void AMonsterBase::OnPatrolTimerElapsed()
{
    if (CurrentState == EMonsterState::Patrol)
    {
        ChooseNextPatrolNode();
    }
}

void AMonsterBase::ChooseNextPatrolNode()
{
    TArray<AActor*> Adjacent = GetAdjacentNodes();
    if (Adjacent.Num() == 0) return;

    // Filter out previous node to avoid ping-pong
    TArray<AActor*> ValidNodes;
    for (AActor* Node : Adjacent)
    {
        if (Node != PreviousNode)
        {
            ValidNodes.Add(Node);
        }
    }

    if (ValidNodes.Num() == 0)
    {
        ValidNodes = Adjacent; // Fallback
    }

    AActor* NextNode = ValidNodes[FMath::RandRange(0, ValidNodes.Num() - 1)];
    MoveToNode(NextNode);

    // Set timer for next patrol decision
    float WaitTime = FMath::FRandRange(PatrolWaitTimeMin, PatrolWaitTimeMax);
    GetWorldTimerManager().SetTimer(PatrolTimerHandle, this, &AMonsterBase::OnPatrolTimerElapsed, WaitTime, false);
}

void AMonsterBase::MoveToNode(AActor* TargetNode)
{
    if (!TargetNode) return;

    PreviousNode = CurrentNode;
    CurrentNode = TargetNode;

    if (AMonsterAIController* AIController = Cast<AMonsterAIController>(GetController()))
    {
        // UE5 MoveToActor signature: MoveToActor(GoalActor, AcceptanceRadius, bStopOnOverlap, bUsePathfinding, bCanStrafe, FilterClass, bAllowPartialPath)
        AIController->MoveToActor(TargetNode, 50.0f, true, true, false, nullptr, true);
    }
}

void AMonsterBase::SetCurrentNode(AActor* NodeActor)
{
    CurrentNode = NodeActor;
}

void AMonsterBase::SetPreviousNode(AActor* NodeActor)
{
    PreviousNode = NodeActor;
}

void AMonsterBase::OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus)
{
	if (bCorridorChaseMode) return;
    if (!Actor) return;

    APlayerBase* Player = Cast<APlayerBase>(Actor);
    if (!Player) return;

    if (Stimulus.WasSuccessfullySensed())
    {
        PerceivedTarget = Actor;
        LastPerceptionTime = GetWorld()->GetTimeSeconds();

        UE_LOG(LogTemp, Log, TEXT("MonsterBase: Player perceived via %s (Strength: %.1f)"),
            *Stimulus.Type.Name.ToString(), Stimulus.Strength);

        // State transitions based on perception
        switch (CurrentState)
        {
            case EMonsterState::Patrol:
            case EMonsterState::Hiding:
                SetMonsterState(EMonsterState::Investigate);
                OnDetectedPlayer.Broadcast(Actor);
                break;

            case EMonsterState::Investigate:
                // Confirm player location, move to peek/chase
                if (IsNodeAdjacentToPlayer(CurrentNode))
                {
                    SetMonsterState(EMonsterState::Peeking);
                }
                break;

            default:
                break;
        }
    }
    else
    {
        // Lost perception
        PerceivedTarget = nullptr;
    }
}

void AMonsterBase::OnTargetLost(AActor* Actor)
{
    if (Actor == PerceivedTarget)
    {
        PerceivedTarget = nullptr;

        if (CurrentState == EMonsterState::Investigate || CurrentState == EMonsterState::Chase)
        {
            SetMonsterState(EMonsterState::Patrol);
        }
    }
}

void AMonsterBase::OnPlayerUsedCamera_Implementation(AActor* CameraActor)
{
    UE_LOG(LogTemp, Log, TEXT("MonsterBase: Player used camera %s"), CameraActor ? *CameraActor->GetName() : TEXT("NULL"));

    if (CurrentState == EMonsterState::Peeking || CurrentState == EMonsterState::Investigate)
    {
        // Player spotted us on camera - hide!
        SetMonsterState(EMonsterState::Hiding);
        MoveToNode(PreviousNode);
    }
}

void AMonsterBase::OnPlayerClosedDoor_Implementation(AActor* DoorActor)
{
    UE_LOG(LogTemp, Log, TEXT("MonsterBase: Player closed door %s"), DoorActor ? *DoorActor->GetName() : TEXT("NULL"));

    if (CurrentState == EMonsterState::Peeking)
    {
        // Door blocked us - retreat
        SetMonsterState(EMonsterState::Hiding);
        MoveToNode(PreviousNode);
    }
}

void AMonsterBase::OnPlayerMadeNoise_Implementation(const FNoiseEvent& NoiseEvent)
{
	if (bCorridorChaseMode) return;
    const FName NoiseTagName = NoiseEvent.NoiseTag.GetTagName();
    const bool bRelevantNoise = NoiseTagName == FName(TEXT("Noise.Event.ScrewTurn"))
        || NoiseTagName == FName(TEXT("Noise.Event.Footstep.Walk"))
        || NoiseTagName == FName(TEXT("Noise.Event.Footstep.Run"));
    if (!bRelevantNoise) return;

    const float EffectiveRange = NoiseEvent.HearRange * FMath::Max(0.1f, NoiseEvent.Loudness);
    if (FVector::DistSquared(GetActorLocation(), NoiseEvent.Location) > FMath::Square(EffectiveRange)) return;

    UE_LOG(LogTemp, Log, TEXT("MonsterBase: Heard noise %s at %s"), *NoiseTagName.ToString(), *NoiseEvent.Location.ToString());

    bInvestigatingNoise = true;
    ++NoiseEventsSinceQuiet;
    PerceivedTarget = NoiseEvent.Instigator;
    GetWorldTimerManager().ClearTimer(NoiseSilenceTimerHandle);
    GetWorldTimerManager().SetTimer(NoiseSilenceTimerHandle, this, &AMonsterBase::OnQuietPeriodElapsed, QuietPeriodToRetreat, false);

    if (CurrentState != EMonsterState::Peeking)
    {
        GetWorldTimerManager().ClearTimer(StateTimerHandle);
        SetMonsterState(EMonsterState::Investigate);
        GetWorldTimerManager().ClearTimer(StateTimerHandle);
    }

    if (CurrentState != EMonsterState::Peeking)
    {
        if (AMonsterAIController* AIController = Cast<AMonsterAIController>(GetController()))
        {
            if (UBlackboardComponent* BB = AIController->GetBlackboardComponent())
            {
                BB->SetValueAsVector("NoiseLocation", NoiseEvent.Location);
            }
            AIController->MoveToLocation(NoiseEvent.Location, 50.0f);
        }
    }

    if (NoiseEventsSinceQuiet >= NoiseEventsToPeek && CurrentState != EMonsterState::Peeking)
    {
        SetMonsterState(EMonsterState::Peeking);
    }
}

void AMonsterBase::HandlePlayerNoiseEvent(const FNoiseEvent& NoiseEvent)
{
    OnPlayerMadeNoise(NoiseEvent);
}

void AMonsterBase::OnQuietPeriodElapsed()
{
    bInvestigatingNoise = false;
    NoiseEventsSinceQuiet = 0;
    PerceivedTarget = nullptr;

    if (CurrentState == EMonsterState::Investigate || CurrentState == EMonsterState::Peeking)
    {
        SetMonsterState(EMonsterState::Hiding);
        if (PreviousNode)
        {
            MoveToNode(PreviousNode);
        }
    }
}

void AMonsterBase::OnPlayerStaredAtPeeker_Implementation()
{
    UE_LOG(LogTemp, Log, TEXT("MonsterBase: Player stared at Peeker"));

    if (CurrentState == EMonsterState::Peeking)
    {
        // Weeping Angel style - freeze or hide
        SetMonsterState(EMonsterState::Hiding);
        MoveToNode(PreviousNode);
    }
}

void AMonsterBase::PerformJumpscare_Implementation()
{
    UE_LOG(LogTemp, Warning, TEXT("MonsterBase: JUMPSCARE!"));

    OnJumpscare.Broadcast(this);

    APlayerBase* Player = GetPlayerPawn();
    if (Player)
    {
        // Trigger game over / restart logic
        UGameplayStatics::OpenLevel(GetWorld(), FName(*GetWorld()->GetName()));
    }
}

void AMonsterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AMonsterBase, CurrentState);
    DOREPLIFETIME(AMonsterBase, CreatureType);
}
