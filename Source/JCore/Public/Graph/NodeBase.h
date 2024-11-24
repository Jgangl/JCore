// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NodeBase.generated.h"

UCLASS(BlueprintType)
class JCORE_API UNodeBase : public UObject
{
    GENERATED_BODY()
public:
    UNodeBase();

    UFUNCTION(BlueprintCallable)
    void AddConnectedNode(UNodeBase* ConnectedNode);

    UFUNCTION(BlueprintCallable)
    void RemoveConnectedNode(UNodeBase* DisconnectedNode);

    UFUNCTION(BlueprintCallable)
    void RemoveEdge(UNodeBase* FromNode);

    UFUNCTION(BlueprintCallable)
    TArray<UNodeBase*> GetAdjacencyList();

    UFUNCTION(BlueprintCallable)
    void SetLocation(const FVector &InLocation);

    UFUNCTION(BlueprintCallable)
    const FVector& GetLocation() const;

    UFUNCTION()
    virtual void PostEdgesAdded();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<UNodeBase*> AdjacencyList;

    int32 MaxConnections;

    UPROPERTY()
    FVector Location;
};
