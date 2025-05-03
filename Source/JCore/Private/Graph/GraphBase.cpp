// Copyright Joshua Gangl. All Rights Reserved.

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

    TArray<uint32> EdgeIndexesToRemove;
    for (int32 i = 0; i < this->Edges.Num(); i++ )
    {
        if (!this->Edges[i]) continue;

        if (this->Edges[i]->Source == NodeToRemove || this->Edges[i]->Destination == NodeToRemove)
        {
            EdgeIndexesToRemove.Add(i);
        }
    }

    // Remove edges in reverse order
    for( int32 i = this->Edges.Num() - 1; i >= 0; i-- )
    {
        if (EdgeIndexesToRemove.Contains(i))
        {
            this->Edges.RemoveAt(i);
        }
    }

    this->Nodes.Remove(NodeToRemove);
    this->OnNodeRemoved.Broadcast(NodeToRemove);

    return true;
}

void UGraphBase::AddEdge(UNodeBase* FromNode, UNodeBase* ToNode)
{
    if (!FromNode || !ToNode)
    {
        return;
    }

    // TODO: Don't allow duplicate edges?

    UEdgeBase* Edge = NewObject<UEdgeBase>(this);

    Edge->Source      = FromNode;
    Edge->Destination = ToNode;
}

void UGraphBase::RemoveEdge(UNodeBase* FromNode, UNodeBase* ToNode)
{
    if (!FromNode || !ToNode)
    {
        return;
    }

    UEdgeBase* EdgeToRemove = nullptr;

    for (UEdgeBase* Edge : this->Edges)
    {
        if (!Edge) continue;

        if (Edge->Source == FromNode && Edge->Destination == ToNode)
        {
            EdgeToRemove = Edge;
        }
    }

    if (EdgeToRemove)
    {
        this->Edges.Remove(EdgeToRemove);
    }
}

int32 UGraphBase::GetNumNodes()
{
    return this->Nodes.Num();
}

int32 UGraphBase::GetNumEdges()
{
    return this->Edges.Num();
}

TArray<UNodeBase*> UGraphBase::GetNodes()
{
    return this->Nodes;
}

TArray<UEdgeBase*> UGraphBase::GetEdges()
{
    return this->Edges;
}

bool UGraphBase::IsDirected()
{
    return this->bIsDirectedGraph;
}

bool UGraphBase::IsRootNode(UNodeBase* InNode)
{
    for (UEdgeBase* Edge : this->Edges)
    {
        if (Edge && Edge->Source == InNode)
        {
            return false;
        }
    }

    return true;
}

/*
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

        if (CurrentNode == TargetNode)
        {
            return true;
        }

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

        if (TargetNodes.Contains(CurrentNode))
        {
            return true;
        }

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
*/
