// Copyright Joshua Gangl. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InventorySlot.h"
#include "Components/ActorComponent.h"

#include "InventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnItemChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemAdded, UItemDataAsset*, ItemAdded, int32, AmountAdded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemRemoved, UItemDataAsset*, ItemRemoved, int32, AmountRemoved);

DECLARE_LOG_CATEGORY_CLASS(LogInventoryComponent, Log, All);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JCORE_API UInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInventoryComponent();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(BlueprintAssignable)
    FOnItemChanged OnItemChanged;

    UPROPERTY(BlueprintAssignable)
    FOnItemAdded OnItemAdded;

    UPROPERTY(BlueprintAssignable)
    FOnItemRemoved OnItemRemoved;

    void InitializeInventorySlots();

    UFUNCTION(BlueprintCallable)
    bool TryAddItem(UItemDataAsset* ItemToAdd, const int Amount = 1);

    UFUNCTION(BlueprintCallable)
    bool TryAddItems(const TMap<UItemDataAsset*, int32> &ItemsToAdd);

    UFUNCTION(Server, BlueprintCallable, Reliable)
    void ServerAddItem(UItemDataAsset* ItemToAdd, const int Amount = 1);

    UFUNCTION(Client, BlueprintCallable, Reliable)
    void ClientOnAddItem(UItemDataAsset* ItemToAdd, const int Amount = 1);

    /** Removes a given item and amount. *MUST BE CALLED ON SERVER* */
    UFUNCTION(BlueprintCallable)
    bool TryRemoveItem(UItemDataAsset* ItemToRemove, const int Amount = 1);

    /** Removes the given items and amounts. *MUST BE CALLED ON SERVER* */
    UFUNCTION(BlueprintCallable)
    bool TryRemoveItems(const TMap<UItemDataAsset*, int32> &ItemsToRemove);

    /** Removes item at the given index. *MUST BE CALLED ON SERVER* */
    UFUNCTION(BlueprintCallable)
    bool TryRemoveItemAtIndex(int32 IndexToRemove, const int Amount = 1);

    UFUNCTION(Server, BlueprintCallable, Reliable)
    void ServerRemoveItem(UItemDataAsset* ItemToRemove, const int Amount = 1);

    UFUNCTION(Client, BlueprintCallable, Reliable)
    void ClientRemoveItem(UItemDataAsset* ItemToRemove, const int Amount = 1);

    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool ContainsItemAmount(UItemDataAsset* ItemToCheck, const int Amount = 1) const;

    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool HasAvailableSpaceForItem(UItemDataAsset* ItemToCheck, const int Amount = 1) const;

    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool HasAvailableSpaceForItems(const TMap<UItemDataAsset*, int32> &InItems) const;

    UFUNCTION(BlueprintCallable, BlueprintPure)
    int32 ContainsItem(UItemDataAsset* ItemToCheck);

    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool ContainsGivenItems(const TMap<UItemDataAsset*, int32> &Items) const;

    /** Returns the index of a slot with a partial stack of the given item. Returns -1 if none found. */
    UFUNCTION(BlueprintCallable, BlueprintPure)
    int32 ContainsPartialStack(UItemDataAsset* ItemToCheck);

    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool HasAnyEmptySlots();

    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool IsInventoryEmpty();

    UFUNCTION(BlueprintCallable)
    void SetNumberOfSlots(int InNumberOfSlots);

    UFUNCTION(BlueprintCallable, BlueprintPure)
    TArray<FInventorySlot>& GetInventorySlots() { return this->InventorySlots; };

    UFUNCTION(BlueprintCallable)
    void SwapInventorySlots(int32 SourceIndex, UInventoryComponent* SourceInventory, int32 TargetIndex);

    UFUNCTION(Server, Reliable)
    void ServerSwapInventorySlots(int32 SourceIndex, UInventoryComponent* SourceInventory, int32 TargetIndex);

protected:
    virtual void OnRegister() override;

    UFUNCTION(BlueprintCallable, BlueprintPure)
    static bool IsSlotFull(const FInventorySlot& SlotToCheck);

    UFUNCTION(BlueprintCallable, BlueprintPure)
    static bool IsSlotEmpty(const FInventorySlot& SlotToCheck);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
    int NumberOfSlots;

    UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_InventorySlots)
    TArray<FInventorySlot> InventorySlots;

private:
    //! @brief  Called on clients when InventorySlots is updated
    UFUNCTION()
    void OnRep_InventorySlots();
};
