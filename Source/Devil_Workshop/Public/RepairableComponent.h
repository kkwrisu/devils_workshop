#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemUseTarget.h"
#include "RepairStep.h"
#include "RepairableComponent.generated.h"

class USkeletalMeshComponent;
class UStaticMeshComponent;
class UParticleSystemComponent;
class UAudioComponent;
class UCurveFloat;
class APlayerBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRepairStepStarted, const FRepairStep&, Step);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRepairStepProgress, const FRepairStep&, Step, float, Progress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRepairStepCompleted, const FRepairStep&, Step);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllRepairsCompleted);

/**
 * Componente que gerencia reparo de um objeto via passos data-driven.
 * - Um único mesh (Skeletal ou Static com Morph Targets)
 * - Passos definidos em DataTable ou array no Editor
 * - Jogador equipa ferramenta, mira no objeto, segura clique para progredir
 * - Feedback visual via morph targets + partículas/sons em sockets
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class URepairableComponent : public UActorComponent, public IItemUseTarget
{
    GENERATED_BODY()

public:
    URepairableComponent();

    // --- IItemUseTarget Interface ---
    virtual bool CanUseItem_Implementation(const FInventoryItem& Item) override;
    virtual bool UseItem_Implementation(const FInventoryItem& Item, AActor* User) override;

    // --- API Pública ---
    /** Inicia/reinicia o reparo (chamado quando jogador interage com objeto) */
    UFUNCTION(BlueprintCallable, Category = "Reparo")
    void StartRepair(AActor* User);

    /** Para o reparo atual (jogador saiu da inspeção, largou ferramenta, etc.) */
    UFUNCTION(BlueprintCallable, Category = "Reparo")
    void StopRepair();

    /** Atualiza progresso do passo ativo (chamado no Tick do Player enquanto segura clique) */
    UFUNCTION(BlueprintCallable, Category = "Reparo")
    void UpdateRepairProgress(float DeltaTime);

    /** Força completar o passo ativo (para debug/teste) */
    UFUNCTION(BlueprintCallable, Category = "Reparo")
    void ForceCompleteCurrentStep();

    /** Verifica se um passo específico pode ser iniciado */
    UFUNCTION(BlueprintPure, Category = "Reparo")
    bool CanStartStep(FName StepId) const;

    /** Obtém índice do passo ativo atual (INDEX_NONE se nenhum) */
    UFUNCTION(BlueprintPure, Category = "Reparo")
    int32 GetActiveStepIndex() const;

    /** Obtém índice do próximo passo disponível (INDEX_NONE se nenhum) */
    UFUNCTION(BlueprintPure, Category = "Reparo")
    int32 GetNextAvailableStepIndex() const;

    /** Obtém referência ao passo ativo (para C++/Blueprint usar com GetActiveStepIndex) */
    UFUNCTION(BlueprintPure, Category = "Reparo")
    const FRepairStep& GetActiveStep() const;

    /** Obtém referência ao próximo passo disponível */
    UFUNCTION(BlueprintPure, Category = "Reparo")
    const FRepairStep& GetNextAvailableStep() const;

    /** Verifica se todos os passos estão completos */
    UFUNCTION(BlueprintPure, Category = "Reparo")
    bool IsFullyRepaired() const { return bIsFullyRepaired; }

    /** Reseta todo o reparo (para replay/new game) */
    UFUNCTION(BlueprintCallable, Category = "Reparo")
    void ResetRepair();

    // --- Configuração ---
    /** Mesh que receberá os morph targets (SkeletalMeshComponent recomendado) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reparo|Configuração", meta = (AllowedClasses = "/Script/Engine.SkeletalMeshComponent,/Script/Engine.StaticMeshComponent"))
    TObjectPtr<USceneComponent> TargetMeshComponent;

    /** Passos de reparo (pode usar DataTable via função abaixo) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reparo|Configuração")
    TArray<FRepairStep> RepairSteps;

    /** DataTable opcional para carregar passos (RowStruct = FRepairStep) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reparo|Configuração")
    TObjectPtr<class UDataTable> RepairStepsDataTable;

    /** Se true, carrega passos do DataTable no BeginPlay */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reparo|Configuração")
    bool bLoadFromDataTable = false;

    /** Distância máxima para interação */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reparo|Configuração", meta = (ClampMin = "0"))
    float InteractionDistance = 200.0f;

    /** Permite reparar sem estar em modo inspeção (apenas proximidade + ferramenta) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reparo|Configuração")
    bool bRequireInspectionMode = true;

    // --- Eventos (para UI, animações, etc.) ---
    UPROPERTY(BlueprintAssignable, Category = "Reparo|Eventos")
    FOnRepairStepStarted OnStepStarted;

    UPROPERTY(BlueprintAssignable, Category = "Reparo|Eventos")
    FOnRepairStepProgress OnStepProgress;

    UPROPERTY(BlueprintAssignable, Category = "Reparo|Eventos")
    FOnRepairStepCompleted OnStepCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Reparo|Eventos")
    FOnAllRepairsCompleted OnAllRepairsCompleted;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Carrega passos do DataTable se configurado
    void LoadStepsFromDataTable();

    // Valida se todos pré-requisitos de um passo estão completos
    bool ArePrerequisitesMet(const FRepairStep& Step) const;

    // Inicia um passo específico
    bool BeginStep(FName StepId);

    // Completa o passo ativo
    void CompleteActiveStep();

    // Avança para o próximo passo disponível
    void AdvanceToNextStep();

    // Aplica morph target no mesh alvo
    void ApplyMorphTarget(const FRepairStep& Step, float Progress);

    // Spawna partícula/som no socket
    void SpawnEffectAtSocket(const FRepairStep& Step, FName SocketName, TObjectPtr<UParticleSystem> Particle, TObjectPtr<USoundBase> Sound);
    void PlayWorkLoopSound(const FRepairStep& Step);
    void StopWorkLoopSound();

    // Encontra componente de mesh (Skeletal ou Static) para morph targets
    USkeletalMeshComponent* GetSkeletalMesh() const;
    UStaticMeshComponent* GetStaticMesh() const;

    // Helper: obtém referência mutável ao passo ativo (ou nullptr)
    FRepairStep* GetActiveStepMutable();
    // Helper: obtém referência constante ao passo ativo (ou nullptr)
    const FRepairStep* GetActiveStepConst() const;

    // Referência ao jogador que está reparando
    UPROPERTY()
    TWeakObjectPtr<APlayerBase> CurrentUser;

    // Índice do passo ativo no array (INDEX_NONE se nenhum)
    int32 ActiveStepIndex = INDEX_NONE;

    // Estado
    UPROPERTY()
    bool bIsRepairing = false;

    UPROPERTY()
    bool bIsFullyRepaired = false;

    // Áudio loop de trabalho
    UPROPERTY()
    TObjectPtr<UAudioComponent> WorkLoopAudioComponent;

    // Para evitar spam de log
    mutable float LastLogTime = 0.0f;
};