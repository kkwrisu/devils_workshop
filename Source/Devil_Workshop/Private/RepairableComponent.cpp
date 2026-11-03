#include "RepairableComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Engine/DataTable.h"
#include "PlayerBase.h"
#include "InventoryComponent.h"
#include "InventoryTypes.h"

URepairableComponent::URepairableComponent()
{
    PrimaryComponentTick.bCanEverTick = false; // Atualizado manualmente via UpdateRepairProgress
}

void URepairableComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bLoadFromDataTable && RepairStepsDataTable)
    {
        LoadStepsFromDataTable();
    }

    // Validação básica
    if (RepairSteps.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("RepairableComponent [%s]: Nenhum passo de reparo configurado!"), *GetOwner()->GetName());
    }

    // Cache do mesh para morph targets
    if (!TargetMeshComponent)
    {
        // Tenta achar automaticamente
        TargetMeshComponent = GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
        if (!TargetMeshComponent)
        {
            TargetMeshComponent = GetOwner()->FindComponentByClass<UStaticMeshComponent>();
        }
    }
}

void URepairableComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopRepair();
    Super::EndPlay(EndPlayReason);
}

void URepairableComponent::LoadStepsFromDataTable()
{
    if (!RepairStepsDataTable) return;

    TArray<FRepairStep*> OutRows;
    RepairStepsDataTable->GetAllRows<FRepairStep>(TEXT(""), OutRows);

    RepairSteps.Reset();
    RepairSteps.Reserve(OutRows.Num());

    for (FRepairStep* Row : OutRows)
    {
        if (Row)
        {
            RepairSteps.Add(*Row);
        }
    }

    // Ordena por StepId se quiser (opcional, DataTable já vem ordenado)
    UE_LOG(LogTemp, Log, TEXT("RepairableComponent [%s]: Carregados %d passos do DataTable"), *GetOwner()->GetName(), RepairSteps.Num());
}

// Helper: obtém referência ao passo ativo (com verificação)
FRepairStep* URepairableComponent::GetActiveStepMutable()
{
    if (ActiveStepIndex != INDEX_NONE && RepairSteps.IsValidIndex(ActiveStepIndex))
    {
        return &RepairSteps[ActiveStepIndex];
    }
    return nullptr;
}

const FRepairStep* URepairableComponent::GetActiveStepConst() const
{
    if (ActiveStepIndex != INDEX_NONE && RepairSteps.IsValidIndex(ActiveStepIndex))
    {
        return &RepairSteps[ActiveStepIndex];
    }
    return nullptr;
}

bool URepairableComponent::CanUseItem_Implementation(const FInventoryItem& Item)
{
    if (bIsFullyRepaired) return false;
    if (!Item.Data) return false;

    // Se já está reparando, só permite continuar com a mesma ferramenta
    if (bIsRepairing)
    {
        const FRepairStep* ActiveStep = GetActiveStepConst();
        if (ActiveStep)
        {
            return Item.Data->HasTag(ActiveStep->RequiredToolTag);
        }
    }

    // Se não está reparando, verifica se há algum passo disponível para esta ferramenta
    for (const FRepairStep& Step : RepairSteps)
    {
        if (Step.bIsCompleted) continue;
        if (!ArePrerequisitesMet(Step)) continue;
        if (Item.Data->HasTag(Step.RequiredToolTag))
        {
            return true;
        }
    }

    return false;
}

bool URepairableComponent::UseItem_Implementation(const FInventoryItem& Item, AActor* User)
{
    APlayerBase* Player = Cast<APlayerBase>(User);
    if (!Player) return false;

    // Se já está reparando com este passo, apenas continua (o Tick do player vai chamar UpdateRepairProgress)
    if (bIsRepairing)
    {
        const FRepairStep* ActiveStep = GetActiveStepConst();
        if (ActiveStep && Item.Data->HasTag(ActiveStep->RequiredToolTag))
        {
            CurrentUser = Player;
            return false; // Não consome o item
        }
    }

    // Tenta iniciar um novo passo compatível
    for (int32 i = 0; i < RepairSteps.Num(); ++i)
    {
        FRepairStep& Step = RepairSteps[i];
        if (Step.bIsCompleted) continue;
        if (!ArePrerequisitesMet(Step)) continue;
        if (!Item.Data->HasTag(Step.RequiredToolTag)) continue;

        CurrentUser = Player;
        if (BeginStep(Step.StepId))
        {
            return false; // Não consome o item
        }
    }

    return false;
}

void URepairableComponent::StartRepair(AActor* User)
{
    APlayerBase* Player = Cast<APlayerBase>(User);
    if (!Player) return;

    if (bIsFullyRepaired) return;
    if (bIsRepairing) return; // Já está reparando

    CurrentUser = Player;
    // Não inicia passo automaticamente - espera UseItem (clique do jogador)
    // Isso permite que o jogador escolha qual ferramenta usar se houver múltiplas opções
}

void URepairableComponent::StopRepair()
{
    FRepairStep* ActiveStep = GetActiveStepMutable();
    if (ActiveStep)
    {
        StopWorkLoopSound();
    }

    bIsRepairing = false;
    ActiveStepIndex = INDEX_NONE;
    CurrentUser.Reset();
}

void URepairableComponent::UpdateRepairProgress(float DeltaTime)
{
    if (!bIsRepairing || !CurrentUser.IsValid()) return;

    FRepairStep* ActiveStep = GetActiveStepMutable();
    if (!ActiveStep) return;

    // Verifica se o jogador ainda está segurando o clique
    if (!CurrentUser->IsClickHeld())
    {
        // Jogador soltou - para o progresso (ou persiste se configurado)
        if (!ActiveStep->bPersistProgress)
        {
            ActiveStep->CurrentProgress = 0.0f;
            ApplyMorphTarget(*ActiveStep, 0.0f);
        }
        StopWorkLoopSound();
        return;
    }

    // Verifica se ainda tem a ferramenta certa equipada
    if (CurrentUser->Inventory)
    {
        FInventoryItem Equipped;
        if (!CurrentUser->Inventory->GetEquippedItem(Equipped) || !Equipped.Data || !Equipped.Data->HasTag(ActiveStep->RequiredToolTag))
        {
            // Ferramenta errada - para
            if (!ActiveStep->bPersistProgress)
            {
                ActiveStep->CurrentProgress = 0.0f;
                ApplyMorphTarget(*ActiveStep, 0.0f);
            }
            StopWorkLoopSound();
            return;
        }
    }

    // Avança progresso
    const float PrevProgress = ActiveStep->CurrentProgress;
    ActiveStep->CurrentProgress = FMath::Min(1.0f, ActiveStep->CurrentProgress + DeltaTime / ActiveStep->WorkDuration);

    // Aplica curva de progresso visual se houver
    float VisualProgress = ActiveStep->CurrentProgress;
    if (ActiveStep->ProgressCurve)
    {
        VisualProgress = ActiveStep->ProgressCurve->GetFloatValue(ActiveStep->CurrentProgress);
    }

    // Aplica morph target
    ApplyMorphTarget(*ActiveStep, VisualProgress);

    // Inicia som de loop se não estiver tocando
    if (PrevProgress == 0.0f && ActiveStep->CurrentProgress > 0.0f)
    {
        PlayWorkLoopSound(*ActiveStep);
    }

    // Broadcast progresso
    OnStepProgress.Broadcast(*ActiveStep, ActiveStep->CurrentProgress);

    // Verifica conclusão
    if (ActiveStep->CurrentProgress >= 1.0f)
    {
        CompleteActiveStep();
    }
}

void URepairableComponent::ForceCompleteCurrentStep()
{
    FRepairStep* ActiveStep = GetActiveStepMutable();
    if (ActiveStep)
    {
        ActiveStep->CurrentProgress = 1.0f;
        ApplyMorphTarget(*ActiveStep, 1.0f);
        CompleteActiveStep();
    }
}

bool URepairableComponent::CanStartStep(FName StepId) const
{
    for (const FRepairStep& Step : RepairSteps)
    {
        if (Step.StepId == StepId)
        {
            if (Step.bIsCompleted) return false;
            return ArePrerequisitesMet(Step);
        }
    }
    return false;
}

int32 URepairableComponent::GetActiveStepIndex() const
{
    return ActiveStepIndex;
}

int32 URepairableComponent::GetNextAvailableStepIndex() const
{
    for (int32 i = 0; i < RepairSteps.Num(); ++i)
    {
        const FRepairStep& Step = RepairSteps[i];
        if (Step.bIsCompleted) continue;
        if (ArePrerequisitesMet(Step))
        {
            return i;
        }
    }
    return INDEX_NONE;
}

const FRepairStep& URepairableComponent::GetActiveStep() const
{
    static FRepairStep EmptyStep;
    const FRepairStep* ActiveStep = GetActiveStepConst();
    return ActiveStep ? *ActiveStep : EmptyStep;
}

const FRepairStep& URepairableComponent::GetNextAvailableStep() const
{
    static FRepairStep EmptyStep;
    const int32 NextIndex = GetNextAvailableStepIndex();
    if (NextIndex != INDEX_NONE && RepairSteps.IsValidIndex(NextIndex))
    {
        return RepairSteps[NextIndex];
    }
    return EmptyStep;
}

void URepairableComponent::ResetRepair()
{
    StopRepair();

    for (FRepairStep& Step : RepairSteps)
    {
        Step.CurrentProgress = 0.0f;
        Step.bIsCompleted = false;
        Step.bIsActive = false;
    }

    bIsFullyRepaired = false;

    // Reset morph targets
    if (TargetMeshComponent)
    {
        USkeletalMeshComponent* SkelMesh = GetSkeletalMesh();
        if (SkelMesh)
        {
            for (const FRepairStep& Step : RepairSteps)
            {
                if (!Step.MorphTargetName.IsNone())
                {
                    SkelMesh->SetMorphTarget(Step.MorphTargetName, 0.0f);
                }
            }
        }
    }
}

void URepairableComponent::ApplyMorphTarget(const FRepairStep& Step, float Progress)
{
    if (Step.MorphTargetName.IsNone()) return;

    USkeletalMeshComponent* SkelMesh = GetSkeletalMesh();
    if (SkelMesh)
    {
        const float Value = FMath::Lerp(0.0f, Step.MorphTargetEndValue, Progress);
        SkelMesh->SetMorphTarget(Step.MorphTargetName, Value);
    }
    // Para StaticMesh com morph targets, precisaria de uma abordagem diferente (Material Parameter Collection ou Niagara)
}

bool URepairableComponent::ArePrerequisitesMet(const FRepairStep& Step) const
{
    for (const FName& PrereqId : Step.PrerequisiteStepIds)
    {
        bool bFound = false;
        for (const FRepairStep& OtherStep : RepairSteps)
        {
            if (OtherStep.StepId == PrereqId)
            {
                if (!OtherStep.bIsCompleted) return false;
                bFound = true;
                break;
            }
        }
        if (!bFound)
        {
            UE_LOG(LogTemp, Warning, TEXT("RepairableComponent [%s]: Pré-requisito '%s' não encontrado para passo '%s'"), *GetOwner()->GetName(), *PrereqId.ToString(), *Step.StepId.ToString());
            return false;
        }
    }
    return true;
}

bool URepairableComponent::BeginStep(FName StepId)
{
    for (int32 i = 0; i < RepairSteps.Num(); ++i)
    {
        FRepairStep& Step = RepairSteps[i];
        if (Step.StepId == StepId)
        {
            if (Step.bIsCompleted) return false;
            if (!ArePrerequisitesMet(Step)) return false;

            // Verifica ferramenta
            if (CurrentUser.IsValid() && CurrentUser->Inventory)
            {
                FInventoryItem Equipped;
                if (!CurrentUser->Inventory->GetEquippedItem(Equipped) || !Equipped.Data || !Equipped.Data->HasTag(Step.RequiredToolTag))
                {
                    return false;
                }
            }

            // Inicia passo
            ActiveStepIndex = i;
            Step.bIsActive = true;
            Step.CurrentProgress = Step.bPersistProgress ? Step.CurrentProgress : 0.0f;
            bIsRepairing = true;

            // Aplica morph target inicial
            ApplyMorphTarget(Step, Step.CurrentProgress);

            // Efeitos de início
            if (!Step.EffectSocketName.IsNone())
            {
                SpawnEffectAtSocket(Step, Step.EffectSocketName, Step.StartParticle, Step.StartSound);
            }

            // Broadcast
            OnStepStarted.Broadcast(Step);

            UE_LOG(LogTemp, Log, TEXT("RepairableComponent [%s]: Iniciado passo '%s' (%s)"), *GetOwner()->GetName(), *Step.StepId.ToString(), *Step.DisplayName.ToString());
            return true;
        }
    }
    return false;
}

void URepairableComponent::CompleteActiveStep()
{
    FRepairStep* ActiveStep = GetActiveStepMutable();
    if (!ActiveStep) return;

    FRepairStep CompletedStep = *ActiveStep; // Copia para broadcast
    const FName CompletedSocket = CompletedStep.EffectSocketName;

    // Marca como completo
    ActiveStep->bIsCompleted = true;
    ActiveStep->CurrentProgress = 1.0f;
    ActiveStep->bIsActive = false;
    ApplyMorphTarget(*ActiveStep, 1.0f);

    // Efeitos de conclusão
    if (!CompletedSocket.IsNone())
    {
        SpawnEffectAtSocket(CompletedStep, CompletedSocket, CompletedStep.CompleteParticle, CompletedStep.CompleteSound);
    }

    StopWorkLoopSound();

    // Broadcast
    OnStepCompleted.Broadcast(CompletedStep);

    UE_LOG(LogTemp, Log, TEXT("RepairableComponent [%s]: Completado passo '%s'"), *GetOwner()->GetName(), *CompletedStep.StepId.ToString());

    // Avança para próximo
    AdvanceToNextStep();
}

void URepairableComponent::AdvanceToNextStep()
{
    ActiveStepIndex = INDEX_NONE;
    bIsRepairing = false;

    // Procura próximo passo disponível
    for (int32 i = 0; i < RepairSteps.Num(); ++i)
    {
        FRepairStep& Step = RepairSteps[i];
        if (Step.bIsCompleted) continue;
        if (!ArePrerequisitesMet(Step)) continue;

        // Próximo passo encontrado - mas NÃO inicia automaticamente
        // Espera o jogador clicar novamente com a ferramenta certa
        UE_LOG(LogTemp, Log, TEXT("RepairableComponent [%s]: Próximo passo disponível: '%s' (requer %s)"), *GetOwner()->GetName(), *Step.StepId.ToString(), *Step.RequiredToolTag.ToString());
        return;
    }

    // Nenhum passo restante - reparo completo!
    bIsFullyRepaired = true;
    OnAllRepairsCompleted.Broadcast();

    UE_LOG(LogTemp, Log, TEXT("RepairableComponent [%s]: REPARO COMPLETO!"), *GetOwner()->GetName());
}

void URepairableComponent::SpawnEffectAtSocket(const FRepairStep& Step, FName SocketName, TObjectPtr<UParticleSystem> Particle, TObjectPtr<USoundBase> Sound)
{
    if (!TargetMeshComponent) return;

    USceneComponent* MeshComp = TargetMeshComponent;
    if (!MeshComp->DoesSocketExist(SocketName))
    {
        UE_LOG(LogTemp, Warning, TEXT("RepairableComponent [%s]: Socket '%s' não existe no mesh!"), *GetOwner()->GetName(), *SocketName.ToString());
        return;
    }

    const FTransform SocketTransform = MeshComp->GetSocketTransform(SocketName, RTS_World);

    if (Particle)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Particle, SocketTransform);
    }

    if (Sound)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), Sound, SocketTransform.GetLocation());
    }
}

void URepairableComponent::PlayWorkLoopSound(const FRepairStep& Step)
{
    if (!Step.WorkLoopSound || !TargetMeshComponent) return;

    if (!TargetMeshComponent->DoesSocketExist(Step.EffectSocketName))
    {
        return;
    }

    const FTransform SocketTransform = TargetMeshComponent->GetSocketTransform(Step.EffectSocketName, RTS_World);

    WorkLoopAudioComponent = UGameplayStatics::SpawnSoundAttached(
        Step.WorkLoopSound,
        TargetMeshComponent,
        Step.EffectSocketName,
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        EAttachLocation::SnapToTarget,
        true // bStopWhenAttachedToDestroyed
    );
}

void URepairableComponent::StopWorkLoopSound()
{
    if (WorkLoopAudioComponent && WorkLoopAudioComponent->IsPlaying())
    {
        WorkLoopAudioComponent->Stop();
        WorkLoopAudioComponent = nullptr;
    }
}

USkeletalMeshComponent* URepairableComponent::GetSkeletalMesh() const
{
    if (!TargetMeshComponent) return nullptr;
    return Cast<USkeletalMeshComponent>(TargetMeshComponent);
}

UStaticMeshComponent* URepairableComponent::GetStaticMesh() const
{
    if (!TargetMeshComponent) return nullptr;
    return Cast<UStaticMeshComponent>(TargetMeshComponent);
}