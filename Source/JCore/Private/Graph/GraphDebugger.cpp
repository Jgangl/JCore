// Fill out your copyright notice in the Description page of Project Settings.

#include "Graph/GraphDebugger.h"

#include "Graph/GraphSubsystem.h"
#include "Kismet/GameplayStatics.h"

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


    UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(GetWorld());

    if (!GameInstance)
    {
        UE_LOG(LogTemp, Warning, TEXT("Game Instance null"));
        return;
    }

    UGraphSubsystem* GraphSubsystem = GameInstance->GetSubsystem<UGraphSubsystem>();

    if (!GraphSubsystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("GraphSubsystem null"));
        return;
    }

    UGraphBase* FoundGraph = GraphSubsystem->GetGraph();

    this->SetGraph(FoundGraph);

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

    //TSet<TPair<UNodeBase*, UNodeBase*>> UniqueEdges;
    TSet<FEdgePair> UniqueEdges;

    // Draw Node locations
    for (UNodeBase* Node : this->Graph->GetNodes())
    {
        if (!Node) continue;

        FVector Center = Node->GetLocation();

        for (UNodeBase* NeighborNode : Node->GetAdjacencyList())
        {
            if (!NeighborNode) continue;

            FVector Direction = (NeighborNode->GetLocation() - Node->GetLocation());
            Direction.Normalize();

            // TODO: Need to only draw UNIQUE edges, we are currently drawing 2 lines for 1 edge
            //TPair<UNodeBase*, UNodeBase*> Edge(Node, NeighborNode);
            FEdgePair Edge;
            Edge.Key = Node;
            Edge.Value = NeighborNode;

            bool EdgeIsDuplicate = false;

            for (FEdgePair UniqueEdge: UniqueEdges)
            {
                if (UniqueEdge == Edge)
                {
                    EdgeIsDuplicate = true;
                }
            }

            if (!EdgeIsDuplicate)
            {
                UniqueEdges.Add(Edge);
            }
        }

        DrawDebugSolidBox(GetWorld(), Center, FVector(20.0f, 20.0f, 20.0f), FColor::Blue, false, -1, SDPG_MAX);
    }

    // Draw Edges between Nodes
    for (FEdgePair UniqueEdge: UniqueEdges)
    {
        FVector Direction = (UniqueEdge.Key->GetLocation() - UniqueEdge.Value->GetLocation());
        Direction.Normalize();

        DrawDebugLine(GetWorld(), UniqueEdge.Key->GetLocation() - (Direction*20.0f), UniqueEdge.Value->GetLocation() + (Direction*20.0f), FColor::Yellow, false, -1, SDPG_Foreground, 3);
    }
}

void AGraphDebugger::SetGraph(UGraphBase* InGraph)
{
    this->Graph = InGraph;
}

