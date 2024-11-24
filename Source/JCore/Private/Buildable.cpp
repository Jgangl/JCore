
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

    this->StaticMeshComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2, ECR_Block);
}

void ABuildable::BeginPlay()
{
    Super::BeginPlay();

    this->GetComponents(this->MeshComponents);

    for (UMeshComponent* MeshComponent : MeshComponents)
    {
        if (MeshComponent)
        {
            this->OriginalMaterials.Add(MeshComponent->GetMaterial(0));

            MeshComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2,
                                                         ECollisionResponse::ECR_Block);
        }
    }
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

void ABuildable::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    GetComponents<UPipeConnectionComponent>(this->PipeConnectionComponents, true);

    for (UPipeConnectionComponent* PipeConnectionComponent : this->PipeConnectionComponents)
    {
        if (!PipeConnectionComponent)
        {
            UE_LOG(LogBuildable, Error, TEXT("OnConstruction: PipeConnectionComponent is null"))
            return;
        }

        //PipeConnectionComponent->OnConnected.AddDynamic(this, &ABuildable::OnConnectionConnected);
        //PipeConnectionComponent->OnDisconnected.AddDynamic(this, &ABuildable::OnConnectionDisconnected);
    }
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

        MeshComponent->SetMaterial(0, NewMaterial);
    }
}

void ABuildable::ResetMaterial()
{
    for (int i = 0; i < this->MeshComponents.Num(); i++)
    {
        UMeshComponent* MeshComponent = this->MeshComponents[i];
        UMaterialInterface* OriginalMaterial = OriginalMaterials[i];

        MeshComponent->SetMaterial(0, OriginalMaterial);
    }
}

const FVector& ABuildable::GetBuildingOffset() const
{
    return this->BuildingOffset;
}

void ABuildable::CompleteBuilding()
{
    TArray<UNodeBase*> OutNeighborNodes;

    for (UPipeConnectionComponent* PipeConnectionComponent : this->PipeConnectionComponents)
    {
        if (!PipeConnectionComponent) continue;

        PipeConnectionComponent->UpdateConnections(OutNeighborNodes);
    }

    UGraphNodeComponent* GraphNodeComponent = Cast<UGraphNodeComponent>(this->GetComponentByClass(UGraphNodeComponent::StaticClass()));

    if (!GraphNodeComponent)
    {
        return;
    }

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

void ABuildable::GetPipeSnapLocations(TArray<FVector>& OutSnapLocations) const
{
    for (UPipeConnectionComponent* PipeConnectionComponent : this->PipeConnectionComponents)
    {
        if (!PipeConnectionComponent) continue;

        if (!PipeConnectionComponent->IsConnected())
        {
            OutSnapLocations.Add(PipeConnectionComponent->GetPipeSnapLocation());
        }
    }
}
