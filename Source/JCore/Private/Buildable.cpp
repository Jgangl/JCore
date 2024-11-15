
#include "Buildable.h"

ABuildable::ABuildable()
{
    PrimaryActorTick.bCanEverTick = true;

    this->bReplicates = true;
}

void ABuildable::BeginPlay()
{
    Super::BeginPlay();

    this->GetComponents(this->StaticMeshComponents);

    for (UStaticMeshComponent* StaticMeshComponent : StaticMeshComponents)
    {
        if (StaticMeshComponent)
        {
            OriginalMaterials.Add(StaticMeshComponent->GetMaterial(0));

            StaticMeshComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2,
                                                               ECollisionResponse::ECR_Block);
        }
    }
}

void ABuildable::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ABuildable::SetMaterial(UMaterialInterface* NewMaterial)
{
    if (!NewMaterial)
    {
        UE_LOG(LogTemp, Error, TEXT("NewMaterial null"))
        return;
    }

    for (UStaticMeshComponent* StaticMeshComponent : this->StaticMeshComponents)
    {
        if (!StaticMeshComponent) continue;

        StaticMeshComponent->SetMaterial(0, NewMaterial);
    }
}

void ABuildable::ResetMaterial()
{
    for (int i = 0; i < StaticMeshComponents.Num(); i++)
    {
        UStaticMeshComponent* StaticMeshComponent = StaticMeshComponents[i];
        UMaterialInterface* OriginalMaterial = OriginalMaterials[i];

        StaticMeshComponent->SetMaterial(0, OriginalMaterial);
    }
}