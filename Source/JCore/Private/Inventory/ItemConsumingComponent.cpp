// Copyright Joshua Gangl. All Rights Reserved.

#include "Inventory/ItemConsumingComponent.h"

UItemConsumingComponent::UItemConsumingComponent()
{
    this->TimeToConsume      = 1.0f;
    this->bLoop              = true;
    this->ItemToConsume      = nullptr;
    this->InventoryComponent = nullptr;
}

void UItemConsumingComponent::StartConsuming()
{
    const UWorld* World = GetWorld();

    if (!World)
    {
        UE_LOG(LogItemConsumingComponent, Error, TEXT("StartConsuming: World is nullptr"))
        return;
    }

    World->GetTimerManager().SetTimer(this->ConsumingTimerHandle,
                                      this,
                                      &UItemConsumingComponent::OnConsumingTimerEnded,
                                      this->TimeToConsume,
                                      this->bLoop);
}

void UItemConsumingComponent::StopConsuming()
{
    const UWorld* World = GetWorld();

    if (!World)
    {
        UE_LOG(LogItemConsumingComponent, Error, TEXT("StartConsuming: World is nullptr"))
        return;
    }

    World->GetTimerManager().ClearTimer(this->ConsumingTimerHandle);
}

void UItemConsumingComponent::OnConsumingTimerEnded()
{
    if (!this->InventoryComponent)
    {
        UE_LOG(LogItemConsumingComponent, Error, TEXT("OnConsumingTimerEnded: InventoryComponent is nullptr"))
        return;
    }

    const int32 AmountToConsume = 1;

    if (this->InventoryComponent->TryRemoveItem(this->ItemToConsume, AmountToConsume))
    {
        this->OnItemConsumed.Broadcast(this->ItemToConsume, AmountToConsume);
    }
}
