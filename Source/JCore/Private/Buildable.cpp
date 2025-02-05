
#include "Buildable.h"

#include "Graph/GraphNodeComponent.h"
#include "Graph/GraphSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "SteamFactory/PipeConnectionComponent.h"

ABuildable::ABuildable()
{
    PrimaryActorTick.bCanEverTick = true;
    this->bReplicates = true;

    this->StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Static Mesh Component"));
    this->SetRootComponent(this->StaticMeshComponent);

    this->bIsPreviewing        = false;
    this->bValidPlacement      = false;
    this->bRequireOverlapCheck = true;

    this->StaticMeshComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2, ECR_Block);

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
    for (UPipeConnectionComponent* PipeConnectionComponent : this->PipeConnectionComponents)
    {
        if (!PipeConnectionComponent) continue;

        PipeConnectionComponent->DisconnectConnections();
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

void ABuildable::OnRep_IsPreviewing()
{
    this->UpdatePreviewing();
}

void ABuildable::UpdatePlacementValidity()
{
    // Update material
    // We should not be able to build if we are overlapping other buildables or a character

    bool bLocalValidPlacement = true;

    TArray<AActor*> OverlapplingActors;
    GetOverlappingActors(OverlapplingActors);

    for (AActor* OverlappingActor : OverlapplingActors)
    {
        if (OverlappingActor->IsA(APawn::StaticClass()) ||
            OverlappingActor->IsA(ABuildable::StaticClass()))
        {
            bLocalValidPlacement = false;
        }
    }

    this->bValidPlacement = bLocalValidPlacement;
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

    GetComponents(this->MeshComponents);

    for (UMeshComponent* MeshComponent : this->MeshComponents)
    {
        if (MeshComponent)
        {
            this->OriginalMaterials.Add(MeshComponent->GetMaterial(0));

            MeshComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2,
                                                         ECollisionResponse::ECR_Block);
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
}

void ABuildable::ResetMaterial()
{
    for (int i = 0; i < this->MeshComponents.Num(); i++)
    {
        UMeshComponent* MeshComponent = this->MeshComponents[i];
        UMaterialInterface* OriginalMaterial = this->OriginalMaterials[i];

        MeshComponent->SetMaterial(0, OriginalMaterial);

        MeshComponent->SetOverlayMaterial(nullptr);
    }
}

bool ABuildable::RequiresOverlapCheck()
{
    return this->bRequireOverlapCheck;
}

const FVector& ABuildable::GetBuildingOffset() const
{
    return this->BuildingOffset;
}

void ABuildable::CompleteBuilding()
{
    this->SetIsPreviewing(false);

    GetComponents<UPipeConnectionComponent>(this->PipeConnectionComponents, true);

    TArray<UNodeBase*> OutNeighborNodes;

    for (UPipeConnectionComponent* PipeConnectionComponent : this->PipeConnectionComponents)
    {
        if (!PipeConnectionComponent) continue;

        PipeConnectionComponent->UpdateConnections(OutNeighborNodes);
    }


    // TODO: Move this section of code to a function
    UGraphNodeComponent* GraphNodeComponent = Cast<UGraphNodeComponent>(this->GetComponentByClass(UGraphNodeComponent::StaticClass()));

    if (!GraphNodeComponent)
    {
        return;
    }

    GraphNodeComponent->SetNodeLocation(this->GetActorLocation());

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
        Graph->AddNodeWithEdges(GraphNodeComponent->GetNode(), OutNeighborNodes);
    }
}

void ABuildable::GetPipeSnapTransforms(TArray<FTransform>& OutSnapTransforms) const
{
    for (UPipeConnectionComponent* PipeConnectionComponent : this->PipeConnectionComponents)
    {
        if (!PipeConnectionComponent) continue;

        if (!PipeConnectionComponent->IsConnected())
        {
            OutSnapTransforms.Add(PipeConnectionComponent->GetPipeSnapTransform());
        }
    }
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
        if (!MeshComponent) continue;

        MeshComponent->SetCollisionProfileName(InCollisionProfileName);
    }
}

bool ABuildable::IsPlacementValid() const
{
    return this->bValidPlacement;
}
