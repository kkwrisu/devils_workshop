#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "RepairStep.generated.h"

class UParticleSystem;
class USoundBase;
class UCurveFloat;

/**
 * Define um único passo de reparo (ex.: "Remover parafuso 1", "Abrir painel traseiro").
 * Tudo é configurado via DataTable ou array no Editor - zero atores extras.
 */
USTRUCT(BlueprintType)
struct FRepairStep : public FTableRowBase
{
    GENERATED_BODY()

    /** ID único do passo (usado para pré-requisitos e save/load) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reparo|Identidade")
    FName StepId;

    /** Nome mostrado ao jogador */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reparo|Identidade")
    FText DisplayName;

    /** Descrição/instrução mostrada durante o passo */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reparo|Identidade")
    FText InstructionText;

    /** Tag que a ferramenta equipada precisa ter (ex.: "ChaveFenda", "Alicate") */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reparo|Requisitos")
    FName RequiredToolTag;

    /** Flag obrigatoria no item equipado (ex.: "Carregada" para bateria carregada). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reparo|Requisitos")
    FName RequiredItemFlag;

    /** Consome o item equipado ao concluir este passo (ex.: bateria instalada no radio). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reparo|Requisitos")
    bool bConsumeRequiredItemOnComplete = false;

    /** Passos que devem ser completados antes deste (ordem implícita) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reparo|Requisitos")
    TArray<FName> PrerequisiteStepIds;

    /** Segundos segurando o clique para completar */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reparo|Timing", meta = (ClampMin = "0.1"))
    float WorkDuration = 1.5f;

    /** Conclui o passo no clique, sem exigir que o jogador segure o botao. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reparo|Timing")
    bool bCompleteImmediatelyOnUse = false;

    /** Se true, o jogador pode soltar e o progresso permanece (não reseta) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reparo|Timing")
    bool bPersistProgress = false;

    // --- Feedback Visual (no mesh único do objeto) ---

    /** Nome do Morph Target a animar (0→1 durante o progresso). Ex.: "Morph_Screw1_Out", "Morph_BackPanel_Open" */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reparo|Visual")
    FName MorphTargetName;

    /** Valor inicial do morph target (padrao 0.0; use 1.0 para animar no sentido inverso). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reparo|Visual", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MorphTargetStartValue = 0.0f;

    /** Valor final do morph target (padrão 1.0) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reparo|Visual", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MorphTargetEndValue = 1.0f;

    /** Restaura <MorphTargetName>_Hide para 0 ao iniciar o passo. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reparo|Visual")
    bool bRestoreHiddenMorphOnStart = false;

    /** Mantem o comportamento de colapsar a peça ao concluir, usando <MorphTargetName>_Hide. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reparo|Visual")
    bool bHideMorphOnComplete = true;

    /** Socket no mesh para spawnar partícula/som no local certo. Ex.: "Socket_Screw1", "Socket_BackPanel" */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reparo|Visual")
    FName EffectSocketName;

    /** Partícula spawnada no socket ao iniciar o passo (opcional) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reparo|Visual")
    TObjectPtr<UParticleSystem> StartParticle;

    /** Partícula spawnada no socket ao completar (opcional) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reparo|Visual")
    TObjectPtr<UParticleSystem> CompleteParticle;

    /** Som tocado no socket ao iniciar (opcional) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reparo|Audio")
    TObjectPtr<USoundBase> StartSound;

    /** Som tocado no socket ao completar (opcional) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reparo|Audio")
    TObjectPtr<USoundBase> CompleteSound;

    /** Som de "trabalhando" em loop enquanto segura (opcional) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reparo|Audio")
    TObjectPtr<USoundBase> WorkLoopSound;

    // --- Feedback de Curva (opcional, para animações não-lineares) ---
    /** Curva de progresso visual (X = tempo normalizado 0-1, Y = progresso visual 0-1). Linear se nulo. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reparo|Visual")
    TObjectPtr<UCurveFloat> ProgressCurve;

    // --- Estado runtime (não salvo, preenchido pelo componente) ---
    UPROPERTY(Transient)
    float CurrentProgress = 0.0f;

    UPROPERTY(Transient)
    bool bIsCompleted = false;

    UPROPERTY(Transient)
    bool bIsActive = false;
};
