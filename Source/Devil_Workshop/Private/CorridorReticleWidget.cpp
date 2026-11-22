#include "CorridorReticleWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"

void UCorridorReticleWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ReticleCanvas"));
	WidgetTree->RootWidget = Canvas;

	ReticleSymbol = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ReticleSymbol"));
	ReticleSymbol->SetText(FText::FromString(TEXT("+")));
	ReticleSymbol->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = ReticleSymbol->GetFont();
	Font.Size = 20;
	ReticleSymbol->SetFont(Font);
	ReticleSymbol->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.82f, 0.84f, 0.8f)));

	if (UCanvasPanelSlot* ReticleCanvasSlot = Canvas->AddChildToCanvas(ReticleSymbol))
	{
		ReticleCanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		ReticleCanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		ReticleCanvasSlot->SetPosition(FVector2D::ZeroVector);
		ReticleCanvasSlot->SetSize(FVector2D(24.0f, 24.0f));
	}
}

void UCorridorReticleWidget::SetHasInteractableTarget(bool bHasTarget)
{
	if (!ReticleSymbol)
	{
		return;
	}

	const FLinearColor Color = bHasTarget
		? FLinearColor(1.0f, 0.68f, 0.16f, 1.0f)
		: FLinearColor(0.78f, 0.82f, 0.84f, 0.8f);
	ReticleSymbol->SetColorAndOpacity(FSlateColor(Color));
}
