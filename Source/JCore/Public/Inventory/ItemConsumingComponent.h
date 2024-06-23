// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"

#include "Inventory/InventoryComponent.h"

#include "ItemConsumingComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemConsumed, UItemDataAsset*, ItemConsumed, int32, AmountConsumed);

DECLARE_LOG_CATEGORY_CLASS(LogItemConsumingComponent, Log, All)

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JCORE_API UItemConsumingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UItemConsumingComponent();

    UPROPERTY(BlueprintAssignable)
    FOnItemConsumed OnItemConsumed;

    UFUNCTION(BlueprintCallable)
    void StartConsuming();

    UFUNCTION(BlueprintCallable)
    void StopConsuming();

protected:

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TimeToConsume;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bLoop;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UItemDataAsset* ItemToConsume;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UInventoryComponent* InventoryComponent;

    FTimerHandle ConsumingTimerHandle;

    UFUNCTION()
    void OnConsumingTimerEnded();
};
