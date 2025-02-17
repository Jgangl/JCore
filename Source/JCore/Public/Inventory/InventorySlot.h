// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemDataAsset.h"
#include "InventorySlot.generated.h"

/**
 *
 */
USTRUCT(BlueprintType)
struct FInventorySlot
{
    GENERATED_USTRUCT_BODY()

    FInventorySlot()
    {
        Item = nullptr;
        CurrentStackSize = 0;
    };

    FInventorySlot(UItemDataAsset* NewItem)
    {
        Item = NewItem;
        CurrentStackSize = 1;
    };

    FInventorySlot(UItemDataAsset* NewItem, int32 Amount)
    {
        Item = NewItem;
        CurrentStackSize = Amount;
    };

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    UItemDataAsset* Item;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int CurrentStackSize;

    void Clear()
    {
        Item = nullptr;
        CurrentStackSize = 0;
    }

    FORCEINLINE bool operator==(const FInventorySlot& OtherSlot) const
    {
        return (Item             == OtherSlot.Item &&
                CurrentStackSize == OtherSlot.CurrentStackSize);
    }

    FORCEINLINE bool operator!=(const FInventorySlot& OtherSlot) const
    {
        return (Item             != OtherSlot.Item ||
                CurrentStackSize != OtherSlot.CurrentStackSize);
    }
};
