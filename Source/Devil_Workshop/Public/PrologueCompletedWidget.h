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

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UBorder> BlackBackground;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CompletionText;

	float ElapsedTime = 0.0f;
	bool bPausedAtEnd = false;
	float FadeDuration = 2.0f;
	float TextFadeDelay = 0.8f;
	float TextFadeDuration = 0.6f;
};
