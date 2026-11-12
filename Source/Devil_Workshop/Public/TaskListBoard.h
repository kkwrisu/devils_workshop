#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TaskListBoard.generated.h"

class UStaticMeshComponent;
class UBoxComponent;

/** Physical board/note the player can click to open the radio repair checklist. */
UCLASS(Blueprintable)
class DEVIL_WORKSHOP_API ATaskListBoard : public AActor
{
	GENERATED_BODY()

public:
	ATaskListBoard();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> InteractionVolume;
};
