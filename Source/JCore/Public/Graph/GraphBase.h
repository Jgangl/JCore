// Copyright Joshua Gangl. All Rights Reserved.

#pragma once

#include "NodeBase.h"

#include "GraphBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNodeAdded, UNodeBase*, AddedNode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNodeRemoved, UNodeBase*, RemovedNode);

UCLASS(BlueprintType)
class JCORE_API UGraphBase : public UObject
{
    GENERATED_BODY()

public:
    UGraphBase();

    UFUNCTION(BlueprintCallable)
    virtual UNodeBase* AddNode(UNodeBase* NewNode);

    UFUNCTION(BlueprintCallable)
    virtual UNodeBase* AddNodeWithEdges(UNodeBase* NewNode, const TArray<UNodeBase*> &NeighborNodes);

    UFUNCTION(BlueprintCallable)
    virtual bool RemoveNode(UNodeBase* NodeToRemove);

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

    UFUNCTION(BlueprintCallable)
    bool BreadthFirstSearch(UNodeBase* SourceNode, UNodeBase* TargetNode);

    UFUNCTION(BlueprintCallable)
    bool BreadthFirstSearchNodes(UNodeBase* SourceNode, TArray<UNodeBase*> TargetNodes);

    UPROPERTY(BlueprintAssignable)
    FOnNodeAdded OnNodeAdded;

    UPROPERTY(BlueprintAssignable)
    FOnNodeRemoved OnNodeRemoved;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<UNodeBase*> Nodes;

    UPROPERTY(EditAnywhere)
    bool bIsDirectedGraph;
};
