#include "Monster/MonsterAIController.h"
#include "Monster/MonsterBase.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Enum.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Name.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"

const FName AMonsterAIController::BB_CurrentNode = FName("CurrentNode");
const FName AMonsterAIController::BB_PreviousNode = FName("PreviousNode");
const FName AMonsterAIController::BB_PerceivedTarget = FName("PerceivedTarget");
const FName AMonsterAIController::BB_MonsterState = FName("MonsterState");
const FName AMonsterAIController::BB_NoiseLocation = FName("NoiseLocation");
const FName AMonsterAIController::BB_PeekSocketName = FName("PeekSocketName");
const FName AMonsterAIController::BB_IsPeeking = FName("IsPeeking");
const FName AMonsterAIController::BB_TimeInState = FName("TimeInState");

AMonsterAIController::AMonsterAIController()
{
    BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));
    BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));

    // AI Perception Component - created here on the Controller
    AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
    SetPerceptionComponent(*AIPerceptionComponent);

    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    SightConfig->SightRadius = 1500.0f;
    SightConfig->LoseSightRadius = 1800.0f;
    SightConfig->PeripheralVisionAngleDegrees = 90.0f;
    SightConfig->SetMaxAge(5.0f);
    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
    SightConfig->AutoSuccessRangeFromLastSeenLocation = 500.0f;

    HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
    HearingConfig->HearingRange = 2000.0f;
    HearingConfig->SetMaxAge(10.0f);
    HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
    HearingConfig->DetectionByAffiliation.bDetectFriendlies = false;
    HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;

    AIPerceptionComponent->ConfigureSense(*SightConfig);
    AIPerceptionComponent->ConfigureSense(*HearingConfig);
    AIPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
}

void AMonsterAIController::BeginPlay()
{
    Super::BeginPlay();
}

void AMonsterAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    AMonsterBase* Monster = Cast<AMonsterBase>(InPawn);
    if (Monster)
    {
        if (Monster->BehaviorTree && Monster->BlackboardAsset)
        {
            StartBehaviorTree();
        }
        else if (BehaviorTree && BlackboardAsset)
        {
            StartBehaviorTree();
        }
    }
}

void AMonsterAIController::OnUnPossess()
{
    StopBehaviorTree();
    Super::OnUnPossess();
}

void AMonsterAIController::InitializePerception()
{
    if (AIPerceptionComponent)
    {
        // Re-configure with current property values
        if (SightConfig)
        {
            SightConfig->SightRadius = SightRadius;
            SightConfig->LoseSightRadius = LoseSightRadius;
            SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngleDegrees;
        }

        if (HearingConfig)
        {
            HearingConfig->HearingRange = HearingRange;
        }

        AIPerceptionComponent->ConfigureSense(*SightConfig);
        AIPerceptionComponent->ConfigureSense(*HearingConfig);
        AIPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
    }
}

void AMonsterAIController::StartBehaviorTree()
{
    if (!BehaviorTreeComponent || !BlackboardComponent) return;

    UBehaviorTree* BT = BehaviorTree;
    UBlackboardData* BB = BlackboardAsset;

    AMonsterBase* Monster = Cast<AMonsterBase>(GetPawn());
    if (Monster)
    {
        if (Monster->BehaviorTree) BT = Monster->BehaviorTree;
        if (Monster->BlackboardAsset) BB = Monster->BlackboardAsset;
    }

    if (BT && BB)
    {
        if (BlackboardComponent->InitializeBlackboard(*BB))
        {
            BehaviorTreeComponent->StartTree(*BT);
            UE_LOG(LogTemp, Log, TEXT("MonsterAIController: Behavior Tree started"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("MonsterAIController: Failed to initialize Blackboard"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("MonsterAIController: No Behavior Tree or Blackboard assigned"));
    }
}

void AMonsterAIController::StopBehaviorTree()
{
    if (BehaviorTreeComponent)
    {
        BehaviorTreeComponent->StopTree();
    }
}

UBlackboardComponent* AMonsterAIController::GetBlackboardComponent() const
{
    return BlackboardComponent;
}

UBehaviorTreeComponent* AMonsterAIController::GetBehaviorTreeComponent() const
{
    return BehaviorTreeComponent;
}

void AMonsterAIController::SetBlackboardValue(FName Key, UObject* Value)
{
    if (BlackboardComponent)
    {
        BlackboardComponent->SetValueAsObject(Key, Value);
    }
}

void AMonsterAIController::SetBlackboardVector(FName Key, FVector Value)
{
    if (BlackboardComponent)
    {
        BlackboardComponent->SetValueAsVector(Key, Value);
    }
}

void AMonsterAIController::SetBlackboardFloat(FName Key, float Value)
{
    if (BlackboardComponent)
    {
        BlackboardComponent->SetValueAsFloat(Key, Value);
    }
}

void AMonsterAIController::SetBlackboardBool(FName Key, bool Value)
{
    if (BlackboardComponent)
    {
        BlackboardComponent->SetValueAsBool(Key, Value);
    }
}

void AMonsterAIController::SetBlackboardEnum(FName Key, uint8 Value)
{
    if (BlackboardComponent)
    {
        BlackboardComponent->SetValueAsEnum(Key, Value);
    }
}