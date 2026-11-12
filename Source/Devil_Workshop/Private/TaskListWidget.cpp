#include "TaskListWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"

void UTaskListWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!WidgetTree) return;

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
	Background->SetBrushColor(FLinearColor(0.015f, 0.02f, 0.03f, 0.9f));
	Root->AddChild(Background);
	if (UCanvasPanelSlot* BackgroundSlot = Cast<UCanvasPanelSlot>(Background->Slot))
	{
		BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BackgroundSlot->SetOffsets(FMargin(0.0f));
	}

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Checklist"));
	Text->SetText(FText::FromString(
		TEXT("OBJETIVO: CONSERTAR O RÁDIO\n\n")
		TEXT("1. Desmonte o rádio: remova os parafusos, o painel traseiro e a bateria.\n\n")
		TEXT("2. Encontre uma bateria nova no cenário.\n\n")
		TEXT("3. Coloque a bateria no carregador e espere o sinal de carga concluída.\n\n")
		TEXT("4. Instale a bateria carregada no rádio.\n\n")
		TEXT("5. Feche o painel traseiro e recoloque os parafusos.\n\n\n")
		TEXT("Pressione Voltar para fechar esta lista.")));
	Text->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 26));
	Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Text->SetJustification(ETextJustify::Left);
	Text->SetAutoWrapText(true);
	Background->SetPadding(FMargin(100.0f, 80.0f));
	Background->SetContent(Text);
}
