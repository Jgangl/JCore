// Copyright Joshua Gangl. All Rights Reserved.

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

void AGraphDebugger::BeginPlay()
{
    Super::BeginPlay();

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

    //this->SetGraph(FoundGraph);
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

    // Draw Node locations
    for (UNodeBase* Node : this->Graph->GetNodes())
    {
        if (!Node) continue;

        FVector Center = Node->GetLocation();

        DrawDebugSolidBox(GetWorld(), Center, FVector(20.0f, 20.0f, 20.0f), FColor::Blue, false, -1, SDPG_MAX);
    }
/*
    TArray<UEdgeBase*> UniqueEdges;

    for (UEdgeBase* Edge : this->Graph->GetEdges())
    {
        if (!Edge) continue;

        FVector Direction = (Edge->Source->GetLocation() - Edge->Destination->GetLocation());
        Direction.Normalize();

        if (Edge)

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
*/
    if (this->Graph->IsDirected())
    {
        // Draw Edges between Nodes
        for (UEdgeBase* Edge: this->Graph->GetEdges())
        {
            FVector Direction = (Edge->Source->GetLocation() - Edge->Destination->GetLocation());
            Direction.Normalize();

            DrawDebugDirectionalArrow(GetWorld(),
                                      Edge->Source->GetLocation() - (Direction*20.0f),
                                      Edge->Destination->GetLocation() + (Direction*20.0f),
                                      200.0f,
                                      FColor::Yellow,
                                      false,
                                      -1,
                                      SDPG_Foreground,
                                      3);
        }
    }
    else
    {
        // Draw Edges between Nodes
        for (UEdgeBase* Edge: this->Graph->GetEdges())
        {
            FVector Direction = (Edge->Source->GetLocation() - Edge->Destination->GetLocation());
            Direction.Normalize();

            DrawDebugLine(GetWorld(),
                          Edge->Source->GetLocation() - (Direction*20.0f),
                          Edge->Destination->GetLocation() + (Direction*20.0f),
                          FColor::Yellow,
                          false,
                          -1,
                          SDPG_Foreground,
                          3);
        }
    }
}

void AGraphDebugger::SetGraph(UGraphBase* InGraph)
{
    this->Graph = InGraph;
}

