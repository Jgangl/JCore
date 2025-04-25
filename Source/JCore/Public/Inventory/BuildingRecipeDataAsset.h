// Copyright Joshua Gangl. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "BuildingDataAsset.h"
#include "ItemDataAsset.h"

#include "BuildingRecipeDataAsset.generated.h"

UCLASS()
class JCORE_API UBuildingRecipeDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<UItemDataAsset*, int32> InItems;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UBuildingDataAsset* OutBuilding;

public:
    UFUNCTION(BlueprintCallable)
    const TMap<UItemDataAsset*, int32>& GetInItems() const { return InItems; }

    UFUNCTION(BlueprintCallable)
    UBuildingDataAsset* GetOutBuilding() const { return OutBuilding; }
};
