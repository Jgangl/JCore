// Fill out your copyright notice in the Description page of Project Settings.

#include "Graph/GraphNodeComponent.h"

#include "Graph/GraphSubsystem.h"
#include "Kismet/GameplayStatics.h"

UGraphNodeComponent::UGraphNodeComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    this->SetIsReplicatedByDefault(true);

    this->Node = nullptr;
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

    this->Node = NewObject<UNodeBase>(this);
    this->Node->SetLocation(Owner->GetActorLocation());

    UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(GetWorld());

    if (!GameInstance)
    {
        return;
    }

    UGraphSubsystem* GraphSubsystem = GameInstance->GetSubsystem<UGraphSubsystem>();

    if (!GraphSubsystem)
    {
        return;
    }

    if (UGraphBase* Graph = GraphSubsystem->GetGraph())
    {
        Graph->AddNode(this->Node);
    }
}

UNodeBase* UGraphNodeComponent::GetNode() const
{
    return this->Node;
}
