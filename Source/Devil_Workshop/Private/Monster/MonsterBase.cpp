#include "Monster/MonsterBase.h"
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
#include "Monsters/MonsterTypes.h"

AMonsterBase::AMonsterBase()
{
    PrimaryActorTick.bCanEverTick = true;

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

    // Start in Patrol state after a short delay
    FTimerHandle InitialTimer;
    GetWorldTimerManager().SetTimer(InitialTimer, [this]()
    {
        if (HasAuthority())
        {
            SetMonsterState(EMonsterState::Patrol);
        }
    }, 1.0f, false);
}

void AMonsterBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

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

    TArray<AActor*> ClickableActors;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Clickable"), ClickableActors);

    for (AActor* Actor : ClickableActors)
    {
        if (Actor && Actor != this)
        {
            NavigationNodes.Add(Actor);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("MonsterBase: Built navigation graph with %d nodes"), NavigationNodes.Num());
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

    return Node == Player->CurrentAreaActor;
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
    UE_LOG(LogTemp, Log, TEXT("MonsterBase: Heard noise %s at %s"), *NoiseEvent.NoiseTag.ToString(), *NoiseEvent.Location.ToString());

    if (CurrentState == EMonsterState::Patrol || CurrentState == EMonsterState::Hiding)
    {
        // Investigate noise location
        SetMonsterState(EMonsterState::Investigate);

        if (AMonsterAIController* AIController = Cast<AMonsterAIController>(GetController()))
        {
            AIController->GetBlackboardComponent()->SetValueAsVector("NoiseLocation", NoiseEvent.Location);
            AIController->MoveToLocation(NoiseEvent.Location, 50.0f);
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