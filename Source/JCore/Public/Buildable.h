// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"

#include "Buildable.generated.h"

UENUM()
enum class ETargetSnapSlot
{
    Floor,
    Wall
};

UCLASS()
class JCORE_API ABuildable : public AActor
{
    GENERATED_BODY()

public:
    ABuildable();
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable)
    void SetMaterial(UMaterialInterface* NewMaterial);

    UFUNCTION(BlueprintCallable)
    void ResetMaterial();

    UFUNCTION(BlueprintCallable)
    ETargetSnapSlot GetTargetSnapSlot();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<UStaticMeshComponent*> StaticMeshComponents;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<UMaterialInterface*> OriginalMaterials;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ETargetSnapSlot TargetSnapSlot;
};
