#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/WrapBox.h"
#include "Components/CanvasPanel.h"
#include "InventoryTypes.h"
#include "InventoryWidget.generated.h"

class UInventoryComponent;
class UWrapBox;
class UCanvasPanel;
class UVerticalBox;
class UBorder;
class UImage;
class UTextBlock;
class UTextureRenderTarget2D;
class UMaterialInstanceDynamic;
class UMaterial;
class USceneCaptureComponent2D;
class AActor;
class UStaticMesh;

UCLASS(Blueprintable)
class UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Inventario")
	TObjectPtr<UInventoryComponent> Inventory;

	/** Material base para preview 3D - cria uma vez no Editor (Material > User Interface > TextureSampleParameter2D chamado "PreviewTexture") */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Inventario|Preview3D")
	TObjectPtr<UMaterial> PreviewBaseMaterial;

	UFUNCTION(BlueprintCallable, Category = "Inventario")
	void BindInventory(UInventoryComponent* InInventory);

	UFUNCTION(BlueprintCallable, Category = "Inventario")
	void Open();

	UFUNCTION(BlueprintCallable, Category = "Inventario")
	void Close();

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventario")
	void OnInventoryOpened();

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventario")
	void OnInventoryClosed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventario")
	void OnItemsRefreshed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventario")
	void OnSelectionUpdated(int32 SelectedIndex);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void HandleItemsChanged();

	UFUNCTION()
	void HandleSelectionChanged(int32 SelectedIndex);

private:
	void RebuildItemList();
	UVerticalBox* CreateItemSlot(const FInventoryItem& Item, int32 ItemIndex);
	void UpdateSelectionVisual(int32 NewSelectedIndex);
	void UpdateItemPreviews();

	// Preview 3D system
	void InitializePreviewSystem();
	void ShutdownPreviewSystem();
	UTextureRenderTarget2D* GetOrCreateRenderTarget(int32 SlotIndex);
	void UpdateSlotPreview(int32 SlotIndex, UStaticMesh* Mesh);
	void UpdatePreviewMaterial(int32 SlotIndex);
	void ApplyPreviewMaterialsToImages();
	void RotatePreviews(float DeltaTime);

	UPROPERTY()
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY()
	TObjectPtr<UWrapBox> ItemsWrapBox;

	// Preview actors/components (hidden in a preview scene)
	TArray<TObjectPtr<AActor>> PreviewActors;
	TArray<TObjectPtr<USceneCaptureComponent2D>> PreviewCaptures;
	TArray<TObjectPtr<UTextureRenderTarget2D>> PreviewRenderTargets;
	TArray<TObjectPtr<UMaterialInstanceDynamic>> PreviewMaterials;

	int32 LastSelectedIndex = INDEX_NONE;
	int32 MaxPreviewSlots = 20;
	float PreviewRotationSpeed = 30.0f;
};