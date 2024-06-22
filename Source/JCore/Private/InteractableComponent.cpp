// Fill out your copyright notice in the Description page of Project Settings.

#include "InteractableComponent.h"

UInteractableComponent::UInteractableComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    this->bCanInteract = true;

    this->SetIsReplicatedByDefault(true);
}

void UInteractableComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void UInteractableComponent::BeginPlay()
{
    Super::BeginPlay();
}

bool UInteractableComponent::CanInteract()
{
    return this->bCanInteract;
}

void UInteractableComponent::BeginFocus()
{
    BeginLookAt();
}

void UInteractableComponent::EndFocus()
{
    EndLookAt();
}

void UInteractableComponent::Interact(APawn* Interactor)
{
    this->OnInteract.Broadcast(Interactor);
}

void UInteractableComponent::BeginLookAt()
{
}

void UInteractableComponent::EndLookAt()
{
}

void UInteractableComponent::SetCanInteract(bool CanInteract)
{
    this->bCanInteract = CanInteract;
}
