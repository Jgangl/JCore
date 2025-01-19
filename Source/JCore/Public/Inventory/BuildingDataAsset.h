// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Buildable.h"
#include "Engine/DataAsset.h"

#include "BuildingDataAsset.generated.h"

UCLASS(Blueprintable, BlueprintType)
class JCORE_API UBuildingDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UTexture2D* Icon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<ABuildable> BuildableClass;

public:
    UFUNCTION(BlueprintCallable, BlueprintPure)
    FString GetDisplayName() const { return this->Name; }

    UFUNCTION(BlueprintCallable, BlueprintPure)
    UTexture2D* GetIcon() const { return this->Icon; }

    UFUNCTION(BlueprintCallable, BlueprintPure)
    TSubclassOf<ABuildable> GetBuildableClass() const { return this->BuildableClass; }
};
