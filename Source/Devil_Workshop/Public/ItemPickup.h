#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemPickup.generated.h"

class UStaticMeshComponent;
class UItemData;
class UInventoryComponent;

UCLASS(Blueprintable)
class AItemPickup : public AActor
{
	GENERATED_BODY()

public:
	AItemPickup();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UItemData> ItemData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	bool bDestroyOnCollect = true;

	UFUNCTION(BlueprintCallable, Category = "Item")
	bool Collect(UInventoryComponent* Inventory);

	UFUNCTION(BlueprintImplementableEvent, Category = "Item")
	void OnCollected();
};