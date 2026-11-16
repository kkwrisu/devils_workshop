#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RepairStep.h"
#include "TaskListWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTaskListCloseRequested);

class UTextBlock;
class SWidget;
class UWidgetSwitcher;

UCLASS()
class DEVIL_WORKSHOP_API UTaskListWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeChecklist(const TArray<FRepairStep>& Steps);
	void MarkStepCompleted(FName StepId);
	void ShowControlsPage();
	void ShowRadioPage();

	UPROPERTY(BlueprintAssignable, Category = "Task List")
	FTaskListCloseRequested OnCloseRequested;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	UFUNCTION()
	void OnControlsButtonClicked();

	UFUNCTION()
	void OnRadioButtonClicked();

	UFUNCTION()
	void OnCloseButtonClicked();

	struct FChecklistEntry
	{
		FName Key;
		FText Label;
		TArray<FName> SourceStepIds;
		bool bCompleted = false;
	};

	void RefreshStepRows();

	TArray<FChecklistEntry> ChecklistEntries;
	TSet<FName> CompletedStepIds;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UTextBlock>> StepRows;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetSwitcher> PageSwitcher;
};
