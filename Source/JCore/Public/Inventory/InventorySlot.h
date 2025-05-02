// Copyright Joshua Gangl. All Rights Reserved.

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

    /**
     *  Is the given FInventorySlot full?
     *
     *  @param SlotToCheck  The FInventorySlot to check fullness
     *
     *  @return True if the given FInventorySlot is full
     */
    static bool IsSlotFull(const FInventorySlot& SlotToCheck)
    {
        UItemDataAsset* ItemDataAsset = SlotToCheck.Item;

        if (!ItemDataAsset)
        {
            return false;
        }

        return SlotToCheck.CurrentStackSize == ItemDataAsset->GetMaxStackSize();
    };

    /**
     *  Is the given FInventorySlot empty?
     *
     *  @param SlotToCheck  The FInventorySlot to check emptiness
     *
     *  @return True if the given FInventorySlot is empty
     */
    static bool IsSlotEmpty(const FInventorySlot& SlotToCheck)
    {
        return SlotToCheck.Item == nullptr;
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
