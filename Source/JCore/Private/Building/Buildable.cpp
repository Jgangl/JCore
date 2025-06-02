// Copyright Joshua Gangl. All Rights Reserved.

#include "Building/Buildable.h"

#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Components/InstancedStaticMeshComponent.h"

#include "JCoreUtils.h"
#include "Components/SphereComponent.h"
#include "Graph/GraphNodeComponent.h"
#include "Graph/GraphSubsystem.h"

ABuildable::ABuildable()
{
    PrimaryActorTick.bCanEverTick = true;
    this->bReplicates = true;

    this->StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Static Mesh Component"));
    this->SetRootComponent(this->StaticMeshComponent);

    this->StaticMeshComponent->SetMobility(EComponentMobility::Static);

    this->bIsPreviewing        = false;
    this->bValidPlacement      = false;
    this->bRequireOverlapCheck = true;

    this->SnapType = EBuildingSnapType::Floor;

    this->bDebug = false;

    this->SnapTransforms.Add(EBuildingSnapType::Floor);
    this->SnapTransforms.Add(EBuildingSnapType::Wall);

    this->StaticMeshComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2, ECR_Overlap);

    const FString ValidMaterialPath = TEXT("/Script/Engine.Material'/Game/Assets/Materials/M_ValidBuildingPreview.M_ValidBuildingPreview'");
    const FString InvalidMaterialPath = TEXT("/Script/Engine.Material'/Game/Assets/Materials/M_InvalidBuildingPreview.M_InvalidBuildingPreview'");

    static ConstructorHelpers::FObjectFinder<UMaterial> ValidMaterialFinder(*ValidMaterialPath);

    if (!ValidMaterialFinder.Succeeded())
    {
        UE_LOG(LogTemp, Error, TEXT("%hs : Path invalid: %s"), __FUNCTION__, *ValidMaterialPath);
    }

    this->ValidPreviewMaterial = ValidMaterialFinder.Object;

    static ConstructorHelpers::FObjectFinder<UMaterial> InvalidMaterialFinder(*InvalidMaterialPath);

    if (!InvalidMaterialFinder.Succeeded())
    {
        UE_LOG(LogTemp, Error, TEXT("%hs : Path invalid: %s"), __FUNCTION__, *InvalidMaterialPath);
    }

    this->InvalidPreviewMaterial = InvalidMaterialFinder.Object;
}

void ABuildable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ABuildable, bIsPreviewing);
    DOREPLIFETIME_CONDITION(ABuildable, ValidPreviewMaterial, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(ABuildable, InvalidPreviewMaterial, COND_InitialOnly);
}

void ABuildable::BeginPlay()
{
    Super::BeginPlay();
}

void ABuildable::Destroyed()
{
    for (UBuildingConnectionComponent* BuildingConnectionComponent : this->BuildingConnectionComponents)
    {
        if (!BuildingConnectionComponent) continue;

        BuildingConnectionComponent->DisconnectConnections();
    }

    this->RemoveGraphNode();

    Super::Destroyed();
}

bool ABuildable::RemoveGraphNode()
{
    UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(GetWorld());

    if (!GameInstance)
    {
        return false;
    }

    UGraphSubsystem* GraphSubsystem = GameInstance->GetSubsystem<UGraphSubsystem>();

    if (!GraphSubsystem)
    {
        return false;
    }

    UGraphBase* Graph = GraphSubsystem->GetGraph();

    if (!Graph)
    {
        return false;
    }

    UGraphNodeComponent* GraphNodeComponent = Cast<UGraphNodeComponent>(this->GetComponentByClass(UGraphNodeComponent::StaticClass()));

    if (!GraphNodeComponent)
    {
        return false;
    }

    Graph->RemoveNode(GraphNodeComponent->GetNode());

    return true;
}

void ABuildable::UpdatePreviewing()
{
    if (this->bIsPreviewing)
    {
        this->SetCollisionProfileName(FName(TEXT("BuildablePreview")));
        this->SetMaterial(this->ValidPreviewMaterial);
    }
    else
    {
        this->SetCollisionProfileName(FName(TEXT("Buildable")));
        this->ResetMaterial();
    }
}

void ABuildable::OnConnectionConnected(UBuildingConnectionComponent* FromConnectedConnection,
                                       UBuildingConnectionComponent* ToConnectedConnection)
{
    if (!FromConnectedConnection)
    {
        UE_LOG(LogBuildable, Error, TEXT("OnConnectionConnected: FromConnectedConnection is nullptr"));
        return;
    }

    if (!ToConnectedConnection)
    {
        UE_LOG(LogBuildable, Error, TEXT("OnConnectionConnected: ToConnectedConnection is nullptr"));
        return;
    }

    UE_LOG(LogBuildable, Verbose, TEXT("OnConnectionConnected: %s was connected to %s"), *FromConnectedConnection->GetName(), *ToConnectedConnection->GetName());
}

void ABuildable::OnConnectionDisconnected(UBuildingConnectionComponent* DisconnectedConnection)
{
    if (!DisconnectedConnection)
    {
        UE_LOG(LogBuildable, Error, TEXT("OnConnectionDisconnected: DisconnectedConnection is nullptr"));
        return;
    }

    UE_LOG(LogBuildable, Verbose, TEXT("OnConnectionDisconnected: %s was disconnected"), *DisconnectedConnection->GetName());
}

void ABuildable::OnRep_IsPreviewing()
{
    this->UpdatePreviewing();
}

void ABuildable::UpdatePlacementValidity()
{
    if (this->GetSize() == FVector::Zero())
    {
        //UE_LOG(LogBuildable, Warning, TEXT("UpdatePlacementValidity: this->Size is FVector::Zero and placement validity will not work"));
    }

    // TODO: Put this in ABuildable as a property?
    FVector OverlapCushion = this->GetSize() * 0.25f;
    FVector OverlapCheckHalfExtents = (this->GetSize() - OverlapCushion) / 2.0f;

    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldStatic));

    const TArray<AActor*> ActorsToIgnore{this};

    // Walls origin is at the ground, we need to offset the box overlap check to compensate for this
    //  Is there a better way to do this?
    FVector OverlapCheckLocation = this->GetActorLocation() + this->GetOriginOffset();

    TArray<AActor*> OverlappingActors;
    float bIsOverlappingActors = UKismetSystemLibrary::BoxOverlapActors(GetWorld(),
                                                                        OverlapCheckLocation,
                                                                        OverlapCheckHalfExtents,
                                                                        ObjectTypes,
                                                                        ABuildable::StaticClass(),
                                                                        ActorsToIgnore,
                                                                        OverlappingActors);

    this->bValidPlacement = !bIsOverlappingActors;
    this->OnRep_bPlacementValid();
}

void ABuildable::OnRep_bPlacementValid()
{
    if (this->bValidPlacement)
    {
        this->SetMaterial(this->ValidPreviewMaterial);
    }
    else
    {
        this->SetMaterialInvalid();
    }
}

void ABuildable::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    GetComponents<UBuildingConnectionComponent>(this->BuildingConnectionComponents, true);
    for (UBuildingConnectionComponent* BuildingConnectionComponent : this->BuildingConnectionComponents)
    {
        if (!BuildingConnectionComponent)
        {
            UE_LOG(LogBuildable, Error, TEXT("OnConstruction: BuildingConnectionComponent is null"))
            return;
        }

        BuildingConnectionComponent->OnConnectionConnected.RemoveAll(this);
        BuildingConnectionComponent->OnConnectionDisconnected.RemoveAll(this);

        BuildingConnectionComponent->OnConnectionConnected.AddUniqueDynamic(this, &ABuildable::OnConnectionConnected);
        BuildingConnectionComponent->OnConnectionDisconnected.AddUniqueDynamic(this, &ABuildable::OnConnectionDisconnected);
    }

    GetComponents(this->MeshComponents);
    for (UMeshComponent* MeshComponent : this->MeshComponents)
    {
        if (MeshComponent && !MeshComponent->IsA(UInstancedStaticMeshComponent::StaticClass()))
        {
            this->OriginalMaterials.Add(MeshComponent->GetMaterial(0));

            MeshComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2,
                                                         ECollisionResponse::ECR_Overlap);

            MeshComponent->SetRenderCustomDepth(true);
        }
    }
}

void ABuildable::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!this->HasAuthority())
    {
        return;
    }

    // Only update placement validity on server
    if (this->bIsPreviewing && this->RequiresOverlapCheck())
    {
        this->UpdatePlacementValidity();
    }

    if (this->bDebug)
    {
        FBox BoundingBox = this->GetComponentsBoundingBox(false);
        DrawDebugBox(GetWorld(), this->GetActorLocation(), BoundingBox.GetExtent(), FColor::Blue, false);
    }
}

void ABuildable::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    GetComponents<UBuildingConnectionComponent>(this->BuildingConnectionComponents, true);
}

FString ABuildable::GetDisplayName() const
{
    return this->DisplayName;
}

UStaticMeshComponent* ABuildable::GetStaticMeshComponent() const
{
    return this->StaticMeshComponent;
}

const FVector& ABuildable::GetSize() const
{
    return this->Size;
}

const FVector& ABuildable::GetOriginOffset() const
{
    return this->OriginOffset;
}

void ABuildable::SetMaterial(UMaterialInterface* NewMaterial)
{
    if (!NewMaterial)
    {
        UE_LOG(LogBuildable, Error, TEXT("NewMaterial null"))
        return;
    }

    for (UMeshComponent* MeshComponent : this->MeshComponents)
    {
        if (!MeshComponent) continue;

        MeshComponent->SetOverlayMaterial(NewMaterial);

        //MeshComponent->SetMaterial(0, NewMaterial);
    }
}

void ABuildable::SetMaterialInvalid()
{
    this->SetMaterial(this->InvalidPreviewMaterial);

    for (UMeshComponent* MeshComponent : this->MeshComponents)
    {
        if (MeshComponent)
        {
            MeshComponent->SetCustomDepthStencilValue(2);
        }
    }
}

void ABuildable::ResetMaterial()
{
    for (int i = 0; i < this->MeshComponents.Num(); i++)
    {
        UMeshComponent* MeshComponent = this->MeshComponents[i];
        UMaterialInterface* OriginalMaterial = this->OriginalMaterials[i];

        if (!MeshComponent) continue;

        MeshComponent->SetMaterial(0, OriginalMaterial);
        MeshComponent->SetOverlayMaterial(nullptr);

        MeshComponent->SetCustomDepthStencilValue(0);
    }
}

void ABuildable::MulticastSetMaterialInvalid_Implementation()
{
    this->SetMaterialInvalid();
}

void ABuildable::MulticastResetMaterial_Implementation()
{
    this->ResetMaterial();
}

bool ABuildable::RequiresOverlapCheck()
{
    return this->bRequireOverlapCheck;
}

const FVector& ABuildable::GetBuildingOffset() const
{
    return this->BuildingOffset;
}

void ABuildable::CompleteBuilding(UBuildingConnectionComponent* FromSnapConnection, UBuildingConnectionComponent* ToSnapConnection)
{
    this->SetIsPreviewing(false);
/*
    if (!FromSnapConnection || !ToSnapConnection)
    {
        return;
    }

    AActor* SnapToOwner = ToSnapConnection->GetOwner();
    if (!SnapToOwner)
    {
        UE_LOG(LogTemp, Error, TEXT("%hs : OtherOwner is nullptr"), __FUNCTION__);
        return;
    }

    UGraphNodeComponent* SnapToNode = SnapToOwner->GetComponentByClass<UGraphNodeComponent>();

    if (!SnapToNode)
    {
        UE_LOG(LogTemp, Warning, TEXT("Couldn't find a graph node component on the snapped connection"));
        return;
    }

    FromSnapConnection->SetConnectedComponent(ToSnapConnection);
    ToSnapConnection->SetConnectedComponent(FromSnapConnection);

    UNodeBase* InputNode = nullptr;
    UNodeBase* OutputNode = nullptr;

    if (ToSnapConnection->IsInput())
    {
        InputNode = GraphNodeComponent->GetNode();
        OutputNode = SnapToNode->GetNode();
    }
    else
    {
        InputNode = SnapToNode->GetNode();
        OutputNode = GraphNodeComponent->GetNode();
    }

    Graph->AddEdge(InputNode, OutputNode);
    */
}

void ABuildable::GetOpenConnectionComponents(TArray<UBuildingConnectionComponent*>& OutConnectionComponents) const
{
    for (UBuildingConnectionComponent* BuildingConnectionComponent : this->BuildingConnectionComponents)
    {
        if (!BuildingConnectionComponent) continue;

        if (!BuildingConnectionComponent->IsConnected())
        {
            OutConnectionComponents.Add(BuildingConnectionComponent);
        }
    }
}

void ABuildable::AddConnection(UBuildingConnectionComponent* FromConnection, UBuildingConnectionComponent* ToConnection)
{
}

void ABuildable::GetConnectionSnapTransforms(TArray<FTransform>& OutSnapTransforms) const
{
    for (UBuildingConnectionComponent* BuildingConnectionComponent : this->BuildingConnectionComponents)
    {
        if (!BuildingConnectionComponent) continue;

        if (!BuildingConnectionComponent->IsConnected())
        {
            OutSnapTransforms.Add(BuildingConnectionComponent->GetSnapTransform());
        }
    }
}

UBuildingConnectionComponent* ABuildable::GetClosestConnectionToLocation(const FVector& InLocation) const
{
    TArray<UBuildingConnectionComponent*> OpenConnections;
    this->GetOpenConnectionComponents(OpenConnections);

    TArray<USceneComponent*> ConnectionComponents(MoveTemp(OpenConnections));

    return Cast<UBuildingConnectionComponent>(UJCoreUtils::GetClosestSceneComponentToPoint(InLocation, ConnectionComponents));
}

UBuildingConnectionComponent* ABuildable::GetOppositeTypeConnection(bool bInput) const
{
    TArray<UBuildingConnectionComponent*> OpenConnections;
    this->GetOpenConnectionComponents(OpenConnections);

    for (UBuildingConnectionComponent* BuildingConnectionComponent : OpenConnections)
    {
        if (bInput != BuildingConnectionComponent->IsInput())
        {
            return BuildingConnectionComponent;
        }
    }

    // No connection components found of the opposite type
    return nullptr;
}

bool ABuildable::HasOpenConnections() const
{
    TArray<FTransform> OpenConnections;
    this->GetConnectionSnapTransforms(OpenConnections);

    return !OpenConnections.IsEmpty();
}

void ABuildable::GetNeighborSnapLocations(TArray<FVector>& OutSnapLocations)
{

}

void ABuildable::SetIsPreviewing(bool InIsPreviewing)
{
    this->bIsPreviewing = InIsPreviewing;

    this->UpdatePreviewing();
}

void ABuildable::SetCollisionProfileName(const FName InCollisionProfileName)
{
    for (UMeshComponent* MeshComponent : this->MeshComponents)
    {
        if (!MeshComponent || MeshComponent->IsA(UInstancedStaticMeshComponent::StaticClass())) continue;

        MeshComponent->SetCollisionProfileName(InCollisionProfileName);
        // This is necessary for Instanced static mesh components to properly generate their collision
        // after building for some reason
        MeshComponent->RecreatePhysicsState();
    }
}

bool ABuildable::IsPlacementValid() const
{
    return this->bValidPlacement;
}

EBuildingSnapType ABuildable::GetSnapType() const
{
    return this->SnapType;
}

void ABuildable::SetSnapTransformsOfType(EBuildingSnapType InSnapType, const TArray<FTransform>& InSnapTransforms)
{
    if (!this->SnapTransforms.Contains(InSnapType))
    {
        return;
    }

    this->SnapTransforms[InSnapType] = InSnapTransforms;
}

void ABuildable::GetSnapTransformsOfType(EBuildingSnapType InSnapType, TArray<FTransform>& OutSnapTransforms) const
{
    if (!this->SnapTransforms.Contains(InSnapType))
    {
        return;
    }

    TArray<FTransform> WorldSnapTransforms;
    for (const FTransform LocalSnapTransform : this->SnapTransforms[InSnapType])
    {
        const FQuat WorldQuat  = this->GetTransform().TransformRotation(LocalSnapTransform.GetRotation());
        const FVector WorldLoc = this->GetTransform().TransformPosition(LocalSnapTransform.GetLocation());

        FTransform WorldSnapTransform = LocalSnapTransform;
        WorldSnapTransform.SetLocation(WorldLoc);
        WorldSnapTransform.SetRotation(WorldQuat);

        WorldSnapTransforms.Add(WorldSnapTransform);
    }

    OutSnapTransforms = WorldSnapTransforms;
}

void ABuildable::OnActorLoaded_Implementation()
{
    this->UpdatePreviewing();
}
