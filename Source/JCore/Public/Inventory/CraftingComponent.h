// Copyright Joshua Gangl. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemRecipeDataAsset.h"

#include "CraftingComponent.generated.h"

DECLARE_LOG_CATEGORY_CLASS(LogCraftingComponent, Log, All)

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JCORE_API UCraftingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UCraftingComponent();

    /**
     *  Attempts to craft the given recipe
     *
     *  @note Expected to be called on the Server
     *
     *  @param Recipe  The recipe with the items to check
     *  @param Inventory  The UInventoryComponent to remove and add the output items to
     *
     *  @return True if Recipe was successfully crafted
     */
    UFUNCTION(BlueprintCallable, Category="Crafting")
    bool TryCraftRecipe(UItemRecipeDataAsset* Recipe,
                        UInventoryComponent*  Inventory);

    /**
     *  Attempts to craft the given recipe
     *
     *  @note Expected to be called on the Server
     *
     *  @param Recipe  The recipe with the items to check
     *  @param SourceInventory  The UInventoryComponent to remove input items from
     *  @param TargetInventory  The UInventoryComponent to add the output items to
     *
     *  @return True if Recipe was successfully crafted
     */
    UFUNCTION(BlueprintCallable, Category="Crafting")
    bool TryCraftRecipeToInventory(UItemRecipeDataAsset* Recipe,
                                   UInventoryComponent*  SourceInventory,
                                   UInventoryComponent*  TargetInventory);

    /**
     *  Attempts to craft the given recipe on the server
     *
     *  @param Recipe  The recipe with the items to check
     *  @param SourceInventory  The UInventoryComponent to remove the input items from
     *  @param TargetInventory  The UInventoryComponent to add the output items to, defaults to the SourceInventory
     */
    UFUNCTION(Server, Reliable, Category="Crafting")
    void ServerCraftRecipe(UItemRecipeDataAsset* Recipe,
                           UInventoryComponent*  SourceInventory,
                           UInventoryComponent*  TargetInventory);

    /**
     *  Checks whether a UInventoryComponent contains the necessary input items to craft the given recipe
     *
     *  @param Recipe  The recipe with the items to check
     *  @param InventoryComponent  The UInventoryComponent to check items from
     *
     *  @return True if inventory contains the input items from the recipe
     */
    UFUNCTION(BlueprintCallable, Category="Crafting")
    static bool InventoryHasItemsInRecipe(UItemRecipeDataAsset* Recipe,
                                          UInventoryComponent*  InventoryComponent);
};
