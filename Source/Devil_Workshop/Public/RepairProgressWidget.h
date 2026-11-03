#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RepairStep.h"
#include "RepairProgressWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UImage;
class UWidgetAnimation;

UCLASS(Blueprintable)
class URepairProgressWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Atualiza o widget com dados do passo atual */
    UFUNCTION(BlueprintCallable, Category = "Reparo")
    void SetStep(const FRepairStep& Step);

    /** Atualiza apenas a barra de progresso (chamado todo frame) */
    UFUNCTION(BlueprintCallable, Category = "Reparo")
    void UpdateProgress(float Progress);

    /** Mostra o widget com animação */
    UFUNCTION(BlueprintCallable, Category = "Reparo")
    void Show();

    /** Esconde o widget com animação */
    UFUNCTION(BlueprintCallable, Category = "Reparo")
    void Hide();

protected:
    virtual void NativeConstruct() override;

    // Widgets do Designer (BindWidget)
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UProgressBar> ProgressBar;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> StepNameText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> InstructionText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ToolRequiredText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> ToolIconImage;

    // Animações
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animações")
    TObjectPtr<UWidgetAnimation> FadeInAnim;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animações")
    TObjectPtr<UWidgetAnimation> FadeOutAnim;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animações")
    TObjectPtr<UWidgetAnimation> PulseAnim;

private:
    FRepairStep CurrentStep;
    bool bHasCurrentStep = false;

    void UpdateVisuals();
};