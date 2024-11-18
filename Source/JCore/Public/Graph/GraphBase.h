// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NodeBase.h"

#include "GraphBase.generated.h"

UCLASS(BlueprintType)
class JCORE_API UGraphBase : public UObject
{
    GENERATED_BODY()

public:
    UGraphBase();

    UFUNCTION(BlueprintCallable)
    UNodeBase* AddNode(UNodeBase* NewNode);

    UFUNCTION(BlueprintCallable)
    bool RemoveNode(UNodeBase* NodeToRemove);

    UFUNCTION(BlueprintCallable)
    void AddEdge(UNodeBase* FromNode, UNodeBase* ToNode);

    UFUNCTION(BlueprintCallable)
    void RemoveEdge(UNodeBase* FromNode, UNodeBase* ToNode);

    UFUNCTION(BlueprintCallable)
    int32 GetNumNodes();

    UFUNCTION(BlueprintCallable)
    int32 GetNumEdges();

    UFUNCTION(BlueprintCallable)
    TArray<UNodeBase*> GetNodes();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<UNodeBase*> Nodes;

    UPROPERTY(EditAnywhere)
    bool bIsDirectedGraph;
};
