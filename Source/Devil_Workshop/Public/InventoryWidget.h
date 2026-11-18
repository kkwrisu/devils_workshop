#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "InventoryTypes.h"
#include "InventoryWidget.generated.h"

class UInventoryComponent;
class UWrapBox;
class UVerticalBox;
class UBorder;
class UImage;
class UTextBlock;
class UTextureRenderTarget2D;
class UTexture;
class UMaterial;
class USceneCaptureComponent2D;
class UBackgroundBlur;
class SWidget;
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
	bool bUse3DPreview = true;

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
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

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
	void ApplyPreviewMaterialsToImages();
	void RotatePreviews(float DeltaTime);
	void BuildInventoryScreen();
	void UpdateSelectedItemPreview(int32 SelectedIndex);

	UPROPERTY()
	TArray<TObjectPtr<AActor>> PreviewActors;

	UPROPERTY()
	TArray<TObjectPtr<USceneCaptureComponent2D>> PreviewCaptures;

	UPROPERTY()
	TArray<TObjectPtr<UTextureRenderTarget2D>> PreviewRenderTargets;
	TArray<FVector> PreviewCaptureBaseLocations;
	float PreviewAnimationTime = 0.0f;
	float PreviewFloatAmplitude = 4.0f;
	UPROPERTY(Transient)
	TObjectPtr<UBackgroundBlur> InventoryBackgroundBlur;
	UPROPERTY(Transient)
	TObjectPtr<UImage> SelectedItemPreview;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SelectedItemName;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SelectedItemDescription;
	UPROPERTY(Transient)
	TObjectPtr<UTexture> SelectedPreviewTexture;
	UPROPERTY(Transient)
	TMap<TObjectPtr<UTexture>, FSlateBrush> CircularPreviewBrushCache;
	bool bInventoryScreenBuilt = false;

	TArray<int32> VisibleItemIndices;

	int32 LastSelectedIndex = INDEX_NONE;
	float AnimatedSelectionPosition = 0.0f;
	float TargetSelectionPosition = 0.0f;
	bool bSelectionAnimationInitialized = false;
	float SelectionAnimationSpeed = 8.0f;
	int32 MaxPreviewSlots = 20;
	float PreviewRotationSpeed = 30.0f;
};
