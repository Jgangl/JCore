// Copyright Joshua Gangl. All Rights Reserved.

#include "Graph/GraphDebugger.h"

struct FEdgePair
{
    FEdgePair()
    {
        Key   = nullptr;
        Value = nullptr;
    }

    FEdgePair(UNodeBase* Node, UNodeBase* OtherNode)
    {
        Key   = Node;
        Value = OtherNode;
    }

    UNodeBase* Key;
    UNodeBase* Value;

    bool operator==(const FEdgePair& OtherNode) const
    {
        return (Key == OtherNode.Key || Key == OtherNode.Value) &&
               (Value == OtherNode.Key || Value == OtherNode.Value);
    }
};

uint32 GetTypeHash(const FEdgePair& Node)
{
    return HashCombine(PointerHash((const void*)Node.Key), PointerHash((const void*)Node.Value));
}

AGraphDebugger::AGraphDebugger()
{
    this->PrimaryActorTick.bCanEverTick = true;

    this->Graph    = nullptr;
    this->bEnabled = true;
}

void AGraphDebugger::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (this->bEnabled)
    {
        this->DrawGraph();
    }
}

void AGraphDebugger::DrawGraph()
{
    if (!this->Graph)
    {
        UE_LOG(LogTemp, Error, TEXT("%hs : Graph is nullptr"), __FUNCTION__);
        return;
    }

    FVector Offset = FVector(0.0f, 0.0f, 100.0f);

    // Draw Node locations
    for (UNodeBase* Node : this->Graph->GetNodes())
    {
        if (!Node) continue;

        FVector Center = Node->GetLocation() + Offset;

        DrawDebugSolidBox(GetWorld(), Center, FVector(20.0f, 20.0f, 20.0f), FColor::Blue, false, -1, SDPG_MAX);
    }

    for (UEdgeBase* Edge : this->Graph->GetEdges())
    {
        FVector Direction = (Edge->Source->GetLocation() - Edge->Destination->GetLocation());
        Direction.Normalize();

        DrawDebugDirectionalArrow(GetWorld(),
                                  Edge->Source->GetLocation() - (Direction*20.0f) + Offset,
                                  Edge->Destination->GetLocation() + (Direction*20.0f) + Offset,
                                  200.0f,
                                  FColor::Yellow,
                                  false,
                                  -1,
                                  SDPG_Foreground,
                                  3);
    }

/*
    // Draw Edges between Nodes
    for (FEdgePair UniqueEdge: UniqueEdges)
    {
        FVector Direction = (UniqueEdge.Key->GetLocation() - UniqueEdge.Value->GetLocation());
        Direction.Normalize();

        DrawDebugLine(GetWorld(), UniqueEdge.Key->GetLocation() - (Direction*20.0f), UniqueEdge.Value->GetLocation() + (Direction*20.0f), FColor::Yellow, false, -1, SDPG_Foreground, 3);
    }
*/
}

void AGraphDebugger::SetGraph(UGraphBase* InGraph)
{
    this->Graph = InGraph;
}
