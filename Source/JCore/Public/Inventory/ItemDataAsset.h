// Copyright Joshua Gangl. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ItemDataAsset.generated.h"

/**
 *
 */
UCLASS(Blueprintable, BlueprintType)
class JCORE_API UItemDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bStackable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxStackSize;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UTexture2D* Icon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<AActor> InGameActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UStaticMesh* StaticMesh;

public:
    UFUNCTION(BlueprintCallable, BlueprintPure)
    FString GetName() { return this->Name; }

    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool IsStackable() const { return this->bStackable; }

    UFUNCTION(BlueprintCallable, BlueprintPure)
    int32 GetMaxStackSize() const { return this->MaxStackSize; }

    UFUNCTION(BlueprintCallable, BlueprintPure)
    UStaticMesh* GetStaticMesh() const { return this->StaticMesh; }
};
