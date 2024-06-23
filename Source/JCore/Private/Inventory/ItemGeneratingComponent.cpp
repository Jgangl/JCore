// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/ItemGeneratingComponent.h"

UItemGeneratingComponent::UItemGeneratingComponent()
{
    this->TimeToGenerate     = 1.0f;
    this->bLoop              = true;
    this->ItemToGenerate     = nullptr;
    this->InventoryComponent = nullptr;
}

void UItemGeneratingComponent::SetInventoryComponent(UInventoryComponent* InInventoryComponent)
{
    this->InventoryComponent = InInventoryComponent;
}

void UItemGeneratingComponent::StartGenerating()
{
    const UWorld* World = GetWorld();

    if (!World)
    {
        UE_LOG(LogItemGeneratingComponent, Error, TEXT("StartGenerating: World is nullptr"))
        return;
    }

    World->GetTimerManager().SetTimer(this->GeneratingTimerHandle,
                                      this,
                                      &UItemGeneratingComponent::OnGeneratingTimerEnded,
                                      this->TimeToGenerate,
                                      this->bLoop);
}

void UItemGeneratingComponent::StopGenerating()
{
    const UWorld* World = GetWorld();

    if (!World)
    {
        UE_LOG(LogItemGeneratingComponent, Error, TEXT("StartGenerating: World is nullptr"))
        return;
    }

    World->GetTimerManager().ClearTimer(this->GeneratingTimerHandle);
}

void UItemGeneratingComponent::OnGeneratingTimerEnded()
{
    if (!this->InventoryComponent)
    {
        UE_LOG(LogItemGeneratingComponent, Error, TEXT("OnGeneratingTimerEnded: InventoryComponent is nullptr"))
        return;
    }

    this->InventoryComponent->TryAddItem(this->ItemToGenerate, 1);
}
