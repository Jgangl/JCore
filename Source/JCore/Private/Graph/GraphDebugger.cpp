// Copyright Joshua Gangl. All Rights Reserved.

#include "Graph/GraphDebugger.h"

#include "Kismet/GameplayStatics.h"
#include "SteamFactory/ConveyorManager.h"

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

    AConveyorManager* ConveyorManager = Cast<AConveyorManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AConveyorManager::StaticClass()));

    if (!ConveyorManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("Couldn't find a conveyor manager"));
        return;
    }

    UGraphBase* FoundGraph = ConveyorManager->GetGraph();

    this->SetGraph(FoundGraph);

    if (this->bEnabled)
    {
        this->DrawGraph();
        this->DrawUpdateOrder();
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

void AGraphDebugger::DrawUpdateOrder()
{
    if (!this->Graph)
    {
        return;
    }

    UItemTransportGraph* ItemTransportGraph = Cast<UItemTransportGraph>(this->Graph);

    if (!ItemTransportGraph) return;

    TArray<UItemTransportNode*> RootNodes = ItemTransportGraph->GetRootNodes();

    for (UItemTransportNode* RootNode : RootNodes)
    {
        if (!RootNode) continue;

        // Draw root nodes
        FVector Center = RootNode->GetLocation() + FVector(0.0f, 0.0f, 200.0f);
        DrawDebugSolidBox(GetWorld(), Center, FVector(20.0f, 20.0f, 20.0f), FColor::Red, false, -1, SDPG_MAX);

        TQueue<UItemTransportNode*> UnvisitedNodes;
        TArray<UItemTransportNode*> VisitedNodes;

        VisitedNodes.Add(RootNode);
        UnvisitedNodes.Enqueue(RootNode);

        // Not sure why this is necessary, but without it the debug strings aren't drawn correctly
        FlushDebugStrings(GetWorld());

        int32 CurrentUpdate = 1;

        while (!UnvisitedNodes.IsEmpty())
        {
            UItemTransportNode* CurrentNode;
            UnvisitedNodes.Dequeue(CurrentNode);

            if (!CurrentNode) continue;

            FVector Loc = CurrentNode->GetLocation() + FVector(0.0f, 0.0f, 150.0f);

            // Draw a number;
            DrawDebugString(GetWorld(), Loc, FString::FromInt(CurrentUpdate), this);
            CurrentUpdate++;

            for (UItemTransportNode* ChildrenNode : ItemTransportGraph->GetChildrenNodes(CurrentNode))
            {
                // Don't visit the same node twice
                if (!VisitedNodes.Contains(ChildrenNode))
                {
                    VisitedNodes.Add(ChildrenNode);
                    UnvisitedNodes.Enqueue(ChildrenNode);
                }
            }
        }
    }
}

