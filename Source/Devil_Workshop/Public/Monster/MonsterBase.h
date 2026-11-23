#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Components/CapsuleComponent.h"
#include "Monsters/MonsterTypes.h"
#include "MonsterBase.generated.h"

class UBehaviorTreeComponent;
class UBlackboardComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class APlayerBase;
class USoundBase;
class USoundAttenuation;
class UAudioComponent;

UCLASS(Abstract, Blueprintable)
class DEVIL_WORKSHOP_API AMonsterBase : public ACharacter
{
    GENERATED_BODY()

public:
    AMonsterBase();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void PossessedBy(AController* NewController) override;
    virtual void UnPossessed() override;

    // State Machine
    UFUNCTION(BlueprintCallable, Category = "Monster|State")
    void SetMonsterState(EMonsterState NewState);

    UFUNCTION(BlueprintPure, Category = "Monster|State")
    EMonsterState GetMonsterState() const { return CurrentState; }

    UFUNCTION(BlueprintCallable, Category = "Monster|Navigation")
    void SetCurrentNode(AActor* NodeActor);

    UFUNCTION(BlueprintPure, Category = "Monster|Navigation")
    AActor* GetCurrentNode() const { return CurrentNode; }

    UFUNCTION(BlueprintCallable, Category = "Monster|Navigation")
    void SetPreviousNode(AActor* NodeActor);

    UFUNCTION(BlueprintPure, Category = "Monster|Navigation")
    AActor* GetPreviousNode() const { return PreviousNode; }

    UFUNCTION(BlueprintCallable, Category = "Monster|Navigation")
    TArray<AActor*> GetAdjacentNodes() const;

    // Perception
    UFUNCTION(BlueprintCallable, Category = "Monster|Perception")
    void OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus);

    UFUNCTION(BlueprintCallable, Category = "Monster|Perception")
    void OnTargetLost(AActor* Actor);

    // Reactions from Player
    UFUNCTION(BlueprintNativeEvent, Category = "Monster|Reactions")
    void OnPlayerUsedCamera(AActor* CameraActor);

    UFUNCTION(BlueprintNativeEvent, Category = "Monster|Reactions")
    void OnPlayerClosedDoor(AActor* DoorActor);

    UFUNCTION(BlueprintNativeEvent, Category = "Monster|Reactions")
    void OnPlayerMadeNoise(const FNoiseEvent& NoiseEvent);

    UFUNCTION(BlueprintNativeEvent, Category = "Monster|Reactions")
    void OnPlayerStaredAtPeeker();

    // Jumpscare
    UFUNCTION(BlueprintNativeEvent, Category = "Monster|Jumpscare")
    void PerformJumpscare();

    // Patrol
    UFUNCTION(BlueprintCallable, Category = "Monster|Patrol")
    void ChooseNextPatrolNode();

    UFUNCTION(BlueprintCallable, Category = "Monster|Patrol")
    void MoveToNode(AActor* TargetNode);

    // Temporarily bypasses patrol/behavior-tree logic and pursues the player through navmesh.
    UFUNCTION(BlueprintCallable, Category = "Monster|Navigation")
    void StartCorridorChase(AActor* TargetActor);

    // Holds the spawned monster still while the player is shown its arrival.
    void PrepareCorridorReveal();
    bool IsCorridorChasing() const { return bCorridorChaseMode && bCorridorChaseActive; }

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|Audio")
    TObjectPtr<USoundBase> CorridorChaseSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|Audio")
    TObjectPtr<USoundBase> CorridorChaseMusic;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|Audio|Footsteps")
    TObjectPtr<USoundBase> MonsterFootstepSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|Audio|Footsteps")
    TObjectPtr<USoundAttenuation> MonsterFootstepAttenuation;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|Audio|Footsteps", meta = (ClampMin = "50.0"))
    float MonsterFootstepStrideDistance = 190.0f;

    void FadeOutCorridorChaseMusic(float FadeDuration);

protected:
    // Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAISenseConfig_Sight> SightConfig;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

public:
    // Behavior Tree
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI")
    TObjectPtr<UBehaviorTree> BehaviorTree;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI")
    TObjectPtr<UBlackboardData> BlackboardAsset;

protected:
    // Navigation Graph
    UPROPERTY()
    TArray<TObjectPtr<AActor>> NavigationNodes;

    UPROPERTY()
    TObjectPtr<AActor> CurrentNode;

    UPROPERTY()
    TObjectPtr<AActor> PreviousNode;

    UPROPERTY(EditAnywhere, Category = "Navigation")
    float NodeAdjacencyDistance = 1500.0f;

    // State
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Replicated)
    EMonsterState CurrentState = EMonsterState::Idle;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Replicated)
    ECreatureType CreatureType = ECreatureType::Peeker;

    // Perception Targets
    UPROPERTY()
    TObjectPtr<AActor> PerceivedTarget;

    UPROPERTY()
    float LastPerceptionTime = 0.0f;

    // Timers
    UPROPERTY()
    FTimerHandle StateTimerHandle;

    UPROPERTY()
    FTimerHandle PatrolTimerHandle;

    // Config
    UPROPERTY(EditAnywhere, Category = "Config")
    float PatrolWaitTimeMin = 2.0f;

    UPROPERTY(EditAnywhere, Category = "Config")
    float PatrolWaitTimeMax = 5.0f;

    UPROPERTY(EditAnywhere, Category = "Config")
    float PeekDuration = 12.0f;

    UPROPERTY(EditAnywhere, Category = "Config")
    float HidingCooldown = 3.0f;

    UPROPERTY(EditAnywhere, Category = "Config")
    float InvestigationDuration = 4.0f;

    // Delegates
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnMonsterStateChanged OnStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnMonsterDetectedPlayer OnDetectedPlayer;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnMonsterJumpscare OnJumpscare;

    // Internal
    virtual void InitializePerception();
    virtual void BuildNavigationGraph();
    virtual bool IsNodeAdjacentToPlayer(AActor* Node) const;
    virtual APlayerBase* GetPlayerPawn() const;

    UFUNCTION()
    void OnStateTimerElapsed();

    UFUNCTION()
    void OnPatrolTimerElapsed();

    virtual void HandleStateTransition(EMonsterState FromState, EMonsterState ToState);

    UFUNCTION()
    void HandlePlayerNoiseEvent(const FNoiseEvent& NoiseEvent);

    UFUNCTION()
    void OnQuietPeriodElapsed();

    FTimerHandle NoiseSilenceTimerHandle;
    int32 NoiseEventsSinceQuiet = 0;
    bool bInvestigatingNoise = false;
    bool bCorridorChaseMode = false;
    bool bCorridorChaseActive = false;
    float CorridorHazardCheckElapsed = 0.0f;
    float CorridorMovementDebugElapsed = 0.0f;
    float CorridorMovementDebugTimeRemaining = 0.0f;
    FVector LastMonsterFootstepLocation = FVector::ZeroVector;
    float MonsterFootstepDistanceAccumulator = 0.0f;
    bool bHasLastMonsterFootstepLocation = false;
    UPROPERTY()
    TObjectPtr<UAudioComponent> CorridorChaseAudio;
    UPROPERTY()
    TObjectPtr<UAudioComponent> CorridorChaseMusicAudio;

    UPROPERTY(EditAnywhere, Category = "Config|Noise", meta = (ClampMin = "1"))
    int32 NoiseEventsToPeek = 3;

    UPROPERTY(EditAnywhere, Category = "Config|Noise", meta = (ClampMin = "0.1"))
    float QuietPeriodToRetreat = 6.0f;
};
