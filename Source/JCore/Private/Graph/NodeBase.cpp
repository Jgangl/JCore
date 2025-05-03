
#include "Graph/NodeBase.h"

UNodeBase::UNodeBase()
{
    this->MaxConnections = -1;
}
/*
void UNodeBase::AddConnectedNode(UNodeBase* ConnectedNode)
{
    if (!ConnectedNode)
    {
        UE_LOG(LogTemp, Error, TEXT("%hs : ConnectedNode is nullptr"), __FUNCTION__);
        return;
    }

    if (this->MaxConnections != -1 && this->AdjacencyList.Num() == this->MaxConnections)
    {
        UE_LOG(LogTemp, Error, TEXT("%hs : Failed, Max number of connections reached."), __FUNCTION__);
        return;
    }

    this->AdjacencyList.Add(ConnectedNode);
}

void UNodeBase::RemoveConnectedNode(UNodeBase* DisconnectedNode)
{
    if (!DisconnectedNode)
    {
        UE_LOG(LogTemp, Error, TEXT("%hs : DisconnectedNode is nullptr"), __FUNCTION__);
        return;
    }

    this->AdjacencyList.Remove(DisconnectedNode);
}

void UNodeBase::RemoveEdge(UNodeBase* FromNode)
{
    if (!FromNode)
    {
        UE_LOG(LogTemp, Error, TEXT("%hs : FromNode is nullptr"), __FUNCTION__);
        return;
    }

    this->AdjacencyList.Remove(FromNode);
}

TArray<UNodeBase*> UNodeBase::GetAdjacencyList()
{
    return this->AdjacencyList;
}
*/
void UNodeBase::SetLocation(const FVector &InLocation)
{
    this->Location = InLocation;
}

const FVector& UNodeBase::GetLocation() const
{
    return this->Location;
}

void UNodeBase::PostEdgesAdded()
{

}
