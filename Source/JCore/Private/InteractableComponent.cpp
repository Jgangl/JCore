// Fill out your copyright notice in the Description page of Project Settings.

#include "InteractableComponent.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"

UInteractableComponent::UInteractableComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

    this->bCanInteract = true;
    this->ZOffset      = 50.0f;
    this->DisplayText  = TEXT("Interact");

    // Don't scale relative to parent
    this->SetAbsolute(false, false, true);

    this->Space = EWidgetSpace::Screen;
    this->SetDrawAtDesiredSize(true);
    this->SetCastShadow(false);
    this->SetVisibility(true);
    this->SetHiddenInGame(true);

    this->SetIsReplicatedByDefault(true);
}

void UInteractableComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION(UInteractableComponent, InteractSound, COND_InitialOnly);
}

void UInteractableComponent::BeginPlay()
{
    Super::BeginPlay();

    // Set interaction text
    if (!IsRunningDedicatedServer())
    {
        UUserWidget* UserWidget = this->GetWidget();

        if (!UserWidget)
        {
            UE_LOG(LogTemp, Error, TEXT("Widget is null"));
            return;
        }

        UWidget* DisplayWidget = UserWidget->GetWidgetFromName(TEXT("DisplayText"));

        if (DisplayWidget)
        {
            UTextBlock* DisplayTextBlock = Cast<UTextBlock>(DisplayWidget);

            if (DisplayTextBlock)
            {
                DisplayTextBlock->SetText(FText::FromString(GetDisplayText()));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("DisplayWidget is null"));
        }
    }
}

void UInteractableComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                           FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    UWorld* World = GetWorld();

    if (!World)
    {
        return;
    }

    APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(World, 0);

    if (!CameraManager)
    {
        return;
    }

    const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(
        GetComponentLocation(), CameraManager->GetCameraLocation());

    this->SetWorldRotation(LookAtRotation);

    FBoxSphereBounds MeshBounds = GetStaticMeshComponentsBounds();

    this->SetWorldLocation(MeshBounds.Origin + FVector(0.0f, 0.0f, MeshBounds.BoxExtent.Z + this->ZOffset));
}

void UInteractableComponent::OnComponentCreated()
{
    Super::OnComponentCreated();

    AActor* Owner = this->GetOwner();

    if (!Owner)
    {
        return;
    }

    Owner->GetComponents(this->MeshComponents);

    for (UMeshComponent* MeshComponent : this->MeshComponents)
    {
        if (MeshComponent)
        {
            //MeshComponent->SetRenderCustomDepth(true);
        }
    }
}

void UInteractableComponent::OnRegister()
{
    Super::OnRegister();
}

FBoxSphereBounds UInteractableComponent::GetStaticMeshComponentsBounds()
{
    AActor* Owner = this->GetOwner();

    if (!Owner)
    {
        return FBoxSphereBounds();
    }

    FBoxSphereBounds MeshComponentsTotalBounds;

    FVector Origin;

    for (UMeshComponent* MeshComponent : this->MeshComponents)
    {
        if (MeshComponent == this)
        {
            continue;
        }

        Origin = MeshComponent->Bounds.Origin;

        FVector BoxExtent = MeshComponent->Bounds.BoxExtent;

        if (BoxExtent.X > MeshComponentsTotalBounds.BoxExtent.X)
        {
            MeshComponentsTotalBounds.BoxExtent.X = BoxExtent.X;
        }

        if (BoxExtent.Y > MeshComponentsTotalBounds.BoxExtent.Y)
        {
            MeshComponentsTotalBounds.BoxExtent.Y = BoxExtent.Y;
        }

        if (BoxExtent.Z > MeshComponentsTotalBounds.BoxExtent.Z)
        {
            MeshComponentsTotalBounds.BoxExtent.Z = BoxExtent.Z;
        }
    }

    MeshComponentsTotalBounds.Origin = Origin;

    return MeshComponentsTotalBounds;
}

bool UInteractableComponent::CanInteract()
{
    return this->bCanInteract;
}

void UInteractableComponent::BeginFocus()
{
    this->BeginLookAt();
}

void UInteractableComponent::EndFocus()
{
    this->EndLookAt();
}

void UInteractableComponent::Interact(APawn* Interactor)
{
    this->OnInteract.Broadcast(Interactor);
}

void UInteractableComponent::SetDisplayText(const FString& InText)
{
    this->DisplayText = InText;
}

void UInteractableComponent::SetInteractSound(USoundBase* InInteractSound)
{
    this->InteractSound = InInteractSound;
}

const FString& UInteractableComponent::GetDisplayText()
{
    return this->DisplayText;
}

USoundBase* UInteractableComponent::GetInteractSound()
{
    return this->InteractSound;
}

void UInteractableComponent::BeginLookAt()
{
    for (UMeshComponent* MeshComponent : this->MeshComponents)
    {
        if (MeshComponent)
        {
            //MeshComponent->SetCustomDepthStencilValue(2);
        }
    }

    this->SetHiddenInGame(false);
}

void UInteractableComponent::EndLookAt()
{
    for (UMeshComponent* MeshComponent : this->MeshComponents)
    {
        if (MeshComponent)
        {
            //MeshComponent->SetCustomDepthStencilValue(0);
        }
    }

    this->SetHiddenInGame(true);
}

void UInteractableComponent::SetCanInteract(bool CanInteract)
{
    this->bCanInteract = CanInteract;
}
