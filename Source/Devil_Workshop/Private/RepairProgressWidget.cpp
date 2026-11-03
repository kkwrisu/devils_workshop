#include "RepairProgressWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Animation/WidgetAnimation.h"

void URepairProgressWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Inicia escondido
    SetVisibility(ESlateVisibility::Collapsed);
}

void URepairProgressWidget::SetStep(const FRepairStep& Step)
{
    CurrentStep = Step;
    bHasCurrentStep = true;
    UpdateVisuals();
    Show();
}

void URepairProgressWidget::UpdateProgress(float Progress)
{
    if (ProgressBar)
    {
        ProgressBar->SetPercent(Progress);
    }
}

void URepairProgressWidget::Show()
{
    SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    if (FadeInAnim)
    {
        PlayAnimation(FadeInAnim);
    }
    if (PulseAnim)
    {
        PlayAnimation(PulseAnim, 0.0f, 0);
    }
}

void URepairProgressWidget::Hide()
{
    bHasCurrentStep = false;
    if (FadeOutAnim)
    {
        PlayAnimation(FadeOutAnim);
        // Esconde de verdade após animação - usar timer ou delegate
        FTimerHandle TimerHandle;
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(TimerHandle, [this]()
            {
                SetVisibility(ESlateVisibility::Collapsed);
            }, 0.3f, false);
        }
    }
    else
    {
        SetVisibility(ESlateVisibility::Collapsed);
    }
}

void URepairProgressWidget::UpdateVisuals()
{
    if (!bHasCurrentStep) return;

    if (StepNameText)
    {
        StepNameText->SetText(CurrentStep.DisplayName);
    }

    if (InstructionText)
    {
        InstructionText->SetText(CurrentStep.InstructionText);
    }

    if (ToolRequiredText && !CurrentStep.RequiredToolTag.IsNone())
    {
        ToolRequiredText->SetText(FText::Format(
            FText::FromString(TEXT("Ferramenta: {0}")),
            FText::FromName(CurrentStep.RequiredToolTag)
        ));
    }

    // ToolIconImage - você pode mapear tags para ícones num DataTable ou mapa no Blueprint
}