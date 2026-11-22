#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PrologueCompletedWidget.generated.h"

class UBorder;
class UTextBlock;

UCLASS()
class DEVIL_WORKSHOP_API UPrologueCompletedWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void FillViewport();
	void SetFadeProgress(float BackgroundAlpha, float TextAlpha);
	void SetCompletionText(const FText& Text);

protected:
	virtual void NativeConstruct() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UBorder> BlackBackground;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CompletionText;
};
