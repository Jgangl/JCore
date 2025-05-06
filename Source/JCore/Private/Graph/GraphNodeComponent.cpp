// Copyright Joshua Gangl. All Rights Reserved.

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
}

void UGraphNodeComponent::OnRegister()
{
    Super::OnRegister();

    UE_LOG(LogTemp, Warning, TEXT("OnRegister : %s"), *this->NodeClass->GetName());

    if (this->NodeClass)
    {
        this->Node = NewObject<UNodeBase>(this, this->NodeClass);
    }
}

void UGraphNodeComponent::InitializeComponent()
{
    Super::InitializeComponent();
    UE_LOG(LogTemp, Warning, TEXT("InitializeComponent : %s"), *this->NodeClass->GetName());
}

void UGraphNodeComponent::PostInitProperties()
{
    Super::PostInitProperties();

    UE_LOG(LogTemp, Warning, TEXT("PostInitProperties : %s"), *this->NodeClass->GetName());
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
