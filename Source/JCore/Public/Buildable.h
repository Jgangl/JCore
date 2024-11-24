// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"

#include "BuildableInterface.h"
#include "SteamFactory/PipeConnectionComponent.h"

#include "Buildable.generated.h"

DECLARE_LOG_CATEGORY_CLASS(LogBuildable, Log, All)

UCLASS(Abstract, Blueprintable)
class JCORE_API ABuildable : public AActor, public IBuildableInterface
{
    GENERATED_BODY()

public:
    ABuildable();

    virtual void OnConstruction(const FTransform& Transform) override;

    UFUNCTION(BlueprintCallable)
    void SetMaterial(UMaterialInterface* NewMaterial);

    UFUNCTION(BlueprintCallable)
    void ResetMaterial();

    const FVector& GetBuildingOffset() const;

    virtual void CompleteBuilding() override;

    virtual void GetPipeSnapLocations(TArray<FVector>& OutSnapLocations) const override;

protected:
    virtual void BeginPlay() override;

    virtual void Destroyed() override;

    bool RemoveGraphNode();

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    UStaticMeshComponent* StaticMeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<UMeshComponent*> MeshComponents;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<UMaterialInterface*> OriginalMaterials;

    UPROPERTY(EditAnywhere)
    FVector BuildingOffset;

    UPROPERTY(EditAnywhere)
    TArray<UPipeConnectionComponent*> PipeConnectionComponents;
};
