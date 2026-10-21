#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryTypes.h"
#include "InventoryWidget.generated.h"

class UInventoryComponent;
class UWrapBox;
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Inventario|Preview3D")
	TObjectPtr<UMaterial> PreviewBaseMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventario|Preview3D")
	bool bUse3DPreview = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventario")
	bool bKeepVisibleWhenClosed = false;

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

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventario")
	void OnEquippedUpdated(int32 EquippedInstanceId);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWrapBox> ItemsWrapBox;

	UFUNCTION()
	void HandleItemsChanged();

	UFUNCTION()
	void HandleSelectionChanged(int32 SelectedIndex);

	UFUNCTION()
	void HandleEquippedChanged(int32 EquippedInstanceId);

private:
	void RebuildItemList();
	UVerticalBox* CreateItemSlot(const FInventoryItem& Item, int32 ItemIndex);
	void UpdateSelectionVisual(int32 NewSelectedIndex);
	void UpdateItemPreviews();

	void InitializePreviewSystem();
	void ShutdownPreviewSystem();
	UTextureRenderTarget2D* GetOrCreateRenderTarget(int32 SlotIndex);
	void UpdateSlotPreview(int32 SlotIndex, UStaticMesh* Mesh);
	void UpdatePreviewMaterial(int32 SlotIndex);
	void ApplyPreviewMaterialsToImages();
	void RotatePreviews(float DeltaTime);

	UPROPERTY()
	TArray<TObjectPtr<AActor>> PreviewActors;

	UPROPERTY()
	TArray<TObjectPtr<USceneCaptureComponent2D>> PreviewCaptures;

	UPROPERTY()
	TArray<TObjectPtr<UTextureRenderTarget2D>> PreviewRenderTargets;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> PreviewMaterials;

	int32 LastSelectedIndex = INDEX_NONE;
	int32 MaxPreviewSlots = 20;
	float PreviewRotationSpeed = 30.0f;
};