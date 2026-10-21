#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemUseTarget.h"
#include "RepairPart.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class APlayerBase;
class ARepairPart;

UENUM(BlueprintType)
enum class ERepairPartState : uint8
{
	Fixed,
	Removed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRepairPartStateChanged, ARepairPart*, Part, ERepairPartState, NewState);

// Parte de um objeto reparavel (ex.: parafuso do radio). Fica anexada ao ator pai
// e so pode ser clicada enquanto o pai esta sendo inspecionado.
UCLASS(Blueprintable)
class ARepairPart : public AActor, public IItemUseTarget
{
	GENERATED_BODY()

public:
	ARepairPart();
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(BlueprintAssignable, Category = "Reparo")
	FOnRepairPartStateChanged OnStateChanged;

	UFUNCTION(BlueprintPure, Category = "Reparo")
	ERepairPartState GetState() const { return State; }

	UFUNCTION(BlueprintCallable, Category = "Reparo")
	void SetInteractable(bool bInteractable);

	virtual bool CanUseItem_Implementation(const FInventoryItem& Item) override;
	virtual bool UseItem_Implementation(const FInventoryItem& Item, AActor* User) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componentes")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componentes")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// Tag que o ItemData da ferramenta precisa ter (ex.: ChaveFenda)
	UPROPERTY(EditAnywhere, Category = "Reparo")
	FName RequiredToolTag = "ChaveFenda";

	// Partes que precisam ter sido removidas antes desta
	UPROPERTY(EditInstanceOnly, Category = "Reparo")
	TArray<TObjectPtr<ARepairPart>> RequiredRemoved;

	// Segundos segurando o clique para concluir
	UPROPERTY(EditAnywhere, Category = "Reparo", meta = (ClampMin = "0.1"))
	float WorkDuration = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Reparo")
	float TurnDegrees = 720.0f;

	// Quanto a peca sobe ao desparafusar (unidades locais)
	UPROPERTY(EditAnywhere, Category = "Reparo")
	float LiftDistance = 0.8f;

	// Eixo do parafuso no espaco local do mesh
	UPROPERTY(EditAnywhere, Category = "Reparo")
	FVector TurnAxis = FVector::UpVector;

private:
	void StopWork(bool bCompleted);
	void ApplyProgressVisual();

	ERepairPartState State = ERepairPartState::Fixed;
	TWeakObjectPtr<APlayerBase> Worker;
	float Progress = 0.0f;
	FVector InitialLocation = FVector::ZeroVector;
	FQuat InitialRotation = FQuat::Identity;
};