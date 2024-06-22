// Fill out your copyright notice in the Description page of Project Settings.

#include "CraftingComponent.h"
#include "Inventory/InventoryComponent.h"

UCraftingComponent::UCraftingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

    this->SetIsReplicatedByDefault(true);
}

void UCraftingComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UCraftingComponent::TickComponent(float DeltaTime,
                                       ELevelTick TickType,
                                       FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}


void UCraftingComponent::TryCraftRecipe(UItemRecipeDataAsset* Recipe, UInventoryComponent* SourceInventory, UInventoryComponent* TargetInventory)
{
    if (!Recipe)
    {
        UE_LOG(LogCraftingComponent, Error, TEXT("TryCraftRecipe: Recipe is nullptr"))
        return;
    }

    if (!SourceInventory)
    {
        UE_LOG(LogCraftingComponent, Error, TEXT("TryCraftRecipe: SourceInventory is nullptr"))
        return;
    }

    if (!TargetInventory)
    {
        UE_LOG(LogCraftingComponent, Error, TEXT("TryCraftRecipe: TargetInventory is nullptr"))
        return;
    }

    // Craft On Server
    this->ServerCraftRecipe(Recipe, SourceInventory, TargetInventory);
}

void UCraftingComponent::ServerCraftRecipe_Implementation(UItemRecipeDataAsset* Recipe, UInventoryComponent* SourceInventory, UInventoryComponent* TargetInventory)
{
    if (!Recipe)
    {
        UE_LOG(LogCraftingComponent, Error, TEXT("ServerCraftRecipe: Recipe is nullptr"))
        return;
    }

    if (!SourceInventory)
    {
        UE_LOG(LogCraftingComponent, Error, TEXT("ServerCraftRecipe: SourceInventory is nullptr"))
        return;
    }

    if (!TargetInventory)
    {
        UE_LOG(LogCraftingComponent, Error, TEXT("ServerCraftRecipe: TargetInventory is nullptr"))
        return;
    }

    this->CraftRecipe(Recipe, SourceInventory, TargetInventory);
}

void UCraftingComponent::CraftRecipe(UItemRecipeDataAsset* Recipe, UInventoryComponent* SourceInventory, UInventoryComponent* TargetInventory)
{
    if (!Recipe)
    {
        UE_LOG(LogCraftingComponent, Error, TEXT("CraftRecipe: Recipe is nullptr"))
        return;
    }

    if (!SourceInventory)
    {
        UE_LOG(LogCraftingComponent, Error, TEXT("CraftRecipe: SourceInventory is nullptr"))
        return;
    }

    if (!TargetInventory)
    {
        UE_LOG(LogCraftingComponent, Error, TEXT("CraftRecipe: TargetInventory is nullptr"))
        return;
    }

    if (!this->InventoryHasItemsInRecipe(Recipe, SourceInventory))
    {
        UE_LOG(LogCraftingComponent, Warning, TEXT("CraftRecipe: Crafting failed, inventory does not have necessary items"))
        return;
    }

    // Remove Recipe In Items
    for (const TPair<UItemDataAsset*, int32> ItemPair : Recipe->GetInItems())
    {
        UItemDataAsset* Item = ItemPair.Key;

        if (!Item)
        {
            UE_LOG(LogCraftingComponent, Error, TEXT("CraftRecipe: Item is nullptr"))
            return;
        }

        const int32 NumItems = ItemPair.Value;

        SourceInventory->TryRemoveItem(Item, NumItems);
    }

    // Add Recipe Out Items
    for (const TPair<UItemDataAsset*, int32> ItemPair : Recipe->GetOutItems())
    {
        UItemDataAsset* Item = ItemPair.Key;

        if (!Item)
        {
            UE_LOG(LogCraftingComponent, Error, TEXT("CraftRecipe: Item is nullptr"))
            return;
        }

        const int32 NumItems = ItemPair.Value;

        // TODO: This won't add if not enough room in inventory
        TargetInventory->TryAddItem(Item, NumItems);

        UE_LOG(LogCraftingComponent, Verbose, TEXT("CraftRecipe: Crafted: %s"), *Item->GetName())
    }
}

bool UCraftingComponent::InventoryHasItemsInRecipe(UItemRecipeDataAsset* Recipe,
                                                   UInventoryComponent*  InventoryComponent)
{
    if (!Recipe)
    {
        UE_LOG(LogCraftingComponent, Error, TEXT("InventoryHasItemsInRecipe: Recipe is nullptr"))
        return false;
    }

    if (!InventoryComponent)
    {
        UE_LOG(LogCraftingComponent, Error, TEXT("InventoryHasItemsInRecipe: InventoryComponent is nullptr"))
        return false;
    }

    // Remove Recipe In Items
    for (const TPair<UItemDataAsset*, int32> ItemPair : Recipe->GetInItems())
    {
        UItemDataAsset* Item = ItemPair.Key;

        if (!Item)
        {
            UE_LOG(LogCraftingComponent, Error, TEXT("InventoryHasItemsInRecipe: Item is nullptr"))
            return false;
        }

        const int32 NumItems = ItemPair.Value;

        if (!InventoryComponent->ContainsItemAmount(Item, NumItems))
        {
            return false;
        }
    }

    return true;
}
