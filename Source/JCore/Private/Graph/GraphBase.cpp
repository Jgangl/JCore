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

