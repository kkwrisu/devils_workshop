#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TaskListWidget.generated.h"

UCLASS()
class DEVIL_WORKSHOP_API UTaskListWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
};
