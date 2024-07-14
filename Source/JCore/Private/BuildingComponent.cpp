// Fill out your copyright notice in the Description page of Project Settings.


#include "BuildingComponent.h"

#include "Buildable.h"
#include "Engine/SCS_Node.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

UBuildingComponent::UBuildingComponent()
{
    this->PrimaryComponentTick.bCanEverTick = true;

    this->RoatationGridSnapValue = 10.0f;
    this->bInDeleteMode          = false;
    this->bInBuildMode           = false;

    this->GridTileSizeX = 100.0f;
    this->GridTileSizeY = 100.0f;
    this->GridTileSizeZ = 100.0f;

    this->GridOffsetX = 50.0f;
    this->GridOffsetY = 50.0f;
    this->GridOffsetZ = 0.0f;

    this->bDebug = true;

    this->SetIsReplicatedByDefault(true);
}

void UBuildingComponent::TickComponent(float DeltaTime,
                                       ELevelTick TickType,
                                       FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (this->CurrentBuildingPreview)
    {
        FVector LerpedLocation = FMath::VInterpTo(this->CurrentBuildingPreview->GetActorLocation(),
                                 this->ServerTargetTransform.GetLocation(),
                                 DeltaTime,
                                 15.0f);

        this->CurrentBuildingPreview->SetActorLocation(LerpedLocation);
        if (!this->CurrentBuildingPreview->GetActorRotation().Equals(this->ServerTargetTransform.GetRotation().Rotator(), 1.0f))
        {
            this->CurrentBuildingPreview->SetActorRotation(this->ServerTargetTransform.GetRotation());
        }
    }


    APawn* OwningPawn = Cast<APawn>(GetOwner());

    if (!OwningPawn || !OwningPawn->IsLocallyControlled())
    {
        return;
    }

    if (!this->bInBuildMode)
    {
        return;
    }

    TArray<FHitResult> OutHits;
    this->GetHitResultsUnderCursor(OutHits);

    if (OutHits.Num() == 0)
    {
        return;
    }

    if (this->bInDeleteMode)
    {
        this->HandleDeleteMode(OutHits);
        return;
    }

    if (this->CurrentBuildingPreview)
    {
        this->HandleBuildingPreview(OutHits);
    }

    this->TargetLocationRepTimer += DeltaTime;
}

void UBuildingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UBuildingComponent, CurrentBuildingPreview);
    DOREPLIFETIME(UBuildingComponent, BuildableHoveringToDelete);
    DOREPLIFETIME(UBuildingComponent, ServerTargetTransform);
    DOREPLIFETIME(UBuildingComponent, bInDeleteMode);
    DOREPLIFETIME(UBuildingComponent, bInBuildMode);
    DOREPLIFETIME_CONDITION(UBuildingComponent, ValidPreviewMaterial, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(UBuildingComponent, InvalidPreviewMaterial, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(UBuildingComponent, TargetLocationRepFrequency, COND_InitialOnly);
}

bool UBuildingComponent::IsTryingToPlaceBuilding() const
{
    return this->CurrentBuildingPreview != nullptr;
}

bool UBuildingComponent::IsInDeleteMode() const
{
    return this->bInDeleteMode;
}

bool UBuildingComponent::IsInBuildMode() const
{
    return this->bInBuildMode;
}

void UBuildingComponent::ServerCancelBuilding_Implementation()
{
    this->CancelBuilding();
}

// THIS SHOULD BE RENAMED TO START PREVIEW BUILD
void UBuildingComponent::ShowBuildGhost_Implementation(TSubclassOf<AActor> ActorClassToPreview)
{
    AActor* ActorToPreview = ActorClassToPreview.GetDefaultObject();

    if (!ActorToPreview)
    {
        UE_LOG(LogBuildingComponent, Error, TEXT("ActorToPreview null"))
        return;
    }

    TArray<UMeshComponent*> MeshComponents = this->GetMeshComponents(ActorClassToPreview);

    AActor* Player = this->GetOwner();

    if (!Player)
    {
        return;
    }

    // Delete previous preview
    if (this->CurrentBuildingPreview)
    {
        this->CurrentBuildingPreview->Destroy();
    }

    const FVector& GridCursorLocation = this->GetClosestGridLocationToCursor();

    const FTransform SpawnTransform(FQuat::Identity, GridCursorLocation, FVector::One());

    this->ServerSetTargetTransform(SpawnTransform);

    this->ClientTargetTransform = SpawnTransform;

    FActorSpawnParameters SpawnParams;
    ABuildingPreview* BuildingPreview =
        GetWorld()->SpawnActor<ABuildingPreview>(ABuildingPreview::StaticClass(), SpawnTransform);

    BuildingPreview->SetValidPreviewMaterial(this->ValidPreviewMaterial);
    BuildingPreview->SetInvalidPreviewMaterial(this->InvalidPreviewMaterial);

    this->CurrentBuildingPreviewClass = ActorClassToPreview;
    this->CurrentBuildingPreview = BuildingPreview;

    if (!this->CurrentBuildingPreview)
    {
        UE_LOG(LogBuildingComponent, Error, TEXT("Spawn Build Ghost failed"))
        return;
    }

    for (UMeshComponent* MeshComponent : MeshComponents)
    {
        if (!MeshComponent)
        {
            continue;
        }

        if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(MeshComponent))
        {
            CurrentBuildingPreview->AddStaticMesh(StaticMeshComponent);
        }
    }
}

void UBuildingComponent::ServerRotateBuildObject_Implementation(bool bClockwise)
{
    if (!this->CurrentBuildingPreview) return;

    const FRotator DeltaRotation = bClockwise
                                       ? FRotator(0.0f, this->RoatationGridSnapValue, 0.0f)
                                       : FRotator(0.0f, -this->RoatationGridSnapValue, 0.0f);

    this->CurrentBuildingPreview->AddActorLocalRotation(DeltaRotation);
}

void UBuildingComponent::RotateBuildObject(bool bClockwise)
{
    const FRotator DeltaRotation = bClockwise
                                       ? FRotator(0.0f, this->RoatationGridSnapValue, 0.0f)
                                       : FRotator(0.0f, -this->RoatationGridSnapValue, 0.0f);

    this->ClientTargetTransform.SetRotation((this->ClientTargetTransform.GetRotation().Rotator() + DeltaRotation).Quaternion());
}

void UBuildingComponent::SetDeleteMode(bool InDeleteMode)
{
    if (this->bInDeleteMode != InDeleteMode)
    {
        this->bInDeleteMode = InDeleteMode;

        if (this->bInDeleteMode)
        {
            this->CancelBuilding();
        }
        else
        {
            if (this->LocalHoveringBuildableActor)
            {
                ABuildable* PreviousHoveringBuildable = Cast<ABuildable>(this->LocalHoveringBuildableActor);
                if (PreviousHoveringBuildable)
                {
                    PreviousHoveringBuildable->ResetMaterial();
                }

                this->ServerSetBuildableHoveringToDelete(nullptr);

                this->LocalHoveringBuildableActor = nullptr;
            }

            this->BuildableHoveringToDelete = nullptr;
        }
    }
}

void UBuildingComponent::SetBuildMode(bool InBuildMode)
{
    if (this->bInBuildMode != InBuildMode)
    {
        this->bInBuildMode = InBuildMode;

        // Exit delete mode when exiting build mode
        if (!this->bInBuildMode)
        {
            this->SetDeleteMode(false);
        }
    }
}

AActor* UBuildingComponent::GetPreviouslyCompletedBuilding()
{
    return this->PreviouslyCompletedBuilding;
}

void UBuildingComponent::ServerSetBuildableHoveringToDelete_Implementation(ABuildable* NewBuildable)
{
    if (this->BuildableHoveringToDelete != NewBuildable)
    {
        this->BuildableHoveringToDelete = NewBuildable;
    }
}

void UBuildingComponent::ServerSetDeleteMode_Implementation(bool InDeleteMode)
{
    this->SetDeleteMode(InDeleteMode);
}

void UBuildingComponent::CancelBuilding()
{
    if (this->CurrentBuildingPreview)
    {
        this->CurrentBuildingPreview->Destroy();
        this->CurrentBuildingPreview = nullptr;

        this->CurrentBuildingPreviewClass = nullptr;
    }
}

bool UBuildingComponent::TryBuild()
{
    if (!this->CurrentBuildingPreview)
    {
        UE_LOG(LogBuildingComponent, Error, TEXT("CurrentBuildingPreview null"))
        return false;
    }

    this->CurrentBuildingPreview->UpdatePlacementValid();

    if (!this->CurrentBuildingPreview->IsPlacementValid())
    {
        UE_LOG(LogBuildingComponent, Error, TEXT("Placement not valid"))
        // TODO: Display error to player
        return false;
    }

    AActor* SpawnedActor = GetWorld()->SpawnActor(this->ActorClassToSpawn,
                                                  &this->ServerTargetTransform);

    // Cleanup preview actor
    if (this->CurrentBuildingPreview)
    {
        this->CurrentBuildingPreview->Destroy();
        this->CurrentBuildingPreview = nullptr;

        this->CurrentBuildingPreviewClass = nullptr;
    }

    this->OnCompletedBuilding.Broadcast(SpawnedActor);

    this->PreviouslyCompletedBuilding = SpawnedActor;

    return true;
}

bool UBuildingComponent::TryDelete()
{
    if (!BuildableHoveringToDelete)
    {
        return false;
    }

    this->BuildableHoveringToDelete->Destroy();
    this->BuildableHoveringToDelete = nullptr;

    return true;
}

// THIS SHOULD BE MOVED TO AN EXTERNAL FILE
TArray<UMeshComponent*> UBuildingComponent::GetMeshComponents(TSubclassOf<AActor> TargetActorClass)
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

void UBuildingComponent::ServerSetTargetTransform_Implementation(const FTransform& TargetTransform)
{
    this->ServerTargetTransform = TargetTransform;
}

void UBuildingComponent::GetHitResultsUnderCursor(TArray<FHitResult>& OutHits) const
{
    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PlayerController)
    {
        return;
    }

    FVector MouseLocation;
    FVector MouseRotation;

    PlayerController->DeprojectMousePositionToWorld(MouseLocation, MouseRotation);

    float Distance = 5000.0f;
    FVector End = MouseLocation + MouseRotation * Distance;

    TArray<AActor*> ActorsToIgnore = {PlayerController->GetPawn(), CurrentBuildingPreview};

    // TODO: Try to only hit landscape or other building objects?

    // TODO: Do a sphere trace instead of line trace to more accurately hit objects
    UKismetSystemLibrary::LineTraceMulti(
        GetWorld(),
        MouseLocation,
        End,
        UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_GameTraceChannel2),
        false,
        ActorsToIgnore,
        EDrawDebugTrace::None,
        OutHits,
        true);
}

const FVector UBuildingComponent::GetClosestGridLocationToCursor() const
{
    TArray<FHitResult> OutHits;
    this->GetHitResultsUnderCursor(OutHits);

    if (OutHits.Num() == 0)
    {
        return FVector::Zero();
    }

    return this->GetGridLocation(OutHits[0].Location);
}

const FVector UBuildingComponent::GetGridLocation(const FVector& InLocation) const
{
    const float GridX = FMath::Floor(InLocation.X / this->GridTileSizeX) * this->GridTileSizeX + this->GridOffsetX;
    const float GridY = FMath::Floor(InLocation.Y / this->GridTileSizeY) * this->GridTileSizeY + this->GridOffsetY;
    const float GridZ = FMath::Floor(InLocation.Z / this->GridTileSizeZ) * this->GridTileSizeZ + this->GridOffsetZ;

    return FVector(GridX, GridY, GridZ);
}

void UBuildingComponent::HandleBuildingPreview(TArray<FHitResult>& OutHits)
{
    FVector HitLocation = OutHits[0].Location;

    if (this->bDebug)
    {
        DrawDebugSphere(GetWorld(), HitLocation, 20.0f, 8, FColor::Blue, false);
    }

    FVector GridLocation = this->GetGridLocation(HitLocation);

    if (this->bDebug)
    {
        DrawDebugSphere(GetWorld(), GridLocation, 20.0f, 8, FColor::Green, false);
    }

    FTransform TargetTransform(this->ClientTargetTransform.GetRotation(), GridLocation, FVector::One());

    ETargetSnapSlot TargetSnapSlot = ETargetSnapSlot::Floor;

    // TODO: FIX this
    FString TargetNameContainsString;
    if (TargetSnapSlot == ETargetSnapSlot::Floor)
    {
        // Only look for walls
        TargetNameContainsString = TEXT("Floor");
    }
    else
    {
        TargetNameContainsString = TEXT("Wall");
    }

    bool bSnapping = false;

    // TODO: Get best hit result that is closest to cursor?

    FHitResult TargetHitResult = OutHits[0];
    for (const FHitResult& HitResult : OutHits)
    {
        bool bHitBuildable = HitResult.GetActor() && HitResult.GetActor()->IsA(ABuildable::StaticClass());
        if (!bHitBuildable)
        {
            continue;
        }

        if (HitResult.Component.IsValid() && HitResult.Component->GetName().Contains(TargetNameContainsString))
        {
            bSnapping = true;
            TargetHitResult = HitResult;
            break;
        }
    }

    if (bSnapping)
    {
        HitLocation = TargetHitResult.Component->GetComponentLocation();
        TargetTransform.SetLocation(HitLocation);
        TargetTransform.SetRotation(TargetHitResult.Component->GetComponentRotation().Quaternion());

        if (!TargetTransform.Equals(this->ClientTargetTransform, 0.01f))
        {
            this->ServerSetTargetTransform(TargetTransform);
        }
    }
    else
    {
        if (this->TargetLocationRepTimer >= this->TargetLocationRepFrequency)
        {
            this->TargetLocationRepTimer = 0;
            this->ServerSetTargetTransform(TargetTransform);
        }
    }

    this->ClientTargetTransform.SetLocation(TargetTransform.GetLocation());
}

void UBuildingComponent::HandleDeleteMode(TArray<FHitResult>& OutHits)
{
    if (OutHits.Num() <= 0)
    {
        return;
    }

    AActor* ActorDirectlyHit = nullptr;

    // Last hit should be a blocking hit of a buildable
    ActorDirectlyHit = OutHits[OutHits.Num() - 1].GetActor();

    // Get current buildable under mouse and set material
    if (ActorDirectlyHit && ActorDirectlyHit->IsA(ABuildable::StaticClass()))
    {
        // Reduce casting
        if (this->LocalHoveringBuildableActor != ActorDirectlyHit)
        {
            if (this->LocalHoveringBuildableActor)
            {
                ABuildable* PreviousHoveringBuildable = Cast<ABuildable>(this->LocalHoveringBuildableActor);
                if (PreviousHoveringBuildable)
                {
                    PreviousHoveringBuildable->ResetMaterial();
                }
            }

            this->LocalHoveringBuildableActor = ActorDirectlyHit;

            ABuildable* HoveringBuildable = Cast<ABuildable>(this->LocalHoveringBuildableActor);

            if (this->BuildableHoveringToDelete != HoveringBuildable)
            {
                if (HoveringBuildable)
                {
                    HoveringBuildable->SetMaterial(this->InvalidPreviewMaterial);
                }

                this->ServerSetBuildableHoveringToDelete(HoveringBuildable);
            }
        }
    }
    else
    {
        if (this->LocalHoveringBuildableActor)
        {
            ABuildable* PreviousHoveringBuildable = Cast<ABuildable>(this->LocalHoveringBuildableActor);
            if (PreviousHoveringBuildable)
            {
                PreviousHoveringBuildable->ResetMaterial();
            }

            this->ServerSetBuildableHoveringToDelete(nullptr);

            this->LocalHoveringBuildableActor = nullptr;
        }
    }
}
