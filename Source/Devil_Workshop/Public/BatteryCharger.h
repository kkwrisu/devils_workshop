#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemUseTarget.h"
#include "BatteryCharger.generated.h"

class UStaticMeshComponent;
class UAudioComponent;
class USoundBase;
class UItemData;
class APlayerBase;

UENUM(BlueprintType)
enum class EChargerState : uint8
{
	Empty,
	Charging,
	Done
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnChargeStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnChargeCompleted);

// Carregador de bateria.
// - Com a bateria equipada, clicar no carregador insere a bateria e comeca a carregar (som em loop).
// - Ao terminar, toca um som de "pronto". Clicar no carregador de novo devolve a bateria
//   carregada ao inventario (com a flag "Carregada").
UCLASS(Blueprintable)
class ABatteryCharger : public AActor, public IItemUseTarget
{
	GENERATED_BODY()

public:
	ABatteryCharger();
	virtual void Tick(float DeltaTime) override;

	virtual bool CanUseItem_Implementation(const FInventoryItem& Item) override;
	virtual bool UseItem_Implementation(const FInventoryItem& Item, AActor* User) override;

	// Devolve a bateria carregada ao jogador. Retorna true se entregou.
	UFUNCTION(BlueprintCallable, Category = "Carregador")
	bool TryCollect(APlayerBase* Player);

	UFUNCTION(BlueprintPure, Category = "Carregador")
	EChargerState GetState() const { return State; }

	// 0 = vazio, 1 = carga completa
	UFUNCTION(BlueprintPure, Category = "Carregador")
	float GetChargeProgress() const;

	UPROPERTY(BlueprintAssignable, Category = "Carregador")
	FOnChargeStarted OnChargeStarted;

	UPROPERTY(BlueprintAssignable, Category = "Carregador")
	FOnChargeCompleted OnChargeCompleted;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componentes")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// Mostra a bateria "dentro" do carregador enquanto carrega (posicione no Blueprint)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componentes")
	TObjectPtr<UStaticMeshComponent> BatteryMesh;

	// Tag que o item precisa ter para ser aceito (no ItemData)
	UPROPERTY(EditAnywhere, Category = "Carregador")
	FName RequiredItemTag = "Bateria";

	// Flag colocada no item quando ele volta carregado
	UPROPERTY(EditAnywhere, Category = "Carregador")
	FName ChargedFlag = "Carregada";

	// Segundos para carregar
	UPROPERTY(EditAnywhere, Category = "Carregador", meta = (ClampMin = "1"))
	float ChargeDuration = 20.0f;

	UPROPERTY(EditAnywhere, Category = "Carregador|Audio")
	TObjectPtr<USoundBase> InsertSound;

	// Precisa estar com Looping marcado no asset de som
	UPROPERTY(EditAnywhere, Category = "Carregador|Audio")
	TObjectPtr<USoundBase> ChargingLoopSound;

	UPROPERTY(EditAnywhere, Category = "Carregador|Audio")
	TObjectPtr<USoundBase> ChargeCompleteSound;

private:
	void FinishCharging();

	EChargerState State = EChargerState::Empty;
	float ChargeElapsed = 0.0f;

	UPROPERTY()
	TObjectPtr<UItemData> StoredItemData;

	UPROPERTY()
	TObjectPtr<UAudioComponent> LoopAudio;
};
