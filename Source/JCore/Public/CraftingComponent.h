// Fill out your copyright notice in the Description page of Project Settings.

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

    virtual void TickComponent(float DeltaTime,
                               ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

    bool TryCraftRecipe(UItemRecipeDataAsset* Recipe,
                    UInventoryComponent*  SourceInventory,
                    UInventoryComponent*  TargetInventory);

protected:
    // Called when the game starts
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable)
    void CraftRecipe(UItemRecipeDataAsset* Recipe,
                     UInventoryComponent*  SourceInventory,
                     UInventoryComponent*  TargetInventory);

    UFUNCTION(Server, Reliable)
    void ServerCraftRecipe(UItemRecipeDataAsset* Recipe,
                           UInventoryComponent*  SourceInventory,
                           UInventoryComponent*  TargetInventory);

    static bool InventoryHasItemsInRecipe(UItemRecipeDataAsset* Recipe, UInventoryComponent* InventoryComponent);
};
