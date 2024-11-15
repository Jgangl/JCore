// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"

#include "Buildable.generated.h"

UCLASS(Abstract, Blueprintable)
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

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<UStaticMeshComponent*> StaticMeshComponents;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<UMaterialInterface*> OriginalMaterials;
};
