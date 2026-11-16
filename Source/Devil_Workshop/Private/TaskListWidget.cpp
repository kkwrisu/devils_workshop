#include "TaskListWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "Widgets/SWidget.h"

namespace
{
	void SetTextOnlyButtonStyle(UButton* Button)
	{
		if (!Button) return;
		FButtonStyle Style = Button->GetStyle();
		Style.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.Hovered.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.Pressed.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
		Button->SetStyle(Style);
	}
}

void UTaskListWidget::InitializeChecklist(const TArray<FRepairStep>& Steps)
{
	ChecklistEntries.Reset();
	CompletedStepIds.Reset();
	TSet<FName> ConsumedStepIds;
	for (const FRepairStep& Step : Steps)
	{
		if (Step.bIsCompleted)
		{
			CompletedStepIds.Add(Step.StepId);
		}
	}

	auto AddGroupedEntry = [this, &Steps, &ConsumedStepIds](FName Key, const TCHAR* Label, TFunction<bool(const FRepairStep&)> Matches)
	{
		FChecklistEntry Entry;
		Entry.Key = Key;
		Entry.Label = FText::FromString(Label);
		for (const FRepairStep& Step : Steps)
		{
			if (!Matches(Step)) continue;
			Entry.SourceStepIds.Add(Step.StepId);
			ConsumedStepIds.Add(Step.StepId);
		}
		if (Entry.SourceStepIds.IsEmpty()) return;

		Entry.bCompleted = !Entry.SourceStepIds.ContainsByPredicate([this](FName StepId)
		{
			return !CompletedStepIds.Contains(StepId);
		});
		ChecklistEntries.Add(MoveTemp(Entry));
	};

	AddGroupedEntry(TEXT("RemoveScrews"), TEXT("Parafusos \u2014 retirar"), [](const FRepairStep& Step)
	{
		return Step.StepId.ToString().StartsWith(TEXT("RemoveScrew"));
	});
	AddGroupedEntry(TEXT("OpenRadio"), TEXT("Carca\u00E7a aberta"), [](const FRepairStep& Step)
	{
		return Step.StepId == FName(TEXT("RemoveBackPanel"));
	});
	AddGroupedEntry(TEXT("RemoveOldBattery"), TEXT("Retirar a bateria antiga"), [](const FRepairStep& Step)
	{
		return Step.StepId == FName(TEXT("RemoveBattery"));
	});
	AddGroupedEntry(TEXT("InstallChargedBattery"), TEXT("Instalar a bateria carregada"), [](const FRepairStep& Step)
	{
		return Step.StepId == FName(TEXT("InstallBattery"));
	});
	AddGroupedEntry(TEXT("CloseBackPanel"), TEXT("Carca\u00E7a fechada"), [](const FRepairStep& Step)
	{
		return Step.StepId == FName(TEXT("InstallBackPanel"));
	});
	AddGroupedEntry(TEXT("InstallScrews"), TEXT("Parafusos \u2014 repor"), [](const FRepairStep& Step)
	{
		return Step.StepId.ToString().StartsWith(TEXT("InstallScrew"));
	});

	for (const FRepairStep& Step : Steps)
	{
		if (ConsumedStepIds.Contains(Step.StepId)) continue;
		FChecklistEntry Entry;
		Entry.Key = Step.StepId;
		Entry.Label = Step.DisplayName.IsEmpty() ? FText::FromName(Step.StepId) : Step.DisplayName;
		Entry.SourceStepIds.Add(Step.StepId);
		Entry.bCompleted = CompletedStepIds.Contains(Step.StepId);
		ChecklistEntries.Add(MoveTemp(Entry));
	}

	RefreshStepRows();
}

void UTaskListWidget::MarkStepCompleted(FName StepId)
{
	CompletedStepIds.Add(StepId);
	for (FChecklistEntry& Entry : ChecklistEntries)
	{
		if (!Entry.SourceStepIds.Contains(StepId)) continue;
		Entry.bCompleted = !Entry.SourceStepIds.ContainsByPredicate([this](FName RequiredStepId)
		{
			return !CompletedStepIds.Contains(RequiredStepId);
		});
	}
	RefreshStepRows();
}

void UTaskListWidget::RefreshStepRows()
{
	for (const FChecklistEntry& Entry : ChecklistEntries)
	{
		TObjectPtr<UTextBlock>* RowPtr = StepRows.Find(Entry.Key);
		if (!RowPtr || !RowPtr->Get()) continue;

		const TCHAR* Mark = Entry.bCompleted ? TEXT("\u2713") : TEXT("\u25CB");
		const FString RowText = FString::Printf(TEXT("%s  %s"), Mark, *Entry.Label.ToString());
		RowPtr->Get()->SetText(FText::FromString(RowText));
		RowPtr->Get()->SetColorAndOpacity(FSlateColor(Entry.bCompleted
			? FLinearColor(0.48f, 0.66f, 0.58f, 1.0f)
			: FLinearColor(0.88f, 0.89f, 0.85f, 1.0f)));
	}
}

void UTaskListWidget::ShowControlsPage()
{
	if (PageSwitcher)
	{
		PageSwitcher->SetActiveWidgetIndex(1);
	}
}

void UTaskListWidget::ShowRadioPage()
{
	if (PageSwitcher)
	{
		PageSwitcher->SetActiveWidgetIndex(0);
	}
}

void UTaskListWidget::OnControlsButtonClicked()
{
	ShowControlsPage();
}

void UTaskListWidget::OnRadioButtonClicked()
{
	ShowRadioPage();
}

void UTaskListWidget::OnCloseButtonClicked()
{
	OnCloseRequested.Broadcast();
}

TSharedRef<SWidget> UTaskListWidget::RebuildWidget()
{
	// Build the UMG tree before Slate creates the native widget representation.
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		UBorder* Dimmer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ScreenDimmer"));
		Dimmer->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.62f));
		Root->AddChild(Dimmer);
		if (UCanvasPanelSlot* DimmerSlot = Cast<UCanvasPanelSlot>(Dimmer->Slot))
		{
			DimmerSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			DimmerSlot->SetOffsets(FMargin(0.0f));
		}

		UBorder* Note = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("NotePanel"));
		Note->SetBrushColor(FLinearColor(0.025f, 0.035f, 0.037f, 1.0f));
		Note->SetPadding(FMargin(32.0f, 28.0f));
		Root->AddChild(Note);
		if (UCanvasPanelSlot* NoteSlot = Cast<UCanvasPanelSlot>(Note->Slot))
		{
			NoteSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			NoteSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			NoteSlot->SetPosition(FVector2D::ZeroVector);
			NoteSlot->SetSize(FVector2D(680.0f, 600.0f));
			NoteSlot->SetZOrder(1);
		}

		UVerticalBox* Layout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("NoteLayout"));
		Note->SetContent(Layout);

		UTextBlock* Heading = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Heading"));
		Heading->SetText(FText::FromString(TEXT("QUADRO DE ANOTA\u00C7\u00D5ES")));
		Heading->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 27));
		Heading->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.76f, 0.39f, 1.0f)));
		Layout->AddChildToVerticalBox(Heading);
		if (UVerticalBoxSlot* HeadingSlot = Cast<UVerticalBoxSlot>(Heading->Slot))
		{
			HeadingSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
		}

		USizeBox* PagesSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PagesSize"));
		PagesSize->SetHeightOverride(420.0f);
		Layout->AddChildToVerticalBox(PagesSize);
		PageSwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("Pages"));
		PagesSize->SetContent(PageSwitcher);

		// Página 1: as anotações sobre o rádio.
		UVerticalBox* RadioPage = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RadioPage"));
		PageSwitcher->AddChild(RadioPage);

		UTextBlock* DateHeading = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DateHeading"));
		DateHeading->SetText(FText::FromString(TEXT("Dia 1: Rádio quebrado")));
		DateHeading->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 22));
		DateHeading->SetColorAndOpacity(FSlateColor(FLinearColor(0.87f, 0.88f, 0.82f, 1.0f)));
		RadioPage->AddChildToVerticalBox(DateHeading);
		if (UVerticalBoxSlot* DateSlot = Cast<UVerticalBoxSlot>(DateHeading->Slot))
		{
			DateSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
		}
		USizeBox* RadioTextSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RadioTextSize"));
		RadioTextSize->SetHeightOverride(330.0f);
		RadioPage->AddChildToVerticalBox(RadioTextSize);
		UScrollBox* RadioTextScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RadioTextScroll"));
		RadioTextScroll->SetScrollBarVisibility(ESlateVisibility::Visible);
		RadioTextSize->SetContent(RadioTextScroll);
		UVerticalBox* RadioTextContent = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RadioTextContent"));
		RadioTextScroll->AddChild(RadioTextContent);

		UTextBlock* StoryNotes = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StoryNotes"));
		StoryNotes->SetText(FText::FromString(
			TEXT("Chave de fenda para abrir a carca\u00E7a. Mult\u00EDmetro dispens\u00E1vel; suspeita de bateria com curto-circuito.\n\n")
			TEXT("Descartar a bateria velha. Procurar uma reserva pela sala e deix\u00E1-la carregando.\n\n")
			TEXT("Depois, remontar e test\u00E1-lo.")));
		StoryNotes->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 18));
		StoryNotes->SetColorAndOpacity(FSlateColor(FLinearColor(0.77f, 0.81f, 0.77f, 1.0f)));
		StoryNotes->SetAutoWrapText(true);
		RadioTextContent->AddChildToVerticalBox(StoryNotes);
		if (UVerticalBoxSlot* StorySlot = Cast<UVerticalBoxSlot>(StoryNotes->Slot))
		{
			StorySlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
		}

		UTextBlock* ProgressHeading = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ProgressHeading"));
		ProgressHeading->SetText(FText::FromString(TEXT("Andamento")));
		ProgressHeading->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 16));
		ProgressHeading->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.76f, 0.39f, 1.0f)));
		RadioTextContent->AddChildToVerticalBox(ProgressHeading);
		if (UVerticalBoxSlot* ProgressSlot = Cast<UVerticalBoxSlot>(ProgressHeading->Slot))
		{
			ProgressSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 7.0f));
		}

		UVerticalBox* StepList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("StepList"));
		RadioTextContent->AddChildToVerticalBox(StepList);
		StepRows.Reset();
		for (const FChecklistEntry& Entry : ChecklistEntries)
		{
			UTextBlock* Row = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Row->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 18));
			StepList->AddChildToVerticalBox(Row);
			if (UVerticalBoxSlot* RowSlot = Cast<UVerticalBoxSlot>(Row->Slot))
			{
				RowSlot->SetPadding(FMargin(3.0f, 5.0f, 3.0f, 5.0f));
			}
			StepRows.Add(Entry.Key, Row);
		}
		RefreshStepRows();

		UHorizontalBox* RadioNavigation = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RadioNavigation"));
		RadioPage->AddChildToVerticalBox(RadioNavigation);
		if (UVerticalBoxSlot* NavSlot = Cast<UVerticalBoxSlot>(RadioNavigation->Slot))
		{
			NavSlot->SetPadding(FMargin(0.0f, 14.0f, 0.0f, 0.0f));
		}
		UButton* ControlsButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ControlsButton"));
		SetTextOnlyButtonStyle(ControlsButton);
		ControlsButton->OnClicked.AddDynamic(this, &UTaskListWidget::OnControlsButtonClicked);
		RadioNavigation->AddChildToHorizontalBox(ControlsButton);
		UTextBlock* ControlsButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ControlsButtonText"));
		ControlsButtonText->SetText(FText::FromString(TEXT("CONTROLES")));
		ControlsButtonText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 14));
		ControlsButtonText->SetColorAndOpacity(FSlateColor(FLinearColor(0.51f, 0.60f, 0.57f, 1.0f)));
		ControlsButton->SetContent(ControlsButtonText);
		UTextBlock* EHint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EHint"));
		EHint->SetText(FText::FromString(TEXT("   E")));
		EHint->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 15));
		EHint->SetColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.70f, 0.66f, 1.0f)));
		RadioNavigation->AddChildToHorizontalBox(EHint);

		// Página 2: controles do jogo.
		UVerticalBox* ControlsPage = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ControlsPage"));
		PageSwitcher->AddChild(ControlsPage);

		UTextBlock* ControlsHeading = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ControlsHeading"));
		ControlsHeading->SetText(FText::FromString(TEXT("CONTROLES")));
		ControlsHeading->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 22));
		ControlsHeading->SetColorAndOpacity(FSlateColor(FLinearColor(0.87f, 0.88f, 0.82f, 1.0f)));
		ControlsPage->AddChildToVerticalBox(ControlsHeading);

		UTextBlock* ControlsNotes = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ControlsNotes"));
		ControlsNotes->SetText(FText::FromString(
			TEXT("Q / E     Virar a c\u00E2mera para os lados\n\n")
			TEXT("CLIQUE     Interagir com objetos e avan\u00E7ar pelas \u00E1reas clic\u00E1veis\n\n")
			TEXT("TAB       Abrir o invent\u00E1rio\n\n")
			TEXT("P         Abrir o menu\n\n")
			TEXT("S         Cancelar ou voltar")));
		ControlsNotes->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 19));
		ControlsNotes->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f, 0.85f, 0.81f, 1.0f)));
		ControlsNotes->SetAutoWrapText(true);
		ControlsPage->AddChildToVerticalBox(ControlsNotes);
		if (UVerticalBoxSlot* ControlsSlot = Cast<UVerticalBoxSlot>(ControlsNotes->Slot))
		{
			ControlsSlot->SetPadding(FMargin(0.0f, 24.0f, 0.0f, 0.0f));
		}

		UHorizontalBox* ControlsNavigation = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ControlsNavigation"));
		ControlsPage->AddChildToVerticalBox(ControlsNavigation);
		if (UVerticalBoxSlot* ControlsNavSlot = Cast<UVerticalBoxSlot>(ControlsNavigation->Slot))
		{
			ControlsNavSlot->SetPadding(FMargin(0.0f, 30.0f, 0.0f, 0.0f));
		}
		UButton* RadioButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RadioButton"));
		SetTextOnlyButtonStyle(RadioButton);
		RadioButton->OnClicked.AddDynamic(this, &UTaskListWidget::OnRadioButtonClicked);
		ControlsNavigation->AddChildToHorizontalBox(RadioButton);
		UTextBlock* RadioButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RadioButtonText"));
		RadioButtonText->SetText(FText::FromString(TEXT("R\u00C1DIO")));
		RadioButtonText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 14));
		RadioButtonText->SetColorAndOpacity(FSlateColor(FLinearColor(0.51f, 0.60f, 0.57f, 1.0f)));
		RadioButton->SetContent(RadioButtonText);
		UTextBlock* QHint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QHint"));
		QHint->SetText(FText::FromString(TEXT("   Q")));
		QHint->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 15));
		QHint->SetColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.70f, 0.66f, 1.0f)));
		ControlsNavigation->AddChildToHorizontalBox(QHint);

		PageSwitcher->SetActiveWidgetIndex(0);

		UButton* CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
		SetTextOnlyButtonStyle(CloseButton);
		CloseButton->OnClicked.AddDynamic(this, &UTaskListWidget::OnCloseButtonClicked);
		Layout->AddChildToVerticalBox(CloseButton);
		UTextBlock* Footer = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Footer"));
		Footer->SetText(FText::FromString(TEXT("S  \u00B7  FECHAR O QUADRO")));
		Footer->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 14));
		Footer->SetColorAndOpacity(FSlateColor(FLinearColor(0.51f, 0.60f, 0.57f, 1.0f)));
		CloseButton->SetContent(Footer);
		if (UVerticalBoxSlot* FooterSlot = Cast<UVerticalBoxSlot>(CloseButton->Slot))
		{
			FooterSlot->SetPadding(FMargin(0.0f, 14.0f, 0.0f, 0.0f));
		}

		UE_LOG(LogTemp, Warning, TEXT("TaskListWidget: quadro montado com %d anotacoes de reparo."), ChecklistEntries.Num());
	}
	else if (!WidgetTree)
	{
		UE_LOG(LogTemp, Error, TEXT("TaskListWidget: WidgetTree is null during RebuildWidget."));
	}

	return Super::RebuildWidget();
}
