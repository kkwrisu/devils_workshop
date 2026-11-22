#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CorridorReticleWidget.generated.h"

class UTextBlock;

/** Minimal center reticle used while the player is in the corridor. */
UCLASS()
class DEVIL_WORKSHOP_API UCorridorReticleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetHasInteractableTarget(bool bHasTarget);

protected:
	virtual void NativeOnInitialized() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ReticleSymbol;
};
