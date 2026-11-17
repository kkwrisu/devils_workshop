#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "MonsterAIController.generated.h"

class UBehaviorTreeComponent;
class UBlackboardComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;

UCLASS(Blueprintable)
class DEVIL_WORKSHOP_API AMonsterAIController : public AAIController
{
    GENERATED_BODY()

public:
    AMonsterAIController();

    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;

    UFUNCTION(BlueprintCallable, Category = "AI")
    void StartBehaviorTree();

    UFUNCTION(BlueprintCallable, Category = "AI")
    void StopBehaviorTree();

    UFUNCTION(BlueprintPure, Category = "AI")
    UBlackboardComponent* GetBlackboardComponent() const;

    UFUNCTION(BlueprintPure, Category = "AI")
    UBehaviorTreeComponent* GetBehaviorTreeComponent() const;

    UFUNCTION(BlueprintCallable, Category = "AI")
    void SetBlackboardValue(FName Key, UObject* Value);

    UFUNCTION(BlueprintCallable, Category = "AI")
    void SetBlackboardVector(FName Key, FVector Value);

    UFUNCTION(BlueprintCallable, Category = "AI")
    void SetBlackboardFloat(FName Key, float Value);

    UFUNCTION(BlueprintCallable, Category = "AI")
    void SetBlackboardBool(FName Key, bool Value);

    UFUNCTION(BlueprintCallable, Category = "AI")
    void SetBlackboardEnum(FName Key, uint8 Value);

    // Perception Config
    UPROPERTY(EditDefaultsOnly, Category = "AI|Perception")
    float SightRadius = 1500.0f;

    UPROPERTY(EditDefaultsOnly, Category = "AI|Perception")
    float LoseSightRadius = 1800.0f;

    UPROPERTY(EditDefaultsOnly, Category = "AI|Perception")
    float PeripheralVisionAngleDegrees = 90.0f;

    UPROPERTY(EditDefaultsOnly, Category = "AI|Perception")
    float HearingRange = 2000.0f;

    // Blackboard Keys
    static const FName BB_CurrentNode;
    static const FName BB_PreviousNode;
    static const FName BB_PerceivedTarget;
    static const FName BB_MonsterState;
    static const FName BB_NoiseLocation;
    static const FName BB_PeekSocketName;
    static const FName BB_IsPeeking;
    static const FName BB_TimeInState;

protected:
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    TObjectPtr<UBehaviorTree> BehaviorTree;

    UPROPERTY(EditDefaultsOnly, Category = "AI")
    TObjectPtr<UBlackboardData> BlackboardAsset;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UBehaviorTreeComponent> BehaviorTreeComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UBlackboardComponent> BlackboardComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAISenseConfig_Sight> SightConfig;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

    virtual void InitializePerception();
};