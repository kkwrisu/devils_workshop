#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemPickup.generated.h"

class UStaticMeshComponent;
class UItemData;
class USoundBase;
class APlayerBase;

// Item solto no cenario (ex.: a bateria nova). Clicar nele poe o item no inventario do jogador
// e o objeto some do cenario.
UCLASS(Blueprintable)
class AItemPickup : public AActor
{
	GENERATED_BODY()

public:
	AItemPickup();

	// Retorna true se o item foi para o inventario
	UFUNCTION(BlueprintCallable, Category = "Item")
	bool TryPickup(APlayerBase* Player);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componentes")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// Qual item este objeto representa (ex.: DA_BateriaNova)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UItemData> ItemData;

	// Som opcional ao pegar
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<USoundBase> PickupSound;
};
