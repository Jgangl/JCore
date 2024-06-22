// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemDataAsset.h"
#include "Engine/DataAsset.h"

#include "ItemRecipeDataAsset.generated.h"

/**
 *
 */
UCLASS()
class JCORE_API UItemRecipeDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<UItemDataAsset*, int32> InItems;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<UItemDataAsset*, int32> OutItems;

public:
    TMap<UItemDataAsset*, int32> GetInItems() { return InItems; }

    TMap<UItemDataAsset*, int32> GetOutItems() { return OutItems; }
};
