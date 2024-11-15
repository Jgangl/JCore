// Fill out your copyright notice in the Description page of Project Settings.


#include "BuildingPreview.h"

#include "Engine/SCS_Node.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"

ABuildingPreview::ABuildingPreview()
{
    this->RootSceneComponent = this->CreateDefaultSubobject<USceneComponent>(TEXT("Root Scene Component"));
    this->SetRootComponent(this->RootSceneComponent);

    this->SetReplicatingMovement(true);

    this->PrimaryActorTick.bCanEverTick = true;
    this->bReplicates = true;

    this->bPlacementValid = false;
}

void ABuildingPreview::BeginPlay()
{
    Super::BeginPlay();
}

void ABuildingPreview::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ABuildingPreview, StaticMeshComponents);
    DOREPLIFETIME(ABuildingPreview, bPlacementValid);
    DOREPLIFETIME_CONDITION(ABuildingPreview, ValidPreviewMaterial, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(ABuildingPreview, InvalidPreviewMaterial, COND_InitialOnly);
}

void ABuildingPreview::UpdatePlacementValid()
{
    TArray<UPrimitiveComponent*> OverlappingComponents;
    this->GetOverlappingComponents(OverlappingComponents);

    bool LocalPlacementValid = true;

    for (UPrimitiveComponent* OverlappingComponent : OverlappingComponents)
    {
        if (OverlappingComponent->IsA(UStaticMeshComponent::StaticClass()))
        {
            LocalPlacementValid = false;
            break;
        }

        if (OverlappingComponent->IsA(UCapsuleComponent::StaticClass()) &&
            OverlappingComponent->GetOwner()->IsA(APawn::StaticClass()))
        {
            LocalPlacementValid = false;
            break;
        }
    }

    if (this->bPlacementValid != LocalPlacementValid)
    {
        this->bPlacementValid = LocalPlacementValid;
        this->OnRep_bPlacementValid();
    }
}

void ABuildingPreview::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!this->HasAuthority())
    {
        return;
    }

    this->UpdatePlacementValid();
}

void ABuildingPreview::AddStaticMesh(UStaticMeshComponent* ReferenceMeshComponent)
{
    if (!ReferenceMeshComponent)
    {
        UE_LOG(LogBuildingPreview, Error, TEXT("ReferenceMeshComponent null"))
        return;
    }

    UActorComponent* NewActorComponent = this->AddComponentByClass(UStaticMeshComponent::StaticClass(), false,
                                                                   FTransform::Identity, false);
    UStaticMeshComponent* NewStaticMeshComponent = Cast<UStaticMeshComponent>(NewActorComponent);

    if (!NewStaticMeshComponent)
    {
        return;
    }

    NewStaticMeshComponent->SetStaticMesh(ReferenceMeshComponent->GetStaticMesh());
    NewStaticMeshComponent->SetupAttachment(this->RootSceneComponent);
    NewStaticMeshComponent->SetIsReplicated(true);

    NewStaticMeshComponent->SetRelativeTransform(ReferenceMeshComponent->GetRelativeTransform());

    this->StaticMeshComponents.AddUnique(NewStaticMeshComponent);
    this->OnRep_StaticMeshComponents();
}

void ABuildingPreview::OnRep_bPlacementValid()
{
    UMaterialInterface* Material = this->bPlacementValid ? this->ValidPreviewMaterial : this->InvalidPreviewMaterial;

    this->SetMaterial(Material);
}

void ABuildingPreview::OnRep_StaticMeshComponents()
{
    for (UStaticMeshComponent* StaticMeshComponent : this->StaticMeshComponents)
    {
        if (!StaticMeshComponent) continue;

        StaticMeshComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
        StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    }

    // Make sure material is initialized
    this->OnRep_bPlacementValid();
}

void ABuildingPreview::UpdateMesh(TSubclassOf<AActor> BuildingClass)
{
    TArray<UMeshComponent*> CurrentMeshComponents;

    this->GetComponents(UMeshComponent::StaticClass(), CurrentMeshComponents);

    // Remove current mesh components if there are any
    for (UMeshComponent* MeshComponent : CurrentMeshComponents)
    {
        if (!MeshComponent)
        {
            continue;
        }

        UE_LOG(LogTemp, Warning, TEXT("Remove mesh component"));

        MeshComponent->DestroyComponent();
    }

    this->StaticMeshComponents.Empty();

    UE_LOG(LogTemp, Warning, TEXT("Adding meshes from : %s"), *BuildingClass->GetName());

    TArray<UMeshComponent*> MeshComponents = this->GetMeshComponents(BuildingClass);

    // Add new mesh components
    for (UMeshComponent* MeshComponent : MeshComponents)
    {
        if (!MeshComponent)
        {
            continue;
        }

        if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(MeshComponent))
        {
            this->AddStaticMesh(StaticMeshComponent);
        }
    }
}

void ABuildingPreview::SetMaterial(UMaterialInterface* NewMaterial)
{
    if (!NewMaterial)
    {
        UE_LOG(LogBuildingPreview, Error, TEXT("NewMaterial null"))
        return;
    }

    for (UStaticMeshComponent* StaticMeshComponent : this->StaticMeshComponents)
    {
        if (!StaticMeshComponent) continue;

        StaticMeshComponent->SetMaterial(0, NewMaterial);
    }
}

void ABuildingPreview::SetMaterials(UStaticMeshComponent* StaticMeshComponent, TArray<UMaterialInterface*> NewMaterials)
{
    if (!StaticMeshComponent)
    {
        return;
    }

    for (int i = 0; i < NewMaterials.Num(); i++)
    {
        if (!NewMaterials[i]) continue;

        StaticMeshComponent->SetMaterial(i, NewMaterials[i]);
    }
}

void ABuildingPreview::SetValidPreviewMaterial(UMaterialInterface* NewValidPreviewMaterial)
{
    this->ValidPreviewMaterial = NewValidPreviewMaterial;
}

void ABuildingPreview::SetInvalidPreviewMaterial(UMaterialInterface* NewInvalidPreviewMaterial)
{
    this->InvalidPreviewMaterial = NewInvalidPreviewMaterial;
}

// THIS SHOULD BE MOVED TO AN EXTERNAL FILE
TArray<UMeshComponent*> ABuildingPreview::GetMeshComponents(TSubclassOf<AActor> TargetActorClass)
{
    TArray<UMeshComponent*> MeshComponents;

    if (!IsValid(TargetActorClass))
    {
        return MeshComponents;
    }

    // Check CDO.
    AActor* ActorCDO = TargetActorClass->GetDefaultObject<AActor>();

    TArray<UObject*> DefaultObjectSubobjects;
    ActorCDO->GetDefaultSubobjects(DefaultObjectSubobjects);

    // Search for ActorComponents created from C++
    for (UObject* DefaultSubObject : DefaultObjectSubobjects)
    {
        if (DefaultSubObject->IsA(UStaticMeshComponent::StaticClass()))
        {
            MeshComponents.AddUnique(Cast<UMeshComponent>(DefaultSubObject));
        }
    }

    // Check blueprint nodes. Components added in blueprint editor only (and not in code) are not available from
    // CDO.
    UBlueprintGeneratedClass* RootBlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(TargetActorClass);
    UClass* ActorClass = TargetActorClass;

    // Go down the inheritance tree to find nodes that were added to parent blueprints of our blueprint graph.
    do
    {
        UBlueprintGeneratedClass* ActorBlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(ActorClass);
        if (!ActorBlueprintGeneratedClass)
        {
            return MeshComponents;
        }

        const TArray<USCS_Node*>& ActorBlueprintNodes =
            ActorBlueprintGeneratedClass->SimpleConstructionScript->GetAllNodes();

        for (USCS_Node* Node : ActorBlueprintNodes)
        {
            //UE_LOG(LogBuildingComponent, Error, TEXT("  %s", *Node->GetActualComponentTemplate(ActorBlueprintGeneratedClass)->GetName())
            if (Node->ComponentClass->IsChildOf(UMeshComponent::StaticClass()))
            {
                MeshComponents.AddUnique(
                    Cast<UMeshComponent>(Node->GetActualComponentTemplate(RootBlueprintGeneratedClass)));
            }
        }

        ActorClass = Cast<UClass>(ActorClass->GetSuperStruct());
    }
    while (ActorClass != AActor::StaticClass());

    return MeshComponents;
}
