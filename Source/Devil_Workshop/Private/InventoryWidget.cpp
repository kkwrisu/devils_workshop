#include "InventoryWidget.h"

#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Components/BackgroundBlur.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Rendering/DrawElementTypes.h"
#include "Rendering/RenderingCommon.h"
#include "Textures/SlateShaderResource.h"
#include "Styling/SlateBrush.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Components/MeshComponent.h"
#include "Engine/Texture2D.h"
#include "Engine/Texture.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Styling/CoreStyle.h"
#include "GameFramework/Actor.h"
#include "InventoryComponent.h"
#include "ItemData.h"
#include "InventoryTypes.h"

TSharedRef<SWidget> UInventoryWidget::RebuildWidget()
{
	BuildInventoryScreen();
	return Super::RebuildWidget();
}

void UInventoryWidget::BuildInventoryScreen()
{
	if (bInventoryScreenBuilt || !WidgetTree) return;

	if (!ItemsWrapBox)
	{
		ItemsWrapBox = Cast<UWrapBox>(WidgetTree->FindWidget(TEXT("ItemsWrapBox")));
	}
	if (ItemsWrapBox)
	{
		ItemsWrapBox->RemoveFromParent();
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("InventoryScreen"));
	WidgetTree->RootWidget = Root;

	InventoryBackgroundBlur = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass(), TEXT("InventoryBackgroundBlur"));
	InventoryBackgroundBlur->SetBlurStrength(12.0f);
	InventoryBackgroundBlur->SetApplyAlphaToBlur(true);
	Root->AddChild(InventoryBackgroundBlur);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(InventoryBackgroundBlur->Slot))
	{
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		CanvasSlot->SetOffsets(FMargin(0.0f));
	}

	UBorder* Dimmer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InventoryDimmer"));
	Dimmer->SetBrushColor(FLinearColor(0.015f, 0.02f, 0.025f, 0.48f));
	Root->AddChild(Dimmer);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Dimmer->Slot))
	{
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		CanvasSlot->SetOffsets(FMargin(0.0f));
		CanvasSlot->SetZOrder(1);
	}

	SelectedItemPreview = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SelectedItemPreview"));
	SelectedItemPreview->SetDesiredSizeOverride(FVector2D(360.0f, 360.0f));
	// This image only supplies layout geometry for NativePaint; the circular texture is drawn in Slate.
	SelectedItemPreview->SetColorAndOpacity(FLinearColor::Transparent);
	Root->AddChild(SelectedItemPreview);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(SelectedItemPreview->Slot))
	{
		CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetPosition(FVector2D(0.0f, 10.0f));
		CanvasSlot->SetSize(FVector2D(360.0f, 360.0f));
		CanvasSlot->SetZOrder(2);
	}

	SelectedItemName = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SelectedItemName"));
	SelectedItemName->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 26));
	SelectedItemName->SetColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.86f, 0.68f, 1.0f)));
	SelectedItemName->SetJustification(ETextJustify::Center);
	Root->AddChild(SelectedItemName);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(SelectedItemName->Slot))
	{
		CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		CanvasSlot->SetPosition(FVector2D(0.0f, -290.0f));
		CanvasSlot->SetSize(FVector2D(600.0f, 42.0f));
		CanvasSlot->SetZOrder(3);
	}

	SelectedItemDescription = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SelectedItemDescription"));
	SelectedItemDescription->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 16));
	SelectedItemDescription->SetColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.77f, 0.76f, 1.0f)));
	SelectedItemDescription->SetJustification(ETextJustify::Center);
	SelectedItemDescription->SetAutoWrapText(true);
	Root->AddChild(SelectedItemDescription);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(SelectedItemDescription->Slot))
	{
		CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		CanvasSlot->SetPosition(FVector2D(0.0f, -252.0f));
		CanvasSlot->SetSize(FVector2D(600.0f, 56.0f));
		CanvasSlot->SetZOrder(3);
	}

	if (!ItemsWrapBox)
	{
		ItemsWrapBox = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("ItemsWrapBox"));
	}
	ItemsWrapBox->SetOrientation(Orient_Horizontal);
	ItemsWrapBox->SetHorizontalAlignment(HAlign_Center);
	ItemsWrapBox->SetExplicitWrapSize(true);
	ItemsWrapBox->SetWrapSize(1300.0f);
	ItemsWrapBox->SetInnerSlotPadding(FVector2D(8.0f, 0.0f));
	UTextBlock* ControlsHint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InventoryControlsHint"));
	ControlsHint->SetText(FText::FromString(TEXT("RODA DO MOUSE  ·  TROCAR ITEM          TAB  ·  FECHAR")));
	ControlsHint->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 13));
	ControlsHint->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.72f, 0.69f, 0.85f)));
	ControlsHint->SetJustification(ETextJustify::Center);
	Root->AddChild(ControlsHint);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ControlsHint->Slot))
	{
		CanvasSlot->SetAnchors(FAnchors(0.5f, 1.0f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 1.0f));
		CanvasSlot->SetPosition(FVector2D(0.0f, -182.0f));
		CanvasSlot->SetSize(FVector2D(700.0f, 24.0f));
		CanvasSlot->SetZOrder(5);
	}

	bInventoryScreenBuilt = true;
}

void UInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	bUse3DPreview = true;

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

void UInventoryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (GetVisibility() != ESlateVisibility::Collapsed)
	{
		RotatePreviews(InDeltaTime);
		if (bSelectionAnimationInitialized && !FMath::IsNearlyEqual(AnimatedSelectionPosition, TargetSelectionPosition, 0.001f))
		{
			AnimatedSelectionPosition = FMath::FInterpTo(AnimatedSelectionPosition, TargetSelectionPosition, InDeltaTime, SelectionAnimationSpeed);
			if (FMath::IsNearlyEqual(AnimatedSelectionPosition, TargetSelectionPosition, 0.01f))
			{
				AnimatedSelectionPosition = TargetSelectionPosition;
			}
			InvalidateLayoutAndVolatility();
		}
	}
}

int32 UInventoryWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 MaxLayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	if (!SelectedItemPreview || !SelectedItemPreview->IsVisible())
	{
		return MaxLayerId;
	}

	const FGeometry& PreviewGeometry = SelectedItemPreview->GetCachedGeometry();
	const FVector2D PreviewSize = PreviewGeometry.GetLocalSize();
	if (PreviewSize.X <= 0.0 || PreviewSize.Y <= 0.0)
	{
		return MaxLayerId;
	}

	const FVector2D PreviewCenter = PreviewGeometry.LocalToAbsolute(PreviewSize * 0.5);
	const FSlateRenderTransform& WidgetRenderTransform = AllottedGeometry.GetAccumulatedRenderTransform();
	int32 DrawLayer = MaxLayerId;

	auto DrawCircularTexture = [&](UTexture* Texture, const FVector2D& AbsoluteCenter, const FVector2D& Size)
	{
		if (!Texture) return;

		constexpr int32 SegmentCount = 64;
		TArray<FSlateVertex> Vertices;
		TArray<SlateIndex> Indices;
		Vertices.Reserve(SegmentCount + 1);
		Indices.Reserve(SegmentCount * 3);

		const FVector2D Center = AllottedGeometry.AbsoluteToLocal(AbsoluteCenter);
		const float RadiusX = static_cast<float>(Size.X * 0.5);
		const float RadiusY = static_cast<float>(Size.Y * 0.5);
		const FColor VertexColor = FColor::White;
		Vertices.Add(FSlateVertex::Make(WidgetRenderTransform, FVector2f(Center), FVector2f(0.5f, 0.5f), VertexColor));

		for (int32 Segment = 0; Segment < SegmentCount; ++Segment)
		{
			const float Angle = 2.0f * PI * static_cast<float>(Segment) / static_cast<float>(SegmentCount);
			const float U = 0.5f + 0.5f * FMath::Cos(Angle);
			const float V = 0.5f + 0.5f * FMath::Sin(Angle);
			const FVector2D Point = AbsoluteCenter + FVector2D(FMath::Cos(Angle) * RadiusX, FMath::Sin(Angle) * RadiusY);
			Vertices.Add(FSlateVertex::Make(WidgetRenderTransform, FVector2f(AllottedGeometry.AbsoluteToLocal(Point)), FVector2f(U, V), VertexColor));
		}

		for (int32 Segment = 0; Segment < SegmentCount; ++Segment)
		{
			Indices.Add(0);
			Indices.Add(static_cast<SlateIndex>(Segment + 1));
			Indices.Add(static_cast<SlateIndex>(((Segment + 1) % SegmentCount) + 1));
		}

		FSlateBrush& TextureBrush = const_cast<UInventoryWidget*>(this)->CircularPreviewBrushCache.FindOrAdd(Texture);
		if (!TextureBrush.GetResourceObject())
		{
			TextureBrush.SetResourceObject(Texture);
			TextureBrush.DrawAs = ESlateBrushDrawType::Image;
		}
		const FSlateResourceHandle& ResourceHandle = TextureBrush.GetRenderingResource();
		if (!ResourceHandle.IsValid()) return;
		const FSlateShaderResourceProxy* ResourceProxy = ResourceHandle.GetResourceProxy();
		if (!ResourceProxy || !ResourceProxy->Resource || !ResourceProxy->Resource->IsResourceValid()) return;
		FSlateDrawElement::MakeCustomVerts(OutDrawElements, ++DrawLayer, ResourceHandle, Vertices, Indices, nullptr, 0, 0, ESlateDrawEffect::InvertAlpha);
	};

	if (Inventory && Inventory->GetItems().IsValidIndex(Inventory->GetSelectedIndex()))
	{
		const TArray<FInventoryItem>& Items = Inventory->GetItems();
		const float FocusPosition = AnimatedSelectionPosition;
		auto GetItemPreviewTexture = [&](int32 ItemIndex) -> UTexture*
		{
			if (!Items.IsValidIndex(ItemIndex) || !Items[ItemIndex].Data) return nullptr;
			if ((Items[ItemIndex].Data->InspectMesh || Items[ItemIndex].Data->InspectSkeletalMesh)
				&& PreviewRenderTargets.IsValidIndex(ItemIndex) && PreviewRenderTargets[ItemIndex])
			{
				return PreviewRenderTargets[ItemIndex];
			}
			return Items[ItemIndex].Data->Icon;
		};

		TArray<int32> VisiblePreviewIndices;
		const float ItemCount = static_cast<float>(Items.Num());
		auto GetWrappedDelta = [&](int32 ItemIndex)
		{
			if (ItemCount == 2.0f)
			{
				// Keep the next item on the right of the first; the previous item sits left of the second.
				return ItemIndex == 0 ? -FocusPosition : 1.0f - FocusPosition;
			}

			float Delta = static_cast<float>(ItemIndex) - FocusPosition;
			if (ItemCount > 1.0f)
			{
				Delta = FMath::Fmod(Delta, ItemCount);
				if (Delta > ItemCount * 0.5f) Delta -= ItemCount;
				if (Delta < -ItemCount * 0.5f) Delta += ItemCount;
				// For an exact half-turn, place the wrapped previous item on the left.
				if (FMath::IsNearlyEqual(Delta, ItemCount * 0.5f)) Delta -= ItemCount;
			}
			return Delta;
		};
		for (int32 ItemIndex = 0; ItemIndex < Items.Num(); ++ItemIndex)
		{
			if (FMath::Abs(GetWrappedDelta(ItemIndex)) <= 1.25f && GetItemPreviewTexture(ItemIndex))
			{
				VisiblePreviewIndices.Add(ItemIndex);
			}
		}
		VisiblePreviewIndices.Sort([&](int32 A, int32 B)
		{
			return FMath::Abs(GetWrappedDelta(A)) > FMath::Abs(GetWrappedDelta(B));
		});

		for (const int32 ItemIndex : VisiblePreviewIndices)
		{
			const float Delta = GetWrappedDelta(ItemIndex);
			const float Distance = FMath::Abs(Delta);
			const float Depth = FMath::Min(Distance, 1.0f);
			const float Size = FMath::Lerp(360.0f, 150.0f, Depth);
			const float Side = FMath::Sign(Delta);
			const FVector2D ItemCenter = PreviewCenter + FVector2D(Side * 285.0f * Depth, 35.0f * Depth);
			DrawCircularTexture(GetItemPreviewTexture(ItemIndex), ItemCenter, FVector2D(Size, Size));
		}
	}
	else if (SelectedPreviewTexture)
	{
		DrawCircularTexture(SelectedPreviewTexture, PreviewCenter, PreviewSize);
	}
	return DrawLayer;
}

void UInventoryWidget::InitializePreviewSystem()
{
	if (!bUse3DPreview || !GetWorld()) return;

	PreviewActors.SetNum(MaxPreviewSlots);
	PreviewSkeletalMeshComponents.SetNum(MaxPreviewSlots);
	PreviewCaptures.SetNum(MaxPreviewSlots);
	PreviewKeyLights.SetNum(MaxPreviewSlots);
	PreviewFillLights.SetNum(MaxPreviewSlots);
	PreviewRenderTargets.SetNum(MaxPreviewSlots);
	PreviewCaptureBaseLocations.SetNum(MaxPreviewSlots);

	for (int32 i = 0; i < MaxPreviewSlots; ++i)
	{
		UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(this, *FString::Printf(TEXT("PreviewRT_%d"), i));
		RT->InitCustomFormat(256, 256, EPixelFormat::PF_B8G8R8A8, false);
		RT->ClearColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
		RT->bAutoGenerateMips = false;
		PreviewRenderTargets[i] = RT;
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
	PreviewSkeletalMeshComponents.Empty();
	PreviewCaptures.Empty();
	PreviewKeyLights.Empty();
	PreviewFillLights.Empty();
	PreviewRenderTargets.Empty();
}

UTextureRenderTarget2D* UInventoryWidget::GetOrCreateRenderTarget(int32 SlotIndex)
{
	if (SlotIndex >= 0 && SlotIndex < PreviewRenderTargets.Num() && PreviewRenderTargets[SlotIndex])
	{
		return PreviewRenderTargets[SlotIndex];
	}
	return nullptr;
}

void UInventoryWidget::UpdateSlotPreview(int32 SlotIndex, const UItemData* ItemData)
{
	if (!ItemData || (!ItemData->InspectMesh && !ItemData->InspectSkeletalMesh)
		|| !GetWorld() || SlotIndex < 0 || SlotIndex >= MaxPreviewSlots) return;

	UTextureRenderTarget2D* RT = GetOrCreateRenderTarget(SlotIndex);
	if (!RT) return;

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
			// O ator fica fora da área jogável, mas visível para a SceneCapture.
			PreviewActor->SetActorHiddenInGame(false);
			PreviewActor->SetActorEnableCollision(false);
			PreviewActor->SetActorTickEnabled(false);

			auto CreatePreviewLight = [PreviewActor](const FName& ComponentName, float Intensity, float Radius, float SourceRadius)
			{
				UPointLightComponent* Light = NewObject<UPointLightComponent>(PreviewActor, ComponentName);
				if (!Light) return static_cast<UPointLightComponent*>(nullptr);

				PreviewActor->AddInstanceComponent(Light);
				Light->SetupAttachment(PreviewActor->GetRootComponent());
				Light->SetMobility(EComponentMobility::Movable);
				Light->SetAbsolute(false, false, true);
				Light->SetIntensity(Intensity);
				Light->SetAttenuationRadius(Radius);
				Light->SetSourceRadius(SourceRadius);
				Light->SetLightColor(FLinearColor::White);
				Light->SetCastShadows(false);
				Light->SetLightingChannels(false, true, false);
				Light->RegisterComponent();
				return Light;
			};
			PreviewKeyLights[SlotIndex] = CreatePreviewLight(FName(*FString::Printf(TEXT("PreviewKeyLight_%d"), SlotIndex)), 2400.0f, 280.0f, 70.0f);
			PreviewFillLights[SlotIndex] = CreatePreviewLight(FName(*FString::Printf(TEXT("PreviewFillLight_%d"), SlotIndex)), 900.0f, 240.0f, 90.0f);

			USceneCaptureComponent2D* NewCapture = NewObject<USceneCaptureComponent2D>(PreviewActor, *FString::Printf(TEXT("PreviewCapture_%d"), SlotIndex));
			PreviewCaptures[SlotIndex] = NewCapture;
			Capture = NewCapture;

			if (Capture)
			{
				Capture->AttachToComponent(PreviewActor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
				const FVector CameraLocation(150.0f, -150.0f, 100.0f);
				Capture->SetRelativeLocation(CameraLocation);
				Capture->SetRelativeRotation((-CameraLocation).Rotation());
				Capture->ProjectionType = ECameraProjectionMode::Orthographic;
				Capture->OrthoWidth = 120.0f;
				Capture->TextureTarget = GetOrCreateRenderTarget(SlotIndex);
				Capture->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
				Capture->CompositeMode = SCCM_Overwrite;
				Capture->ShowFlags.SetAtmosphere(false);
				Capture->ShowFlags.SetCloud(false);
				Capture->ShowFlags.SetFog(false);
				Capture->ShowFlags.SetSkyLighting(false);
				Capture->ShowFlags.SetVolumetricFog(false);
				Capture->bCaptureEveryFrame = true;
				Capture->bCaptureOnMovement = false;
				Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
				Capture->ShowOnlyActorComponents(PreviewActor);
				Capture->RegisterComponent();
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
			MeshActor->SetActorLocation(FVector(0.0f, 0.0f, -10000.0f));

			if (!PreviewSkeletalMeshComponents[SlotIndex])
			{
				USkeletalMeshComponent* NewSkeletalMeshComp = NewObject<USkeletalMeshComponent>(PreviewActor, TEXT("PreviewSkeletalMesh"));
				NewSkeletalMeshComp->SetupAttachment(PreviewActor->GetRootComponent());
				NewSkeletalMeshComp->SetMobility(EComponentMobility::Movable);
				NewSkeletalMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				NewSkeletalMeshComp->SetComponentTickEnabled(false);
				PreviewActor->AddInstanceComponent(NewSkeletalMeshComp);
				NewSkeletalMeshComp->RegisterComponent();
				PreviewSkeletalMeshComponents[SlotIndex] = NewSkeletalMeshComp;
			}

			USkeletalMeshComponent* SkeletalMeshComp = PreviewSkeletalMeshComponents[SlotIndex];
			UPrimitiveComponent* ActiveMeshComponent = nullptr;
			FBoxSphereBounds Bounds(ForceInit);
			if (ItemData->InspectSkeletalMesh && SkeletalMeshComp)
			{
				MeshComp->SetVisibility(false, false);
				MeshComp->SetHiddenInGame(true, false);
				MeshComp->SetStaticMesh(nullptr);
				SkeletalMeshComp->SetSkeletalMesh(ItemData->InspectSkeletalMesh);
				SkeletalMeshComp->EmptyOverrideMaterials();
				for (int32 MaterialIndex = 0; MaterialIndex < ItemData->InspectMaterialOverrides.Num(); ++MaterialIndex)
				{
					if (ItemData->InspectMaterialOverrides[MaterialIndex])
					{
						SkeletalMeshComp->SetMaterial(MaterialIndex, ItemData->InspectMaterialOverrides[MaterialIndex]);
					}
				}
				SkeletalMeshComp->SetLightingChannels(false, true, false);
				SkeletalMeshComp->SetVisibility(true, true);
				SkeletalMeshComp->SetHiddenInGame(false, true);
				ActiveMeshComponent = SkeletalMeshComp;
				Bounds = ItemData->InspectSkeletalMesh->GetBounds();
			}
			else if (ItemData->InspectMesh)
			{
				if (SkeletalMeshComp)
				{
					SkeletalMeshComp->SetVisibility(false, true);
					SkeletalMeshComp->SetHiddenInGame(true, true);
					SkeletalMeshComp->SetSkeletalMesh(nullptr);
				}
				MeshComp->SetStaticMesh(ItemData->InspectMesh);
				MeshComp->EmptyOverrideMaterials();
				for (int32 MaterialIndex = 0; MaterialIndex < ItemData->InspectMaterialOverrides.Num(); ++MaterialIndex)
				{
					if (ItemData->InspectMaterialOverrides[MaterialIndex])
					{
						MeshComp->SetMaterial(MaterialIndex, ItemData->InspectMaterialOverrides[MaterialIndex]);
					}
				}
				MeshComp->SetLightingChannels(false, true, false);
				MeshComp->SetVisibility(true, false);
				MeshComp->SetHiddenInGame(false, false);
				ActiveMeshComponent = MeshComp;
				Bounds = ItemData->InspectMesh->GetBounds();
			}

			if (!ActiveMeshComponent) return;
			Capture->ClearShowOnlyComponents();
			Capture->ShowOnlyComponent(ActiveMeshComponent);

			float MaxDim = FMath::Max3(Bounds.BoxExtent.X, Bounds.BoxExtent.Y, Bounds.BoxExtent.Z);
			if (MaxDim > 0.0f)
			{
				float TargetSize = 80.0f;
				float Scale = TargetSize / (MaxDim * 2.0f);
				MeshActor->SetActorScale3D(FVector(Scale));

				if (PreviewKeyLights.IsValidIndex(SlotIndex) && PreviewKeyLights[SlotIndex])
				{
					PreviewKeyLights[SlotIndex]->SetRelativeLocation(FVector(80.0f, -90.0f, 80.0f) / Scale);
				}
				if (PreviewFillLights.IsValidIndex(SlotIndex) && PreviewFillLights[SlotIndex])
				{
					PreviewFillLights[SlotIndex]->SetRelativeLocation(FVector(40.0f, -80.0f, -65.0f) / Scale);
				}

				// SceneCapture is attached to this scaled component, so offset its relative position
				// to aim at the mesh bounds center while keeping a fixed world-space camera distance.
				const FVector CameraOffset(150.0f, -150.0f, 100.0f);
				Capture->SetRelativeLocation(Bounds.Origin + CameraOffset / Scale);
				Capture->SetRelativeRotation((-CameraOffset).Rotation());
				PreviewCaptureBaseLocations[SlotIndex] = Capture->GetRelativeLocation();
			}
			Capture->CaptureScene();
		}

	}
}

void UInventoryWidget::UpdateItemPreviews()
{
	if (!bUse3DPreview || !Inventory) return;

	const TArray<FInventoryItem>& Items = Inventory->GetItems();
	for (int32 i = 0; i < Items.Num() && i < MaxPreviewSlots; ++i)
	{
		if (Items[i].Data && (Items[i].Data->InspectMesh || Items[i].Data->InspectSkeletalMesh))
		{
			UpdateSlotPreview(i, Items[i].Data);
		}
	}
}

void UInventoryWidget::RotatePreviews(float DeltaTime)
{
	PreviewAnimationTime += DeltaTime;
	float YawDelta = PreviewRotationSpeed * DeltaTime;
	for (int32 Index = 0; Index < PreviewActors.Num(); ++Index)
	{
		AActor* Actor = PreviewActors[Index];
		if (Actor && Actor->IsValidLowLevel())
		{
			Actor->AddActorWorldRotation(FRotator(0.0f, YawDelta, 0.0f));

			if (PreviewCaptures.IsValidIndex(Index) && PreviewCaptures[Index] && PreviewCaptureBaseLocations.IsValidIndex(Index))
			{
				const float ActorScale = FMath::Max(Actor->GetActorScale3D().Z, KINDA_SMALL_NUMBER);
				const float BobOffset = FMath::Sin(PreviewAnimationTime * 1.8f + Index * 0.7f) * PreviewFloatAmplitude;
				PreviewCaptures[Index]->SetRelativeLocation(PreviewCaptureBaseLocations[Index] + FVector(0.0f, 0.0f, BobOffset / ActorScale));
			}
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
		const int32 SelectedIndex = Inventory->GetSelectedIndex();
		AnimatedSelectionPosition = static_cast<float>(SelectedIndex);
		TargetSelectionPosition = AnimatedSelectionPosition;
		bSelectionAnimationInitialized = true;
		UpdateSelectionVisual(SelectedIndex);
		UpdateSelectedItemPreview(SelectedIndex);
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
	if (Inventory && Inventory->GetItems().IsValidIndex(SelectedIndex))
	{
		const float ItemCount = static_cast<float>(Inventory->GetItems().Num());
		if (!bSelectionAnimationInitialized)
		{
			AnimatedSelectionPosition = static_cast<float>(SelectedIndex);
			TargetSelectionPosition = AnimatedSelectionPosition;
			bSelectionAnimationInitialized = true;
		}
		else if (ItemCount > 1.0f)
		{
			float CurrentTargetIndex = FMath::Fmod(TargetSelectionPosition, ItemCount);
			if (CurrentTargetIndex < 0.0f) CurrentTargetIndex += ItemCount;
			float Delta = static_cast<float>(SelectedIndex) - CurrentTargetIndex;
			if (Delta > ItemCount * 0.5f) Delta -= ItemCount;
			if (Delta < -ItemCount * 0.5f) Delta += ItemCount;
			TargetSelectionPosition += Delta;
		}
		else
		{
			TargetSelectionPosition = static_cast<float>(SelectedIndex);
		}
	}
	else
	{
		TargetSelectionPosition = static_cast<float>(SelectedIndex);
	}
	UpdateSelectionVisual(SelectedIndex);
	UpdateSelectedItemPreview(SelectedIndex);
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
	VisibleItemIndices.Reset();

	const TArray<FInventoryItem>& Items = Inventory->GetItems();
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		const FInventoryItem& Item = Items[i];
		if (Item.Data)
		{
			VisibleItemIndices.Add(i);
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
	UpdateSelectedItemPreview(Inventory ? Inventory->GetSelectedIndex() : INDEX_NONE);

	UE_LOG(LogTemp, Warning, TEXT("InventoryWidget: lista montada | filhos = %d | visibilidade = %d | no viewport = %d"), ItemsWrapBox->GetChildrenCount(), static_cast<int32>(GetVisibility()), IsInViewport() ? 1 : 0);

	OnItemsRefreshed();
}

void UInventoryWidget::ApplyPreviewMaterialsToImages()
{
	if (!bUse3DPreview || !ItemsWrapBox) return;

	for (int32 i = 0; i < ItemsWrapBox->GetChildrenCount() && i < VisibleItemIndices.Num(); ++i)
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
						const int32 ItemIndex = VisibleItemIndices[i];
						if (Inventory && Inventory->GetItems().IsValidIndex(ItemIndex)
							&& Inventory->GetItems()[ItemIndex].Data
							&& (Inventory->GetItems()[ItemIndex].Data->InspectMesh
								|| Inventory->GetItems()[ItemIndex].Data->InspectSkeletalMesh)
							&& PreviewRenderTargets.IsValidIndex(ItemIndex) && PreviewRenderTargets[ItemIndex])
						{
							FSlateBrush PreviewBrush;
							PreviewBrush.SetResourceObject(PreviewRenderTargets[ItemIndex]);
							PreviewBrush.ImageSize = FVector2D(144.0f, 120.0f);
							PreviewBrush.DrawAs = ESlateBrushDrawType::Image;
							PreviewImage->SetBrush(PreviewBrush);
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
			PreviewImage->SetDesiredSizeOverride(FVector2D(144.0f, 120.0f));

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
	const int32 NewChildIndex = VisibleItemIndices.IndexOfByKey(NewSelectedIndex);
	const int32 ChildCount = ItemsWrapBox->GetChildrenCount();
	const float CenteredSelectionOffset = NewChildIndex != INDEX_NONE
		? (static_cast<float>(NewChildIndex) - (static_cast<float>(ChildCount) - 1.0f) * 0.5f) * 184.0f
		: 0.0f;

	for (int32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
	{
		const int32 Distance = NewChildIndex == INDEX_NONE ? 1 : FMath::Abs(ChildIndex - NewChildIndex);
		const float Scale = ChildIndex == NewChildIndex ? 1.15f : (Distance == 1 ? 0.86f : (Distance == 2 ? 0.68f : 0.54f));
		const float Opacity = ChildIndex == NewChildIndex ? 1.0f : (Distance == 1 ? 0.78f : (Distance == 2 ? 0.52f : 0.30f));
		UWidget* ChildWidget = ItemsWrapBox->GetChildAt(ChildIndex);
		ChildWidget->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		ChildWidget->SetRenderTransform(FWidgetTransform(
			FVector2D(-CenteredSelectionOffset, static_cast<float>(Distance) * 7.0f),
			FVector2D(Scale, Scale), FVector2D::ZeroVector, 0.0f));
		ChildWidget->SetRenderOpacity(Opacity);

		if (UVerticalBox* SlotContainer = Cast<UVerticalBox>(ChildWidget))
		{
			if (UPanelWidget* Panel = Cast<UPanelWidget>(SlotContainer))
			{
				if (UBorder* Border = Cast<UBorder>(Panel->GetChildAt(0)))
				{
					FSlateBrush Brush;
					Brush.TintColor = FSlateColor(ChildIndex == NewChildIndex
						? FLinearColor(1.0f, 0.8f, 0.1f, 1.0f)
						: FLinearColor(0.08f, 0.08f, 0.1f, 1.0f));
					Brush.ImageSize = FVector2D(100.0f, 100.0f);
					Brush.DrawAs = ESlateBrushDrawType::Image;
					Border->SetBrush(Brush);
				}
			}
		}
	}

	LastSelectedIndex = NewSelectedIndex;
}

void UInventoryWidget::UpdateSelectedItemPreview(int32 SelectedIndex)
{
	SelectedPreviewTexture = nullptr;
	if (!SelectedItemPreview || !Inventory || !Inventory->GetItems().IsValidIndex(SelectedIndex))
	{
		if (SelectedItemPreview) SelectedItemPreview->SetVisibility(ESlateVisibility::Collapsed);
		if (SelectedItemName) SelectedItemName->SetText(FText::GetEmpty());
		if (SelectedItemDescription) SelectedItemDescription->SetText(FText::GetEmpty());
		return;
	}

	const FInventoryItem& Item = Inventory->GetItems()[SelectedIndex];
	if (!Item.Data)
	{
		SelectedItemPreview->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SelectedItemPreview->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	SelectedItemPreview->SetBrush(FSlateBrush());
	SelectedItemName->SetText(Item.Data->DisplayName);
	SelectedItemDescription->SetText(Item.Data->Description);

	if ((Item.Data->InspectMesh || Item.Data->InspectSkeletalMesh)
		&& PreviewRenderTargets.IsValidIndex(SelectedIndex) && PreviewRenderTargets[SelectedIndex])
	{
		SelectedPreviewTexture = PreviewRenderTargets[SelectedIndex];
		return;
	}

	if (Item.Data->Icon)
	{
		SelectedPreviewTexture = Item.Data->Icon;
	}
	else
	{
		SelectedItemPreview->SetVisibility(ESlateVisibility::Collapsed);
	}
}
