
#include "Graph/NodeBase.h"

UNodeBase::UNodeBase()
{
    this->MaxConnections = -1;
}

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
