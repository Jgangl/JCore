// Fill out your copyright notice in the Description page of Project Settings.

#include "Graph/GraphNodeComponent.h"

#include "Graph/GraphSubsystem.h"

UGraphNodeComponent::UGraphNodeComponent()
{
    this->PrimaryComponentTick.bCanEverTick = true;
    this->SetIsReplicatedByDefault(true);

    this->Node      = nullptr;
    this->NodeClass = UNodeBase::StaticClass();
}

void UGraphNodeComponent::BeginPlay()
{
    Super::BeginPlay();

    AActor* Owner = GetOwner();

    if (!Owner)
    {
        UE_LOG(LogTemp, Error, TEXT("%hs : Owner is nullptr"), __FUNCTION__);
        return;
    }

    this->Node = NewObject<UNodeBase>(this, this->NodeClass);
}

UNodeBase* UGraphNodeComponent::GetNode() const
{
    return this->Node;
}

void UGraphNodeComponent::SetNodeClass(TSubclassOf<UNodeBase> InNodeClass)
{
    this->NodeClass = InNodeClass;
}

void UGraphNodeComponent::SetNodeLocation(const FVector& InLocation)
{
    if (!this->Node)
    {
        UE_LOG(LogTemp, Error, TEXT("%hs : Node is nullptr"), __FUNCTION__);
        return;
    }

    this->Node->SetLocation(InLocation);
}
