// Copyright Joshua Gangl. All Rights Reserved.

#include "Building/BuildingConnectionComponent.h"

#include "Graph/GraphNodeComponent.h"
#include "Graph/GraphSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

UBuildingConnectionComponent::UBuildingConnectionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    this->SetIsReplicatedByDefault(true);

    this->SetArrowFColor(FColor::White);
    this->SetArrowSize(1.5f);
    this->SetArrowLength(30.0f);

    this->ConnectedComponent = nullptr;
}

void UBuildingConnectionComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
    this->OnConnectionConnected.RemoveAll(this);
    this->OnConnectionDisconnected.RemoveAll(this);

    Super::OnComponentDestroyed(bDestroyingHierarchy);
}

UBuildingConnectionComponent* UBuildingConnectionComponent::GetConnectedComponent()
{
    return this->ConnectedComponent;
}

void UBuildingConnectionComponent::SetConnectedComponent(UBuildingConnectionComponent* InConnectionComponent)
{
    this->ConnectedComponent = InConnectionComponent;

    if (this->ConnectedComponent)
    {
        this->OnConnectionConnected.Broadcast(this, this->ConnectedComponent);
    }
    else
    {
        this->OnConnectionDisconnected.Broadcast(this);
    }
}

bool UBuildingConnectionComponent::IsConnected()
{
    return this->GetConnectedComponent() != nullptr;
}

void UBuildingConnectionComponent::DisconnectConnections()
{
    UBuildingConnectionComponent* OtherConnection = this->GetConnectedComponent();

    // We don't have a connection
    if (!OtherConnection)
    {
        return;
    }

    OtherConnection->SetConnectedComponent(nullptr);
    this->SetConnectedComponent(nullptr);
}

const FTransform& UBuildingConnectionComponent::GetSnapTransform() const
{
    return this->GetComponentTransform();
}

