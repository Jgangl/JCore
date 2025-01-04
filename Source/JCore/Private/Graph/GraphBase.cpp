// Fill out your copyright notice in the Description page of Project Settings.

#include "Graph/GraphBase.h"

UGraphBase::UGraphBase()
{
    this->bIsDirectedGraph = false;
}

UNodeBase* UGraphBase::AddNode(UNodeBase* NewNode)
{
    if (!NewNode)
    {
        return nullptr;
    }

    this->Nodes.AddUnique(NewNode);

    this->OnNodeAdded.Broadcast(NewNode);

    return NewNode;
}

UNodeBase* UGraphBase::AddNodeWithEdges(UNodeBase* NewNode, const TArray<UNodeBase*> &NeighborNodes)
{
    this->AddNode(NewNode);

    for (UNodeBase* NeighborNode : NeighborNodes)
    {
        if (!NeighborNode) continue;

        this->AddEdge(NewNode, NeighborNode);
    }

    return NewNode;
}

bool UGraphBase::RemoveNode(UNodeBase* NodeToRemove)
{
    if (!NodeToRemove)
    {
        UE_LOG(LogTemp, Error, TEXT("%hs : NodeToRemove is nullptr"), __FUNCTION__);
        return false;
    }

    // Delete all edges to the Node that is being removed
    for (UNodeBase* Node : this->Nodes)
    {
        if (!Node) continue;

        Node->RemoveConnectedNode(NodeToRemove);
    }

    this->Nodes.Remove(NodeToRemove);
    this->OnNodeRemoved.Broadcast(NodeToRemove);

    return true;
}

void UGraphBase::AddEdge(UNodeBase* FromNode, UNodeBase* ToNode)
{
    if (!FromNode)
    {
        return;
    }

    if (!ToNode)
    {
        return;
    }

    FromNode->AddConnectedNode(ToNode);

    // Also add edge to ToNode if this is an undirected graph
    if (!this->bIsDirectedGraph)
    {
        ToNode->AddConnectedNode(FromNode);
    }
}

void UGraphBase::RemoveEdge(UNodeBase* FromNode, UNodeBase* ToNode)
{
    if (!FromNode)
    {
        return;
    }

    if (!ToNode)
    {
        return;
    }

    // Remove node from first node's adjacency list
    if (this->Nodes.Contains(FromNode))
    {
        FromNode->RemoveEdge(ToNode);
    }

    // Remove node from second node's adjacency list
    if (this->Nodes.Contains(ToNode))
    {
        ToNode->RemoveEdge(FromNode);
    }
}

int32 UGraphBase::GetNumNodes()
{
    return this->Nodes.Num();
}

int32 UGraphBase::GetNumEdges()
{
    int32 NumEdges = 0;

    for (UNodeBase* Node : this->Nodes)
    {
        if (!Node) continue;

        NumEdges += Node->GetAdjacencyList().Num();
    }

    return NumEdges;
}

TArray<UNodeBase*> UGraphBase::GetNodes()
{
    return this->Nodes;
}

bool UGraphBase::BreadthFirstSearch(UNodeBase* SourceNode, UNodeBase* TargetNode)
{
    if (!SourceNode)
    {
        UE_LOG(LogTemp, Error, TEXT("%hs : SourceNode is nullptr"), __FUNCTION__);
        return false;
    }

    if (!TargetNode)
    {
        UE_LOG(LogTemp, Error, TEXT("%hs : TargetNode is nullptr"), __FUNCTION__);
        return false;
    }

    TQueue<UNodeBase*> UnvisitedNodes;
    TArray<UNodeBase*> VisitedNodes;

    VisitedNodes.Add(SourceNode);
    UnvisitedNodes.Enqueue(SourceNode);

    while (!UnvisitedNodes.IsEmpty())
    {
        UNodeBase* CurrentNode;
        UnvisitedNodes.Dequeue(CurrentNode);

        if (!CurrentNode) continue;

        //UE_LOG(LogTemp, Warning, TEXT("Visiting Node: %s"), *CurrentNode->GetName());

        if (CurrentNode == TargetNode)
        {
            return true;
        }

        int NumNeighbors = CurrentNode->GetAdjacencyList().Num();

        //UE_LOG(LogTemp, Warning, TEXT("Num neighbors: %d"), NumNeighbors);

        for (UNodeBase* NeighborNode : CurrentNode->GetAdjacencyList())
        {
            // Don't visit the same node twice
            if (!VisitedNodes.Contains(NeighborNode))
            {
                VisitedNodes.Add(NeighborNode);
                UnvisitedNodes.Enqueue(NeighborNode);
            }
        }
    }

    return false;
}

bool UGraphBase::BreadthFirstSearchNodes(UNodeBase* SourceNode, TArray<UNodeBase*> TargetNodes)
{
    if (!SourceNode)
    {
        UE_LOG(LogTemp, Error, TEXT("%hs : SourceNode is nullptr"), __FUNCTION__);
        return false;
    }

    if (TargetNodes.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("%hs : TargetNode Array is empty"), __FUNCTION__);
        return false;
    }

    TQueue<UNodeBase*> UnvisitedNodes;
    TArray<UNodeBase*> VisitedNodes;

    VisitedNodes.Add(SourceNode);
    UnvisitedNodes.Enqueue(SourceNode);

    while (!UnvisitedNodes.IsEmpty())
    {
        UNodeBase* CurrentNode;
        UnvisitedNodes.Dequeue(CurrentNode);

        if (!CurrentNode) continue;

        //UE_LOG(LogTemp, Warning, TEXT("Visiting Node: %s"), *CurrentNode->GetName());

        if (TargetNodes.Contains(CurrentNode))
        {
            return true;
        }

        int NumNeighbors = CurrentNode->GetAdjacencyList().Num();

        //UE_LOG(LogTemp, Warning, TEXT("Num neighbors: %d"), NumNeighbors);

        for (UNodeBase* NeighborNode : CurrentNode->GetAdjacencyList())
        {
            // Don't visit the same node twice
            if (!VisitedNodes.Contains(NeighborNode))
            {
                VisitedNodes.Add(NeighborNode);
                UnvisitedNodes.Enqueue(NeighborNode);
            }
        }
    }

    return false;
}

