// Copyright Joshua Gangl. All Rights Reserved.

#include "Inventory/InventoryComponent.h"

#include "Net/UnrealNetwork.h"

UInventoryComponent::UInventoryComponent()
{
    this->PrimaryComponentTick.bCanEverTick = true;
    this->NumberOfSlots = 10;

    this->SetIsReplicatedByDefault(true);
}

void UInventoryComponent::OnRegister()
{
    Super::OnRegister();

    if (!GetOwner()->HasAuthority())
    {
        return;
    }

    this->InitializeInventorySlots();
}

void UInventoryComponent::SwapInventorySlots(int32 SourceIndex,
                                             UInventoryComponent* SourceInventory,
                                             int32 TargetIndex)
{
    this->ServerSwapInventorySlots(SourceIndex, SourceInventory, TargetIndex);
}

void UInventoryComponent::ServerSwapInventorySlots_Implementation(int32 SourceIndex,
                                                                  UInventoryComponent* SourceInventory,
                                                                  int32 TargetIndex)
{
    if (!SourceInventory)
    {
        UE_LOG(LogInventoryComponent, Error, TEXT("ServerSwapInventorySlots: SourceInventory is nullptr"))
        return;
    }

    // Invalid Index
    if (SourceIndex < 0 || SourceInventory->GetInventorySlots().Num() <= SourceIndex)
    {
        UE_LOG(LogInventoryComponent, Error, TEXT("ServerSwapInventorySlots: SourceIndex is Invalid"))
        return;
    }

    const FInventorySlot TempSlot = SourceInventory->GetInventorySlots()[SourceIndex];

    if (TargetIndex < 0 || this->GetInventorySlots().Num() <= TargetIndex)
    {
        // Invalid Index
        UE_LOG(LogInventoryComponent, Error, TEXT("ServerSwapInventorySlots: TargetIndex is Invalid"))
        return;
    }

    SourceInventory->InventorySlots[SourceIndex] = this->InventorySlots[TargetIndex];

    this->InventorySlots[TargetIndex] = TempSlot;

    if (SourceInventory != this)
    {
        SourceInventory->OnItemChanged.Broadcast();
    }

    this->OnItemChanged.Broadcast();
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UInventoryComponent, InventorySlots);
    DOREPLIFETIME(UInventoryComponent, NumberOfSlots);
}

void UInventoryComponent::InitializeInventorySlots()
{
    this->InventorySlots.Init(FInventorySlot(), this->NumberOfSlots);
    this->OnItemChanged.Broadcast();
}

bool UInventoryComponent::TryAddItem(UItemDataAsset* ItemToAdd, const int Amount)
{
    if (!ItemToAdd)
    {
        UE_LOG(LogTemp, Error, TEXT("TryAddItem: ItemToAdd was null"))
        return false;
    }

    APawn* OwningPawn = Cast<APawn>(GetOwner());

    // Add item if it already exists in inventory
    for (int i = 0; i < this->InventorySlots.Num(); i++)
    {
        FInventorySlot InventorySlot = this->InventorySlots[i];

        if (FInventorySlot::IsSlotFull(InventorySlot))
        {
            continue;
        }

        // Item Exists in Inventory slot
        if (ItemToAdd == InventorySlot.Item)
        {
            const int32 NewAmount       = InventorySlot.CurrentStackSize + Amount;
            const int32 MaxStackSize    = ItemToAdd->GetMaxStackSize();
            const int32 StackDifference = NewAmount - MaxStackSize;

            InventorySlot.CurrentStackSize = FMath::Min(NewAmount,
                                                        MaxStackSize);

            this->InventorySlots[i] = InventorySlot;

            if (StackDifference > 0)
            {
                // Add Leftovers
                this->ServerAddItem(ItemToAdd, StackDifference);
            }

            this->OnItemAdded.Broadcast(ItemToAdd, Amount);

            // Notify clients of item adding/removing
            if (OwningPawn && !OwningPawn->IsLocallyControlled())
            {
                this->ClientOnAddItem(ItemToAdd, Amount);
            }

            this->OnItemChanged.Broadcast();

            return true;
        }
    }

    // Find first empty slot
    for (int i = 0; i < this->InventorySlots.Num(); i++)
    {
        FInventorySlot InventorySlot = this->InventorySlots[i];

        // Slot is occupied
        if (InventorySlot.Item != nullptr)
        {
            continue;
        }

        InventorySlot.Item = ItemToAdd;
        InventorySlot.CurrentStackSize = FMath::Min(Amount, ItemToAdd->GetMaxStackSize());

        this->InventorySlots[i] = InventorySlot;

        this->OnItemAdded.Broadcast(ItemToAdd, Amount);

        // Notify clients of item adding/removing
        if (OwningPawn && !OwningPawn->IsLocallyControlled())
        {
            this->ClientOnAddItem(ItemToAdd, Amount);
        }

        this->OnItemChanged.Broadcast();

        return true;
    }

    UE_LOG(LogTemp, Warning, TEXT("TryAddItem: Inventory Full"))

    return false;
}

bool UInventoryComponent::TryAddItems(const TMap<UItemDataAsset*, int32>& ItemsToAdd)
{
    for (const TTuple<UItemDataAsset*, int> ItemToAdd : ItemsToAdd)
    {
        if (!this->TryAddItem(ItemToAdd.Key, ItemToAdd.Value))
        {
            return false;
        }
    }

    return true;
}

void UInventoryComponent::ServerAddItem_Implementation(UItemDataAsset* ItemToAdd, const int Amount)
{
    if (!ItemToAdd)
    {
        UE_LOG(LogTemp, Error, TEXT("ServerAddItem: ItemToAdd was nullptr"))
        return;
    }

    if (!this->TryAddItem(ItemToAdd, Amount))
    {
        UE_LOG(LogTemp, Error, TEXT("ServerAddItem: TryAddItem failed"))
    }
}

void UInventoryComponent::ClientOnAddItem_Implementation(UItemDataAsset* ItemToAdd, const int Amount)
{
    // Broadcast on clients to handle UI
    this->OnItemAdded.Broadcast(ItemToAdd, Amount);
}

bool UInventoryComponent::TryRemoveItem(UItemDataAsset* ItemToRemove, int Amount)
{
    if (!ItemToRemove)
    {
        UE_LOG(LogTemp, Error, TEXT("TryRemoveItem: ItemToRemove was null"))
        return false;
    }

    if (this->ContainsItem(ItemToRemove) < Amount)
    {
        // Not enough of the item
        return false;
    }

    int32 NumItemsRemoved = 0;

    for (int i = 0; i < this->InventorySlots.Num(); i++)
    {
        FInventorySlot InventorySlot = this->InventorySlots[i];

        // Ignore empty slots and items that don't match the given item
        if (FInventorySlot::IsSlotEmpty(InventorySlot) || InventorySlot.Item != ItemToRemove) continue;

        const int32 NumItemsRemovedAttempted = NumItemsRemoved + InventorySlot.CurrentStackSize;

        if (NumItemsRemovedAttempted > Amount)
        {
            // Remove partial stack
            const int32 TempNumItemsToRemove = Amount - NumItemsRemoved;
            NumItemsRemoved += TempNumItemsToRemove;

            InventorySlot.CurrentStackSize -= TempNumItemsToRemove;
            this->InventorySlots[i] = InventorySlot;
        }
        else
        {
            // Remove entire stack
            NumItemsRemoved += InventorySlot.CurrentStackSize;

            this->InventorySlots[i] = FInventorySlot();
        }

        if (NumItemsRemoved == Amount)
        {
            break;
        }
    }

    this->OnItemRemoved.Broadcast(ItemToRemove, Amount);

    APawn* OwningPawn = Cast<APawn>(GetOwner());

    // Notify clients of item adding/removing
    if (OwningPawn && !OwningPawn->IsLocallyControlled())
    {
        this->ClientRemoveItem(ItemToRemove, Amount);
    }

    this->OnItemChanged.Broadcast();

    return true;
}

bool UInventoryComponent::TryRemoveItems(const TMap<UItemDataAsset*, int32>& ItemsToRemove)
{
    if (!this->ContainsGivenItems(ItemsToRemove))
    {
        return false;
    }

    for (const TTuple<UItemDataAsset*, int> ItemToRemove : ItemsToRemove)
    {
        if (!this->TryRemoveItem(ItemToRemove.Key, ItemToRemove.Value))
        {
            return false;
        }
    }

    return true;
}

bool UInventoryComponent::TryRemoveItemAtIndex(int32 IndexToRemove, const int Amount)
{
    if (!this->InventorySlots.IsValidIndex(IndexToRemove))
    {
        UE_LOG(LogTemp, Error, TEXT("TryRemoveItemAtIndex: IndexToRemove is invalid"))
        return false;
    }

    FInventorySlot TempSlot = this->InventorySlots[IndexToRemove];

    // Not enough amount
    if (TempSlot.CurrentStackSize < Amount)
    {
        return false;
    }

    if (TempSlot.CurrentStackSize > Amount)
    {
        TempSlot.CurrentStackSize -= Amount;

        this->InventorySlots[IndexToRemove] = TempSlot;
    }
    else
    {
        this->InventorySlots[IndexToRemove] = FInventorySlot();
    }


    this->OnItemRemoved.Broadcast(TempSlot.Item, Amount);

    APawn* OwningPawn = Cast<APawn>(GetOwner());

    // Notify clients of item adding/removing
    if (OwningPawn && !OwningPawn->IsLocallyControlled())
    {
        this->ClientRemoveItem(TempSlot.Item, Amount);
    }

    this->OnItemChanged.Broadcast();

    return true;
}

void UInventoryComponent::ServerRemoveItem_Implementation(UItemDataAsset* ItemToRemove, const int Amount)
{
    if (!ItemToRemove)
    {
        UE_LOG(LogTemp, Error, TEXT("ServerRemoveItem: ItemToRemove was null"))
        return;
    }

    if (!this->TryRemoveItem(ItemToRemove, Amount))
    {
        UE_LOG(LogTemp, Error, TEXT("ServerRemoveItem: Failed to remove %d %s"), Amount, *ItemToRemove->GetName())
    }
}

void UInventoryComponent::ClientRemoveItem_Implementation(UItemDataAsset* ItemToRemove, const int Amount)
{
    this->OnItemRemoved.Broadcast(ItemToRemove, Amount);
}

bool UInventoryComponent::ContainsItemAmount(UItemDataAsset* ItemToCheck, const int Amount) const
{
    if (!ItemToCheck)
    {
        UE_LOG(LogTemp, Error, TEXT("ContainsItemAmount: ItemToCheck was null"))
        return false;
    }

    // TODO: Handle checking for split stacks adding up to the correct amount
    // Currently this will only check each slot individually and not add up totals

    for (int i = 0; i < this->InventorySlots.Num(); i++)
    {
        const FInventorySlot InventorySlot = this->InventorySlots[i];

        if (FInventorySlot::IsSlotEmpty(InventorySlot))
        {
            continue;
        }

        const bool bHasItem = ItemToCheck == InventorySlot.Item;
        const bool bHasAmount = Amount <= InventorySlot.CurrentStackSize;

        if (bHasItem && bHasAmount)
        {
            return true;
        }
    }

    return false;
}

bool UInventoryComponent::HasAvailableSpaceForItem(UItemDataAsset* ItemToCheck, const int Amount) const
{
    if (!ItemToCheck)
    {
        UE_LOG(LogTemp, Error, TEXT("%hs : ItemToCheck is nullptr"), __FUNCTION__);
        return false;
    }

    const int32 MaxStackSize = ItemToCheck->GetMaxStackSize();

    int32 NumAvailableSpots = 0;

    for (int i = 0; i < this->InventorySlots.Num(); i++)
    {
        const FInventorySlot& InventorySlot = this->InventorySlots[i];

        if (FInventorySlot::IsSlotFull(InventorySlot))
        {
            continue;
        }

        // Item Exists in Inventory slot
        if (ItemToCheck == InventorySlot.Item)
        {
            NumAvailableSpots += MaxStackSize - InventorySlot.CurrentStackSize;
        }

        if (FInventorySlot::IsSlotEmpty(InventorySlot))
        {
            NumAvailableSpots += MaxStackSize;
        }
    }

    if (NumAvailableSpots < Amount)
    {
        return false;
    }

    return true;
}

bool UInventoryComponent::HasAvailableSpaceForItems(const TMap<UItemDataAsset*, int32>& InItems) const
{
    // TODO: Implement
    UE_LOG(LogInventoryComponent, Error, TEXT("UNIMPLEMENTED"));
    return false;
}

int32 UInventoryComponent::ContainsItem(UItemDataAsset* ItemToCheck)
{
    if (!ItemToCheck)
    {
        UE_LOG(LogTemp, Error, TEXT("ContainsItem: ItemToCheck was null"))
        return 0;
    }

    int32 NumItems = 0;

    for (int i = 0; i < this->InventorySlots.Num(); i++)
    {
        const FInventorySlot InventorySlot = this->InventorySlots[i];

        if (FInventorySlot::IsSlotEmpty(InventorySlot))
        {
            continue;
        }

        if (ItemToCheck == InventorySlot.Item)
        {
            NumItems += InventorySlot.CurrentStackSize;
        }
    }

    return NumItems;
}

bool UInventoryComponent::ContainsGivenItems(const TMap<UItemDataAsset*, int32>& Items) const
{
    for (const TTuple<UItemDataAsset*, int> Item : Items)
    {
        if (!this->ContainsItemAmount(Item.Key, Item.Value))
        {
            return false;
        }
    }

    return true;
}

int32 UInventoryComponent::ContainsPartialStack(UItemDataAsset* ItemToCheck)
{
    if (!ItemToCheck)
    {
        UE_LOG(LogTemp, Error, TEXT("ContainsItem: ItemToCheck was null"))
        return -1;
    }

    for (int i = 0; i < this->InventorySlots.Num(); i++)
    {
        const FInventorySlot InventorySlot = this->InventorySlots[i];

        if (FInventorySlot::IsSlotEmpty(InventorySlot)) continue;

        if (ItemToCheck == InventorySlot.Item && !FInventorySlot::IsSlotFull(InventorySlot))
        {
            return i;
        }
    }

    return -1;
}

// TODO: Should track in bool whenever adding/removing item
bool UInventoryComponent::HasAnyEmptySlots()
{
    bool bFull = true;

    for (int i = 0; i < this->InventorySlots.Num(); i++)
    {
        const FInventorySlot InventorySlot = this->InventorySlots[i];

        if (FInventorySlot::IsSlotEmpty(InventorySlot))
        {
            bFull = false;
            break;
        }
    }

    return bFull;
}

bool UInventoryComponent::IsInventoryEmpty()
{
    bool bEmpty = true;

    for (int i = 0; i < this->InventorySlots.Num(); i++)
    {
        const FInventorySlot InventorySlot = this->InventorySlots[i];

        if (!FInventorySlot::IsSlotEmpty(InventorySlot))
        {
            bEmpty = false;
            break;
        }
    }

    return bEmpty;
}

void UInventoryComponent::SetNumberOfSlots(int InNumberOfSlots)
{
    this->NumberOfSlots = InNumberOfSlots;
}


void UInventoryComponent::OnRep_InventorySlots()
{
    this->OnItemChanged.Broadcast();
}
