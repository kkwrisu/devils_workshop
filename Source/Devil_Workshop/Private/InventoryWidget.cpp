#include "InventoryWidget.h"

#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "InventoryComponent.h"
#include "ItemData.h"
#include "InventoryTypes.h"

void UInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UE_LOG(LogTemp, Warning, TEXT("InventoryWidget: NativeConstruct | ItemsWrapBox = %d"), ItemsWrapBox ? 1 : 0);

	InitializePreviewSystem();

	if (Inventory)
	{
		RebuildItemList();
	}
}

void UInventoryWidget::NativeDestruct()
{
	ShutdownPreviewSystem();
	Super::NativeDestruct();
}

void UInventoryWidget::InitializePreviewSystem()
{
	if (!bUse3DPreview || !GetWorld()) return;

	PreviewActors.SetNum(MaxPreviewSlots);
	PreviewCaptures.SetNum(MaxPreviewSlots);
	PreviewRenderTargets.SetNum(MaxPreviewSlots);
	PreviewMaterials.SetNum(MaxPreviewSlots);

	for (int32 i = 0; i < MaxPreviewSlots; ++i)
	{
		UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(this, *FString::Printf(TEXT("PreviewRT_%d"), i));
		RT->InitCustomFormat(256, 256, EPixelFormat::PF_B8G8R8A8, false);
		RT->ClearColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
		RT->bAutoGenerateMips = false;
		PreviewRenderTargets[i] = RT;
		PreviewMaterials[i] = nullptr;
	}
}

void UInventoryWidget::ShutdownPreviewSystem()
{
	for (AActor* Actor : PreviewActors)
	{
		if (Actor && Actor->IsValidLowLevel())
		{
			Actor->Destroy();
		}
	}
	PreviewActors.Empty();
	PreviewCaptures.Empty();
	PreviewRenderTargets.Empty();
	PreviewMaterials.Empty();
}

UTextureRenderTarget2D* UInventoryWidget::GetOrCreateRenderTarget(int32 SlotIndex)
{
	if (SlotIndex >= 0 && SlotIndex < PreviewRenderTargets.Num() && PreviewRenderTargets[SlotIndex])
	{
		return PreviewRenderTargets[SlotIndex];
	}
	return nullptr;
}

void UInventoryWidget::UpdatePreviewMaterial(int32 SlotIndex)
{
	if (!PreviewBaseMaterial || SlotIndex < 0 || SlotIndex >= MaxPreviewSlots) return;

	UTextureRenderTarget2D* RT = GetOrCreateRenderTarget(SlotIndex);
	if (!RT) return;

	if (!PreviewMaterials[SlotIndex])
	{
		PreviewMaterials[SlotIndex] = UMaterialInstanceDynamic::Create(PreviewBaseMaterial, this);
		if (!PreviewMaterials[SlotIndex])
		{
			UE_LOG(LogTemp, Warning, TEXT("InventoryWidget: Falha ao criar MaterialInstanceDynamic para slot %d"), SlotIndex);
			return;
		}
	}

	PreviewMaterials[SlotIndex]->SetTextureParameterValue(FName("PreviewTexture"), PreviewRenderTargets[SlotIndex]);
}

void UInventoryWidget::UpdateSlotPreview(int32 SlotIndex, UStaticMesh* Mesh)
{
	if (!Mesh || !GetWorld() || SlotIndex < 0 || SlotIndex >= MaxPreviewSlots) return;

	UTextureRenderTarget2D* RT = GetOrCreateRenderTarget(SlotIndex);
	if (!RT) return;

	UpdatePreviewMaterial(SlotIndex);

	AActor* PreviewActor = PreviewActors[SlotIndex];
	USceneCaptureComponent2D* Capture = PreviewCaptures[SlotIndex];

	if (!PreviewActor || !PreviewActor->IsValidLowLevel())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Name = *FString::Printf(TEXT("InventoryPreviewActor_%d"), SlotIndex);
		SpawnParams.ObjectFlags |= RF_Transient;
		AActor* NewActor = GetWorld()->SpawnActor<AStaticMeshActor>(FVector(0, 0, -10000), FRotator::ZeroRotator, SpawnParams);
		PreviewActors[SlotIndex] = NewActor;
		PreviewActor = NewActor;

		if (PreviewActor)
		{
			PreviewActor->SetActorHiddenInGame(true);
			PreviewActor->SetActorEnableCollision(false);
			PreviewActor->SetActorTickEnabled(false);

			USceneCaptureComponent2D* NewCapture = NewObject<USceneCaptureComponent2D>(PreviewActor, *FString::Printf(TEXT("PreviewCapture_%d"), SlotIndex));
			PreviewCaptures[SlotIndex] = NewCapture;
			Capture = NewCapture;

			if (Capture)
			{
				Capture->RegisterComponent();
				Capture->AttachToComponent(PreviewActor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
				Capture->SetRelativeLocation(FVector(150.0f, 0.0f, 0.0f));
				Capture->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
				Capture->FOVAngle = 30.0f;
				Capture->ProjectionType = ECameraProjectionMode::Perspective;
				Capture->TextureTarget = GetOrCreateRenderTarget(SlotIndex);
				Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
				Capture->bCaptureEveryFrame = true;
				Capture->bCaptureOnMovement = false;
			}
		}
	}

	if (PreviewActor && Capture)
	{
		AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(PreviewActor);
		if (MeshActor && MeshActor->GetStaticMeshComponent())
		{
			UStaticMeshComponent* MeshComp = MeshActor->GetStaticMeshComponent();
			MeshComp->SetMobility(EComponentMobility::Movable);
			MeshComp->SetStaticMesh(Mesh);

			FBoxSphereBounds Bounds = Mesh->GetBounds();
			float MaxDim = FMath::Max3(Bounds.BoxExtent.X, Bounds.BoxExtent.Y, Bounds.BoxExtent.Z);
			if (MaxDim > 0.0f)
			{
				float TargetSize = 80.0f;
				float Scale = TargetSize / (MaxDim * 2.0f);
				MeshActor->SetActorScale3D(FVector(Scale));
				MeshActor->GetStaticMeshComponent()->SetRelativeLocation(-Bounds.Origin * Scale);
			}
		}

	}
}

void UInventoryWidget::UpdateItemPreviews()
{
	if (!bUse3DPreview || !Inventory) return;

	const TArray<FInventoryItem>& Items = Inventory->GetItems();
	for (int32 i = 0; i < Items.Num() && i < MaxPreviewSlots; ++i)
	{
		if (Items[i].Data && Items[i].Data->InspectMesh)
		{
			UpdateSlotPreview(i, Items[i].Data->InspectMesh);
		}
	}
}

void UInventoryWidget::RotatePreviews(float DeltaTime)
{
	float YawDelta = PreviewRotationSpeed * DeltaTime;
	for (AActor* Actor : PreviewActors)
	{
		if (Actor && Actor->IsValidLowLevel())
		{
			Actor->AddActorWorldRotation(FRotator(0.0f, YawDelta, 0.0f));
		}
	}
}

void UInventoryWidget::BindInventory(UInventoryComponent* InInventory)
{
	if (Inventory)
	{
		Inventory->OnItemsChanged.RemoveDynamic(this, &UInventoryWidget::HandleItemsChanged);
		Inventory->OnSelectionChanged.RemoveDynamic(this, &UInventoryWidget::HandleSelectionChanged);
		Inventory->OnEquippedChanged.RemoveDynamic(this, &UInventoryWidget::HandleEquippedChanged);
	}

	Inventory = InInventory;

	if (Inventory)
	{
		Inventory->OnItemsChanged.AddDynamic(this, &UInventoryWidget::HandleItemsChanged);
		Inventory->OnSelectionChanged.AddDynamic(this, &UInventoryWidget::HandleSelectionChanged);
		Inventory->OnEquippedChanged.AddDynamic(this, &UInventoryWidget::HandleEquippedChanged);

		RebuildItemList();
		OnEquippedUpdated(Inventory->GetEquippedInstanceId());
	}
}

void UInventoryWidget::Open()
{
	UE_LOG(LogTemp, Warning, TEXT("InventoryWidget: Open chamado"));

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	OnInventoryOpened();

	if (Inventory)
	{
		RebuildItemList();
		UpdateSelectionVisual(Inventory->GetSelectedIndex());
	}
}

void UInventoryWidget::Close()
{
	SetVisibility(bKeepVisibleWhenClosed ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	OnInventoryClosed();
}

void UInventoryWidget::HandleItemsChanged()
{
	RebuildItemList();
}

void UInventoryWidget::HandleSelectionChanged(int32 SelectedIndex)
{
	UpdateSelectionVisual(SelectedIndex);
	OnSelectionUpdated(SelectedIndex);
}

void UInventoryWidget::HandleEquippedChanged(int32 EquippedInstanceId)
{
	OnEquippedUpdated(EquippedInstanceId);
}

void UInventoryWidget::RebuildItemList()
{
	UE_LOG(LogTemp, Warning, TEXT("InventoryWidget: RebuildItemList | ItemsWrapBox = %d | Inventory = %d | itens = %d"), ItemsWrapBox ? 1 : 0, Inventory ? 1 : 0, Inventory ? Inventory->GetItems().Num() : -1);

	if (!ItemsWrapBox || !Inventory) return;

	ItemsWrapBox->ClearChildren();

	const TArray<FInventoryItem>& Items = Inventory->GetItems();
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		const FInventoryItem& Item = Items[i];
		if (Item.Data)
		{
			UVerticalBox* ItemSlotWidget = CreateItemSlot(Item, i);
			if (ItemSlotWidget)
			{
				UWrapBoxSlot* WrapSlot = ItemsWrapBox->AddChildToWrapBox(ItemSlotWidget);
				if (WrapSlot)
				{
					WrapSlot->SetHorizontalAlignment(HAlign_Center);
					WrapSlot->SetVerticalAlignment(VAlign_Center);
					WrapSlot->SetPadding(FMargin(16.0f));
				}
			}
		}
	}

	UpdateItemPreviews();
	ApplyPreviewMaterialsToImages();
	UpdateSelectionVisual(Inventory ? Inventory->GetSelectedIndex() : INDEX_NONE);

	UE_LOG(LogTemp, Warning, TEXT("InventoryWidget: lista montada | filhos = %d | visibilidade = %d | no viewport = %d"), ItemsWrapBox->GetChildrenCount(), static_cast<int32>(GetVisibility()), IsInViewport() ? 1 : 0);

	OnItemsRefreshed();
}

void UInventoryWidget::ApplyPreviewMaterialsToImages()
{
	if (!bUse3DPreview || !ItemsWrapBox) return;

	for (int32 i = 0; i < ItemsWrapBox->GetChildrenCount() && i < MaxPreviewSlots; ++i)
	{
		UWidget* SlotWidget = ItemsWrapBox->GetChildAt(i);
		if (UVerticalBox* SlotContainer = Cast<UVerticalBox>(SlotWidget))
		{
			if (UPanelWidget* Panel = Cast<UPanelWidget>(SlotContainer))
			{
				if (UBorder* Border = Cast<UBorder>(Panel->GetChildAt(0)))
				{
					if (UImage* PreviewImage = Cast<UImage>(Border->GetContent()))
					{
						if (i < PreviewMaterials.Num() && PreviewMaterials[i])
						{
							PreviewImage->SetBrushFromMaterial(PreviewMaterials[i]);
						}
					}
				}
			}
		}
	}
}

UVerticalBox* UInventoryWidget::CreateItemSlot(const FInventoryItem& Item, int32 ItemIndex)
{
	if (!Item.Data) return nullptr;

	UVerticalBox* SlotContainer = NewObject<UVerticalBox>(this, *FString::Printf(TEXT("ItemSlot_%d"), ItemIndex));
	if (!SlotContainer) return nullptr;

	UBorder* PreviewBorder = NewObject<UBorder>(SlotContainer, *FString::Printf(TEXT("PreviewBorder_%d"), ItemIndex));
	if (PreviewBorder)
	{
		FSlateBrush BorderBrush;
		BorderBrush.TintColor = FSlateColor(FLinearColor(0.08f, 0.08f, 0.1f, 1.0f));
		BorderBrush.ImageSize = FVector2D(100.0f, 100.0f);
		BorderBrush.DrawAs = ESlateBrushDrawType::Image;
		PreviewBorder->SetBrush(BorderBrush);

		UImage* PreviewImage = NewObject<UImage>(PreviewBorder, *FString::Printf(TEXT("PreviewImage_%d"), ItemIndex));
		if (PreviewImage)
		{
			PreviewImage->SetDesiredSizeOverride(FVector2D(96.0f, 96.0f));

			if (Item.Data->Icon)
			{
				PreviewImage->SetBrushFromTexture(Item.Data->Icon);
			}
			PreviewBorder->SetContent(PreviewImage);
		}

		SlotContainer->AddChild(PreviewBorder);
		if (UVerticalBoxSlot* BorderSlot = Cast<UVerticalBoxSlot>(PreviewBorder->Slot))
		{
			BorderSlot->SetHorizontalAlignment(HAlign_Center);
			BorderSlot->SetVerticalAlignment(VAlign_Center);
			BorderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}
	}

	UTextBlock* ItemNameText = NewObject<UTextBlock>(SlotContainer, *FString::Printf(TEXT("Name_%d"), ItemIndex));
	if (ItemNameText)
	{
		ItemNameText->SetText(Item.Data->DisplayName);
		ItemNameText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 12));
		ItemNameText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.9f, 0.9f, 1.0f)));
		ItemNameText->SetJustification(ETextJustify::Center);
		ItemNameText->SetShadowOffset(FVector2D(1.0f, 1.0f));
		ItemNameText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
		ItemNameText->SetWrapTextAt(140.0f);

		SlotContainer->AddChild(ItemNameText);
		if (UVerticalBoxSlot* TextSlot = Cast<UVerticalBoxSlot>(ItemNameText->Slot))
		{
			TextSlot->SetHorizontalAlignment(HAlign_Center);
		}
	}

	return SlotContainer;
}

void UInventoryWidget::UpdateSelectionVisual(int32 NewSelectedIndex)
{
	if (!ItemsWrapBox) return;

	if (LastSelectedIndex != INDEX_NONE && LastSelectedIndex >= 0 && LastSelectedIndex < ItemsWrapBox->GetChildrenCount())
	{
		UWidget* OldSlotWidget = ItemsWrapBox->GetChildAt(LastSelectedIndex);
		if (UVerticalBox* OldContainer = Cast<UVerticalBox>(OldSlotWidget))
		{
			OldContainer->SetRenderTransform(FWidgetTransform(FVector2D(1.0f, 1.0f), FVector2D(1.0f, 1.0f), FVector2D::ZeroVector, 0.0f));

			if (UPanelWidget* Panel = Cast<UPanelWidget>(OldContainer))
			{
				if (UBorder* Border = Cast<UBorder>(Panel->GetChildAt(0)))
				{
					FSlateBrush Brush;
					Brush.TintColor = FSlateColor(FLinearColor(0.08f, 0.08f, 0.1f, 1.0f));
					Brush.ImageSize = FVector2D(100.0f, 100.0f);
					Brush.DrawAs = ESlateBrushDrawType::Image;
					Border->SetBrush(Brush);
				}
			}
		}
	}

	if (NewSelectedIndex != INDEX_NONE && NewSelectedIndex >= 0 && NewSelectedIndex < ItemsWrapBox->GetChildrenCount())
	{
		UWidget* NewSlotWidget = ItemsWrapBox->GetChildAt(NewSelectedIndex);
		if (UVerticalBox* NewContainer = Cast<UVerticalBox>(NewSlotWidget))
		{
			NewContainer->SetRenderTransform(FWidgetTransform(FVector2D(1.06f, 1.06f), FVector2D(1.06f, 1.06f), FVector2D::ZeroVector, 0.0f));

			if (UPanelWidget* Panel = Cast<UPanelWidget>(NewContainer))
			{
				if (UBorder* Border = Cast<UBorder>(Panel->GetChildAt(0)))
				{
					FSlateBrush Brush;
					Brush.TintColor = FSlateColor(FLinearColor(1.0f, 0.8f, 0.1f, 1.0f));
					Brush.ImageSize = FVector2D(100.0f, 100.0f);
					Brush.DrawAs = ESlateBrushDrawType::Image;
					Border->SetBrush(Brush);
				}
			}
		}
	}

	LastSelectedIndex = NewSelectedIndex;
}