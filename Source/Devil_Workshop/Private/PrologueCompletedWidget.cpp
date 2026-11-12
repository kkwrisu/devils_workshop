#include "PrologueCompletedWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Styling/CoreStyle.h"

void UPrologueCompletedWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!WidgetTree) return;

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	BlackBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BlackBackground"));
	BlackBackground->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
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

void UPrologueCompletedWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	ElapsedTime += InDeltaTime;
	const float BackgroundAlpha = FMath::Clamp(ElapsedTime / FadeDuration, 0.0f, 1.0f);
	const float TextAlpha = FMath::Clamp((ElapsedTime - TextFadeDelay) / TextFadeDuration, 0.0f, 1.0f);

	if (BlackBackground)
	{
		BlackBackground->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, BackgroundAlpha));
	}
	if (CompletionText)
	{
		CompletionText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, TextAlpha)));
	}

	if (!bPausedAtEnd && BackgroundAlpha >= 1.0f && TextAlpha >= 1.0f)
	{
		bPausedAtEnd = UGameplayStatics::SetGamePaused(this, true);
	}
}
