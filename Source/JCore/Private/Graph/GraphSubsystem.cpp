#include "Graph/GraphSubsystem.h"

UGraphSubsystem::UGraphSubsystem()
{
}

void UGraphSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    this->Graphs.Empty();
    this->CreateGraph();
}

void UGraphSubsystem::Deinitialize()
{
    this->Graphs.Empty();
}

UGraphBase* UGraphSubsystem::CreateGraph()
{
    UGraphBase* NewGraph = NewObject<USteamGraph>();

    this->Graphs.Add(NewGraph);

    return NewGraph;
}

UGraphBase* UGraphSubsystem::GetGraph()
{
    // TODO: Handle multiple graphs
    return this->Graphs.Last();
}
