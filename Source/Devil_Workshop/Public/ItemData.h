#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ItemData.generated.h"

class UTexture2D;
class UStaticMesh;
class USkeletalMesh;
class UMaterialInterface;

UCLASS(BlueprintType)
class UItemData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FName ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UStaticMesh> InspectMesh;

	/** Optional skeletal mesh used by the inventory 3D preview. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Preview")
	TObjectPtr<USkeletalMesh> InspectSkeletalMesh;

	/** Optional per-slot material overrides for either inspect mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Preview")
	TArray<TObjectPtr<UMaterialInterface>> InspectMaterialOverrides;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TArray<FName> Tags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TMap<FName, float> DefaultValues;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TArray<FName> DefaultFlags;

	UFUNCTION(BlueprintPure, Category = "Item")
	bool HasTag(FName Tag) const { return Tags.Contains(Tag); }
};
