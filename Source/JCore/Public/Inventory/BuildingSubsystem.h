// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "BuildingRecipeDataAsset.h"

#include "BuildingSubsystem.generated.h"

UCLASS()
class JCORE_API UBuildingSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UBuildingSubsystem();

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable)
    UBuildingRecipeDataAsset* GetRecipe(const TSubclassOf<ABuildable> InBuildableClass) const;

    UPROPERTY(EditAnywhere)
    TArray<UBuildingRecipeDataAsset*> AllBuildingRecipeDataAssets;

protected:
    void FindBuildingRecipeAssets();
};
