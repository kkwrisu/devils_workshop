#include "PrologueCompletedWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"

void UPrologueCompletedWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!WidgetTree) return;

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	BlackBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BlackBackground"));
	BlackBackground->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
	BlackBackground->SetHorizontalAlignment(HAlign_Center);
	BlackBackground->SetVerticalAlignment(VAlign_Center);
	Root->AddChild(BlackBackground);
	if (UCanvasPanelSlot* BackgroundSlot = Cast<UCanvasPanelSlot>(BlackBackground->Slot))
	{
		BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BackgroundSlot->SetOffsets(FMargin(0.0f));
	}

	CompletionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CompletionText"));
	CompletionText->SetText(FText::FromString(TEXT("Prólogo concluído!")));
	CompletionText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 48));
	CompletionText->SetJustification(ETextJustify::Center);
	CompletionText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.0f)));
	BlackBackground->SetContent(CompletionText);
	BlackBackground->SetPadding(FMargin(80.0f));
}

void UPrologueCompletedWidget::SetFadeProgress(float BackgroundAlpha, float TextAlpha)
{
	if (BlackBackground)
	{
		BlackBackground->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, FMath::Clamp(BackgroundAlpha, 0.0f, 1.0f)));
	}
	if (CompletionText)
	{
		CompletionText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, FMath::Clamp(TextAlpha, 0.0f, 1.0f))));
	}
	InvalidateLayoutAndVolatility();
}

void UPrologueCompletedWidget::FillViewport()
{
	if (UCanvasPanelSlot* ViewportCanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		ViewportCanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		ViewportCanvasSlot->SetOffsets(FMargin(0.0f));
		ViewportCanvasSlot->SetAlignment(FVector2D::ZeroVector);
	}
}

void UPrologueCompletedWidget::SetCompletionText(const FText& Text)
{
	if (CompletionText)
	{
		CompletionText->SetText(Text);
	}
}
