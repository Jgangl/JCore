// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"

#include "Inventory/InventoryComponent.h"

#include "ItemGeneratingComponent.generated.h"

DECLARE_LOG_CATEGORY_CLASS(LogItemGeneratingComponent, Log, All)

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JCORE_API UItemGeneratingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UItemGeneratingComponent();

    UFUNCTION(BlueprintCallable)
    void SetInventoryComponent(UInventoryComponent* InInventoryComponent);

    UFUNCTION(BlueprintCallable)
    void StartGenerating();

    UFUNCTION(BlueprintCallable)
    void StopGenerating();

protected:

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TimeToGenerate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bLoop;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UItemDataAsset* ItemToGenerate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UInventoryComponent* InventoryComponent;

    FTimerHandle GeneratingTimerHandle;

    UFUNCTION()
    void OnGeneratingTimerEnded();
};
