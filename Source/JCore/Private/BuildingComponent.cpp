// Fill out your copyright notice in the Description page of Project Settings.


#include "BuildingComponent.h"

#include "Buildable.h"
#include "Inventory/InventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

UBuildingComponent::UBuildingComponent()
{
    this->PrimaryComponentTick.bCanEverTick = true;

    this->RotationGridSnapValue      = 10.0f;
    this->BuildingPreviewInterpSpeed = 25.0f;
    this->bInDeleteMode              = false;
    this->bInBuildMode               = false;

    this->GridTileSizeX = 100.0f;
    this->GridTileSizeY = 100.0f;
    this->GridTileSizeZ = 100.0f;

    this->GridOffsetX = 50.0f;
    this->GridOffsetY = 50.0f;
    this->GridOffsetZ = 0.0f;

    this->bDebug = false;

    this->bFirstPersonInteraction = true;

    this->BuildingPreviewRotationAxis = EAxis::Type::Z;
    this->BuildingPreviewRotationOffset = FRotator(0.0f, 0.0f, 0.0f);

    this->CurrentBuildingPreviewRecipe = nullptr;

    this->DeleteHoldTime = 0.45f;

    this->bRequireItemsToBuild = true;

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
                                 this->BuildingPreviewInterpSpeed);

        this->CurrentBuildingPreview->SetActorLocation(LerpedLocation);
        this->CurrentBuildingPreview->SetActorRotation(this->ServerTargetTransform.GetRotation());
    }

    APawn* OwningPawn = Cast<APawn>(GetOwner());

    if (!OwningPawn || !OwningPawn->IsLocallyControlled())
    {
        return;
    }

    if (!this->bInBuildMode && !this->bInDeleteMode)
    {
        return;
    }

    TArray<FHitResult> OutHits;

    if (this->bFirstPersonInteraction)
    {
        this->GetFirstPersonHitResults(OutHits);
    }
    else
    {
        this->GetHitResultsUnderCursor(OutHits);
    }

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
    this->SetBuildMode(false);
    this->ClearBuildingPreview(true);
}

void UBuildingComponent::ServerStartBuildPreview_Implementation(TSubclassOf<AActor> ActorClassToPreview)
{
    if (!ActorClassToPreview)
    {
        return;
    }

    AActor* ActorToPreview = ActorClassToPreview.GetDefaultObject();

    if (!ActorToPreview)
    {
        UE_LOG(LogBuildingComponent, Error, TEXT("ActorToPreview null"))
        return;
    }

    this->ActorClassToSpawn = ActorClassToPreview;

    AActor* Player = this->GetOwner();

    if (!Player)
    {
        return;
    }

    // Delete previous preview
    if (this->CurrentBuildingPreview)
    {
        this->CurrentBuildingPreview->Destroy();
        this->CurrentBuildingPreview = nullptr;
    }

    FVector GridCursorLocation = this->GetClosestGridLocationToCursor();

    this->AddCurrentBuildableOffset(GridCursorLocation);

    const FTransform SpawnTransform(FQuat::Identity, GridCursorLocation, FVector::One());

    this->ServerSetTargetTransform(SpawnTransform);

    this->ClientTargetTransform = SpawnTransform;

    ABuildable* BuildingPreview =
        GetWorld()->SpawnActor<ABuildable>(this->ActorClassToSpawn, SpawnTransform);

    this->CurrentBuildingPreview        = BuildingPreview;
    this->CurrentBuildingClassInPreview = ActorClassToPreview;

    if (!this->CurrentBuildingPreview)
    {
        UE_LOG(LogBuildingComponent, Error, TEXT("Spawn Build Ghost failed"))
        return;
    }

    this->CurrentBuildingPreview->SetIsPreviewing(true);
}

void UBuildingComponent::RotateBuildObject(bool bClockwise)
{
    // If the building preview is snapped, we want to rotate around the snapped connection's X Axis.
    // If the building preview is NOT snapped, we want to rotate around the world Z Axis.

    if (!this->CurrentBuildingPreview)
    {
        UE_LOG(LogTemp, Error, TEXT("%hs : this->CurrentBuildingPreview is nullptr"), __FUNCTION__);
        return;
    }

    FRotator DeltaRotation;
    float RotationAmount = bClockwise ? this->RotationGridSnapValue :
                                        -this->RotationGridSnapValue;

    if (this->bIsSnapping)
    {
        DeltaRotation = FRotator(0.0f, 0.0f, RotationAmount);
    }
    else
    {
        switch (this->BuildingPreviewRotationAxis)
        {
            case EAxis::Type::X :
            {
                DeltaRotation = FRotator(RotationAmount, 0.0f, 0.0f);
                break;
            }
            case EAxis::Type::Y :
            {
                DeltaRotation = FRotator(0.0f, 0.0f, RotationAmount);
                break;
            }
            case EAxis::Type::Z :
            {
                DeltaRotation = FRotator(0.0f, RotationAmount, 0.0f);
                break;
            }
            default :
            {
                UE_LOG(LogTemp, Warning, TEXT("Invalid rotation axis"));
            }
        }
    }

    this->BuildingPreviewRotationOffset += DeltaRotation;
}

void UBuildingComponent::SetDeleteMode(bool InDeleteMode)
{
    if (this->bInDeleteMode != InDeleteMode)
    {
        this->bInDeleteMode = InDeleteMode;

        if (this->bInDeleteMode)
        {
            UE_LOG(LogTemp, Verbose, TEXT("Enter Delete Mode"));
            this->ServerCancelBuilding();
        }
        else
        {
            UE_LOG(LogTemp, Verbose, TEXT("Exit Delete Mode"));

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

            this->CancelDeleting();
        }
    }
}

void UBuildingComponent::SetBuildMode(bool InBuildMode)
{
    if (this->bInBuildMode != InBuildMode)
    {
        this->bInBuildMode = InBuildMode;

        // Exit delete mode when entering build mode
        if (this->bInBuildMode)
        {
            this->SetDeleteMode(false);
        }
    }
}

void UBuildingComponent::ServerStartBuilding_Implementation(TSubclassOf<AActor> ActorToBuild)
{
    this->BuildingPreviewRotationOffset = FRotator::ZeroRotator;
    this->BuildingPreviewSnapIndex      = 0;

    this->SetBuildMode(true);
    this->ServerStartBuildPreview(ActorToBuild);
}

AActor* UBuildingComponent::GetPreviouslyCompletedBuilding()
{
    return this->PreviouslyCompletedBuilding;
}

void UBuildingComponent::SetActorClassToSpawn(TSubclassOf<AActor> InActorClassToSpawn)
{
    this->ActorClassToSpawn = InActorClassToSpawn;

    if (this->IsTryingToPlaceBuilding())
    {
        this->CurrentBuildingClassInPreview = this->ActorClassToSpawn;
    }
}

void UBuildingComponent::SetCurrentBuildingRecipe(UBuildingRecipeDataAsset* InBuildingRecipe)
{
    this->CurrentBuildingPreviewRecipe = InBuildingRecipe;
}

void UBuildingComponent::AddCurrentBuildableOffset(FVector& InLocation) const
{
    if (ABuildable* Buildable = Cast<ABuildable>(this->ActorClassToSpawn->GetDefaultObject()))
    {
        InLocation += Buildable->GetBuildingOffset();
    }
}

void UBuildingComponent::IncrementBuildingPreviewRotationAxis()
{
    if (this->bIsSnapping)
    {
        this->IncrementBuildingPreviewSnapIndex();
    }
    else
    {
        uint8 NextAxis = this->BuildingPreviewRotationAxis + 1;

        if (NextAxis > 3)
        {
            // Ignore "None" Axis
            NextAxis = 1;
        }

        this->BuildingPreviewRotationAxis = static_cast<EAxis::Type>(NextAxis);
    }
}

void UBuildingComponent::StartCopyBuilding()
{
    TArray<FHitResult> OutHits;
    this->GetFirstPersonHitResults(OutHits);

    if (OutHits.Num() == 0)
    {
        return;
    }

    if (ABuildable* BuildableUnderCursor = Cast<ABuildable>(OutHits[0].GetActor()))
    {
        const TSubclassOf<ABuildable> CopiedBuildableClass = BuildableUnderCursor->GetClass();

        this->ServerStartBuilding(CopiedBuildableClass);
    }
}

const FTimerHandle& UBuildingComponent::GetDeleteTimerHandle() const
{
    return this->DeleteTimerHandle;
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

void UBuildingComponent::ServerStartBuildingFromRecipe_Implementation(UBuildingRecipeDataAsset* RecipeDataAsset)
{
    if (!RecipeDataAsset)
    {
        UE_LOG(LogTemp, Error, TEXT("%hs : RecipeDataAsset is nullptr"), __FUNCTION__);
        return;
    }

    this->SetCurrentBuildingRecipe(RecipeDataAsset);

    UBuildingDataAsset* BuildingDataAsset = RecipeDataAsset->GetOutBuilding();

    if (!BuildingDataAsset)
    {
        UE_LOG(LogTemp, Error, TEXT("%hs : BuildingDataAsset is nullptr"), __FUNCTION__);
        return;
    }

    this->ServerStartBuilding(BuildingDataAsset->GetBuildableClass());
}

void UBuildingComponent::ClearBuildingPreview(bool bDestroy)
{
    if (this->CurrentBuildingPreview)
    {
        if (bDestroy)
        {
            this->CurrentBuildingPreview->Destroy();
        }

        this->CurrentBuildingPreview = nullptr;
    }

    this->CurrentBuildingClassInPreview = nullptr;
    this->CurrentBuildingPreviewRecipe  = nullptr;
}

void UBuildingComponent::ServerTryBuild_Implementation()
{
    ABuildable* BuildableToBuild                     = this->CurrentBuildingPreview;
    UBuildingRecipeDataAsset* PreviousBuildingRecipe = this->CurrentBuildingPreviewRecipe;

    if (!BuildableToBuild)
    {
        UE_LOG(LogBuildingComponent, Verbose, TEXT("TryBuild : BuildableToBuild null"))
        return;
    }

    // Check placement validity
    if (BuildableToBuild->RequiresOverlapCheck() && !BuildableToBuild->IsPlacementValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("%hs : Building placement is invalid"), __FUNCTION__);
        // TODO: Warning to player that attempted placement is invalid
        return;
    }

    AActor* Owner = GetOwner();

    if (!Owner)
    {
        UE_LOG(LogTemp, Error, TEXT("%hs : Owner is nullptr"), __FUNCTION__);
        return;
    }

    UInventoryComponent* PlayerInventory = Cast<UInventoryComponent>(Owner->GetComponentByClass(UInventoryComponent::StaticClass()));

    if (!PlayerInventory)
    {
        UE_LOG(LogTemp, Error, TEXT("%hs : PlayerInventory was not found on Owner"), __FUNCTION__);
        return;
    }

    if (!this->CurrentBuildingPreviewRecipe)
    {
        UE_LOG(LogTemp, Error, TEXT("%hs : CurrentBuildingPreviewRecipe is nullptr"), __FUNCTION__);
        return;
    }

    // Check if player has buildable recipe items
    // Try to remove items
    // If successful, complete building
    if (this->bRequireItemsToBuild)
    {
        bool bPlayerHasSufficientItems = PlayerInventory->ContainsGivenItems(this->CurrentBuildingPreviewRecipe->GetInItems());

        if (!bPlayerHasSufficientItems)
        {
            // TODO: Warn player of insufficient items
            UE_LOG(LogBuildingComponent, Warning, TEXT("ServerTryBuild : Insufficient items for building"));
            return;
        }

        // Remove items from inventory
        if (!PlayerInventory->TryRemoveGivenItems(this->CurrentBuildingPreviewRecipe->GetInItems()))
        {
            UE_LOG(LogBuildingComponent, Warning, TEXT("ServerTryBuild : Removing items failed, this is not good, should be fixed if this is reached"));
            return;
        }
    }

    this->ClearBuildingPreview(false);

    BuildableToBuild->SetActorTransform(this->ServerTargetTransform);

    this->OnCompletedBuilding.Broadcast(BuildableToBuild);

    // Notify the buildable that it is finished building
    if (IBuildableInterface* BuildableInterface = Cast<IBuildableInterface>(BuildableToBuild))
    {
        BuildableInterface->CompleteBuilding();
    }

    this->PreviouslyCompletedBuilding = BuildableToBuild;

    this->ServerStartBuildingFromRecipe(PreviousBuildingRecipe);
}

void UBuildingComponent::StartDeleting()
{
    if (!this->BuildableHoveringToDelete)
    {
        return;
    }

    UWorld* World = GetWorld();

    if (!World) return;

    World->GetTimerManager().SetTimer(this->DeleteTimerHandle,
                                      this,
                                      &UBuildingComponent::FinishDeleting,
                                      this->DeleteHoldTime,
                                      false);

    this->OnDeletingStarted.Broadcast();
}

void UBuildingComponent::CancelDeleting()
{
    UWorld* World = GetWorld();

    if (!World) return;

    World->GetTimerManager().ClearTimer(this->DeleteTimerHandle);

    this->OnDeletingCanceled.Broadcast();
}

void UBuildingComponent::FinishDeleting()
{
    if (!this->BuildableHoveringToDelete)
    {
        return;
    }

    UWorld* World = GetWorld();

    if (!World) return;

    World->GetTimerManager().ClearTimer(this->DeleteTimerHandle);

    this->BuildableHoveringToDelete->Destroy();
    this->BuildableHoveringToDelete = nullptr;
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
    FVector End    = MouseLocation + MouseRotation * Distance;

    TArray<AActor*> ActorsToIgnore = {PlayerController->GetPawn(), this->CurrentBuildingPreview};

    // TODO: Try to only hit landscape or other building objects?

    // TODO: Do a sphere trace instead of line trace to more accurately hit objects
    UKismetSystemLibrary::LineTraceMulti(
        GetWorld(),
        MouseLocation,
        End,
        UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_Visibility),
        false,
        ActorsToIgnore,
        EDrawDebugTrace::None,
        OutHits,
        true);
}

void UBuildingComponent::GetFirstPersonHitResults(TArray<FHitResult>& OutHits) const
{
    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PlayerController || !PlayerController->PlayerCameraManager)
    {
        return;
    }

    FVector CameraLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
    FVector CameraDirection = PlayerController->PlayerCameraManager->GetCameraRotation().Vector();

    float Distance = 5000.0f;
    FVector Start  = CameraLocation;
    FVector End    = Start + (CameraDirection * Distance);
    float Radius   = 10.0f;

    TArray<AActor*> ActorsToIgnore = {PlayerController->GetPawn(), this->CurrentBuildingPreview};

    // TODO: Try to only hit landscape or other building objects?

    // TODO: Do a sphere trace instead of line trace to more accurately hit objects
    UKismetSystemLibrary::SphereTraceMulti(
        GetWorld(),
        Start,
        End,
        Radius,
        UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_Visibility),
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

const FTransform UBuildingComponent::GetClosestConnectionTransform(const FVector &Location, const TArray<FTransform> &ConnectionTransforms)
{
    float ClosestSnapLocationDistance = 10000000.0f;
    FTransform ClosestTransform = FTransform();

    // Get closest snap location
    for (const FTransform SnapTransform : ConnectionTransforms)
    {
        const float Distance = FVector::Distance(Location, SnapTransform.GetLocation());

        if (Distance < ClosestSnapLocationDistance)
        {
            ClosestTransform            = SnapTransform;
            ClosestSnapLocationDistance = Distance;
        }
    }

    return ClosestTransform;
}

TSubclassOf<ABuildable> UBuildingComponent::GetCurrentRecipeBuildingClass() const
{
    if (!this->CurrentBuildingPreviewRecipe)
    {
        return nullptr;
    }

    UBuildingDataAsset* BuildingDataAsset = this->CurrentBuildingPreviewRecipe->GetOutBuilding();

    if (!BuildingDataAsset)
    {
        return nullptr;
    }

    return BuildingDataAsset->GetBuildableClass();
}

void UBuildingComponent::HandleBuildingPreview(TArray<FHitResult>& OutHits)
{
    if (!this->CurrentBuildingPreview)
    {
        UE_LOG(LogTemp, Error, TEXT("%hs : this->CurrentBuildingPreview is nullptr"), __FUNCTION__);
        return;
    }

    if (OutHits.Num() == 0)
    {
        return;
    }

    // TODO: Get hit result different to the 1st one in array
    FHitResult TargetHitResult           = OutHits[0];
    IBuildableInterface* TargetBuildable = Cast<IBuildableInterface>(TargetHitResult.GetActor());

    FVector HitLocation  = TargetHitResult.Location;
    FVector GridLocation = this->GetGridLocation(HitLocation);
    this->AddCurrentBuildableOffset(GridLocation);

    if (this->bDebug)
    {
        DrawDebugSphere(GetWorld(), HitLocation, 20.0f, 8, FColor::Blue, false);
        DrawDebugSphere(GetWorld(), GridLocation, 20.0f, 8, FColor::Green, false);
    }

    FTransform TargetTransform(this->ClientTargetTransform.GetRotation(), GridLocation, this->CurrentBuildingPreview->GetTransform().GetScale3D());

    bool bSnapping = false;

    TArray<FTransform> OutTargetBuildablePipeSnapTransforms;

    if (TargetBuildable)
    {
        TargetBuildable->GetPipeSnapTransforms(OutTargetBuildablePipeSnapTransforms);

        if (OutTargetBuildablePipeSnapTransforms.Num() > 0)
        {
            bSnapping = true;
        }
    }

    // Floors
    // Look for any buildables near current preview location
    // Find if there are any snap locations near current preview location
    // If current preview location is within a certain range, snap preview to location

    // TODO: Fix snap location function to be snap transform since for walls, we need to rotate the snap points to match the snap points rotation

    FTransform SnapTransform = this->GetClosestBuildableSnapTransform(HitLocation, EBuildingSnapType::Wall);

    if (!SnapTransform.Equals(FTransform::Identity))
    {
        TargetTransform.SetLocation(SnapTransform.GetLocation());
        TargetTransform.SetRotation(SnapTransform.GetRotation());

        this->ServerSetTargetTransform(TargetTransform);

        return;
    }

/*
    UStaticMeshComponent* HitStaticMeshComponent = Cast<UStaticMeshComponent>(TargetHitResult.GetComponent());

    if(HitStaticMeshComponent && HitStaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources.Num() > 0)
    {
        FPositionVertexBuffer* VertexBuffer = &HitStaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[0].VertexBuffers.PositionVertexBuffer;
        if (VertexBuffer)
        {
            const int32 VertexCount = VertexBuffer->GetNumVertices();
            for (int32 Index = 0;  Index < VertexCount; Index++ )
            {
                //This is in the Static Mesh Actor Class, so it is location and tranform of the SMActor
                const FVector WorldSpaceVertexLocation = **GetActorLocation() + GetTransform().TransformVector(VertexBuffer->VertexPosition(Index));**
                //add to output FVector array
            }
        }
    }
*/

    // TODO: Abstract this function into a utils class
    const FTransform ClosestTransform = this->GetClosestConnectionTransform(HitLocation, OutTargetBuildablePipeSnapTransforms);

    FRotator OppositeRotatorWorld = (ClosestTransform.GetLocation() + ClosestTransform.GetUnitAxis(EAxis::X) * -50.0f - ClosestTransform.GetLocation()).Rotation();

    TArray<FTransform> OutBuildingPreviewSnapTransforms;
    this->CurrentBuildingPreview->GetPipeSnapTransforms(OutBuildingPreviewSnapTransforms);

    if (bSnapping && OutBuildingPreviewSnapTransforms.Num() > 0)
    {
        this->bIsSnapping = true;

        if (!OutBuildingPreviewSnapTransforms.IsValidIndex(this->BuildingPreviewSnapIndex))
        {
            UE_LOG(LogTemp, Error, TEXT("BuildingPreviewSnapIndex is invalid"));
            return;
        }

        if (this->bDebug)
        {
            FVector End = ClosestTransform.GetLocation() + OppositeRotatorWorld.Vector() * 100.0f;
            DrawDebugLine(GetWorld(), ClosestTransform.GetLocation(), End, FColor::Green);
            DrawDebugSphere(GetWorld(), End, 20.0f, 10, FColor::Green);
        }

        FTransform PreviewSnapTransformWorld    = OutBuildingPreviewSnapTransforms[this->BuildingPreviewSnapIndex];
        FTransform PreviewSnapRelativeTransform = PreviewSnapTransformWorld.GetRelativeTransform(this->CurrentBuildingPreview->GetTransform());

        FQuat RotationOffsetWorld = FQuat(PreviewSnapRelativeTransform.GetRotation().Vector(), FMath::DegreesToRadians(this->BuildingPreviewRotationOffset.Euler().X));

        FRotator TargetRotation = (OppositeRotatorWorld.Quaternion() * PreviewSnapRelativeTransform.Rotator().GetInverse().Quaternion()).Rotator();

        // Add rotation offset
        TargetRotation = (TargetRotation.Quaternion() * RotationOffsetWorld).Rotator();

        FVector RelativePreviewSnapLoc = this->CurrentBuildingPreview->GetTransform().InverseTransformPosition(PreviewSnapTransformWorld.GetLocation());

        RelativePreviewSnapLoc = TargetRotation.RotateVector(RelativePreviewSnapLoc);

        FVector CalculatedLocation = ClosestTransform.GetLocation() - RelativePreviewSnapLoc;
        TargetTransform.SetRotation(TargetRotation.Quaternion());
        TargetTransform.SetLocation(CalculatedLocation);

        this->ServerSetTargetTransform(TargetTransform);
    }
    else
    {
        this->bIsSnapping = false;

        TargetTransform.SetRotation(this->BuildingPreviewRotationOffset.Quaternion());

        if (this->TargetLocationRepTimer >= this->TargetLocationRepFrequency)
        {
            this->TargetLocationRepTimer = 0;
            this->ServerSetTargetTransform(TargetTransform);
        }
    }

    this->ClientTargetTransform.SetRotation(TargetTransform.GetRotation());
    this->ClientTargetTransform.SetLocation(TargetTransform.GetLocation());
}

void UBuildingComponent::IncrementBuildingPreviewSnapIndex()
{
    this->BuildingPreviewSnapIndex++;

    int32 NumSnapTransforms = 0;

    if (this->CurrentBuildingPreview)
    {
        TArray<FTransform> SnapTransforms;
        this->CurrentBuildingPreview->GetPipeSnapTransforms(SnapTransforms);

        NumSnapTransforms = SnapTransforms.Num();
    }

    if (this->BuildingPreviewSnapIndex == NumSnapTransforms)
    {
        this->BuildingPreviewSnapIndex = 0;
    }
}

const FTransform UBuildingComponent::GetClosestBuildableSnapTransform(const FVector& Center, EBuildingSnapType SnapType) const
{
    if (!this->CurrentBuildingPreview)
    {
        UE_LOG(LogTemp, Error, TEXT("%hs : this->CurrentBuildingPreview is nullptr"), __FUNCTION__);
        return FTransform::Identity;
    }

    FVector BuildingPreviewOrigin;
    FVector BuildingPreviewExtents;
    this->CurrentBuildingPreview->GetActorBounds(false, BuildingPreviewOrigin, BuildingPreviewExtents, false);

    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldStatic));

    TArray<AActor*> ActorsToIgnore{this->CurrentBuildingPreview};

    TArray<AActor*> OverlappingActors;
    UKismetSystemLibrary::SphereOverlapActors(GetWorld(),
                                              Center,
                                              150.0f,
                                              ObjectTypes,
                                              ABuildable::StaticClass(),
                                              ActorsToIgnore,
                                              OverlappingActors);

    if (this->bDebug)
    {
        DrawDebugSphere(GetWorld(), Center, 150.0f, 10, FColor::Blue);
    }

    // If snapping wall to a wall, then we only want to snap left/right, we probably want to match the rotation of the existing wall though
    // If snapping wall to floor, then we want to snap all 4 directions but we need to rotate the wall snap points
    // If snapping floor to floor, then we want to snap all 4 directions, we don't want to rotate
    // If snapping floor to wall, then we want to snap all 4 directions, we don't want to rotate
    // Don't rotate if we are building a floor.
    // Rotate if we are building a wall and it's getting snapped to a floor

    TArray<FTransform> PossibleSnapTransforms;
    TArray<FTransform> AvailableSnapTransforms;

    for (AActor* OverlappedActor : OverlappingActors)
    {
        ABuildable* OverlappedBuildable = Cast<ABuildable>(OverlappedActor);
        if (!OverlappedBuildable) continue;

        FVector BuildableLocation = OverlappedBuildable->GetActorLocation();

        float TranslationX = this->CurrentBuildingPreview->GetSize().X / 2.0f + OverlappedBuildable->GetSize().X / 2.0f;
        const float TranslationY = this->CurrentBuildingPreview->GetSize().Y / 2.0f + OverlappedBuildable->GetSize().Y / 2.0f;

        // If we are snapping a wall to floor, the snap point should be directly on the edge of the floor
        if (this->CurrentBuildingPreview->GetSnapType() == EBuildingSnapType::Wall &&
            OverlappedBuildable->GetSnapType() == EBuildingSnapType::Floor)
        {
            TranslationX = OverlappedBuildable->GetSize().X / 2.0f;
        }

        TArray<FTransform> BuildableSnapTransforms;
        OverlappedBuildable->GetSnapTransformsOfType(this->CurrentBuildingPreview->GetSnapType(), BuildableSnapTransforms);

        PossibleSnapTransforms.Append(BuildableSnapTransforms);



        // Can we put these snap transforms in the buildable itself in some sort of map like TMap<EBuildingSnapType, TArray<FTransform>>
        // where we can query each buildable for its snap transforms instead of doing the below garbage that's awful
        //FVector NorthSnapLocation = BuildableLocation + FVector(0.0f, TranslationY, 0.0f);
        //FVector SouthSnapLocation = BuildableLocation - FVector(0.0f, TranslationY, 0.0f);
        //FVector EastSnapLocation  = BuildableLocation + FVector(TranslationX, 0.0f, 0.0f);
        //FVector WestSnapLocation  = BuildableLocation - FVector(TranslationX, 0.0f, 0.0f);

        //FTransform NorthSnapTransform{NorthSnapLocation};
        //FTransform SouthSnapTransform{SouthSnapLocation};
        //FTransform EastSnapTransform{FRotator(0.0f, 90.0f, 0.0f), EastSnapLocation};
        //FTransform WestSnapTransform{FRotator(0.0f, -90.0f, 0.0f), WestSnapLocation};
/*
        if (this->CurrentBuildingPreview->GetSnapType() == EBuildingSnapType::Wall &&
            OverlappedBuildable->GetSnapType() == EBuildingSnapType::Wall)
        {
            PossibleSnapTransforms.Append({EastSnapTransform, WestSnapTransform});
        }
        else
        {
            PossibleSnapTransforms.Append({NorthSnapTransform, SouthSnapTransform, EastSnapTransform, WestSnapTransform});
        }
        */
    }

    // TODO: Put this in ABuildable as a property?
    FVector OverlapCushion = this->CurrentBuildingPreview->GetSize() * 0.02f;
    FVector OverlapCheckHalfExtents = (this->CurrentBuildingPreview->GetSize() - OverlapCushion) / 2.0f;

    for (const FTransform& PossibleSnapTransform : PossibleSnapTransforms)
    {
        if (this->IsSnapPointAvailable(PossibleSnapTransform, OverlapCheckHalfExtents))
        {
            AvailableSnapTransforms.Add(PossibleSnapTransform);
        }
    }

    float ClosestSnapDistance       = 9999999999999999.0f;
    FTransform ClosestSnapTransform = FTransform::Identity;

    // TODO: Move this into a utils class
    for (const FTransform& PossibleSnapTransform : PossibleSnapTransforms)
    {
        float CurrentSnapDistance = FVector::Distance(PossibleSnapTransform.GetLocation(), Center);

        if (CurrentSnapDistance < ClosestSnapDistance)
        {
            ClosestSnapTransform = PossibleSnapTransform;
            ClosestSnapDistance = CurrentSnapDistance;
        }
    }

    return ClosestSnapTransform;
}

bool UBuildingComponent::IsSnapPointAvailable(const FTransform& SnapTransform, const FVector& Extents) const
{
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldStatic));

    const TArray<AActor*> ActorsToIgnore{this->CurrentBuildingPreview};

    // Do we need to rotate the extents by the rotation in the SnapTransform?
    const FVector RotatedExtents = Extents.RotateAngleAxis(SnapTransform.GetRotation().Euler().Z, FVector::UpVector);

    // Walls origin is at the ground, we need to offset the box overlap check to compensate for this
    //  Is there a better way to do this?
    FVector SnapOverlapLocation = SnapTransform.GetLocation() + FVector(0.0f, 0.0f, this->CurrentBuildingPreview->GetSize().Z / 2.0f);

    TArray<AActor*> SnapOverlappingActors;
    bool bSnapPointOccupied = UKismetSystemLibrary::BoxOverlapActors(GetWorld(),
                                                                     SnapOverlapLocation,
                                                                     RotatedExtents,
                                                                     ObjectTypes,
                                                                     ABuildable::StaticClass(),
                                                                     ActorsToIgnore,
                                                                     SnapOverlappingActors);

    if (this->bDebug)
    {
        DrawDebugBox(GetWorld(), SnapTransform.GetLocation(), RotatedExtents, FColor::Black);

        if (bSnapPointOccupied)
        {
            DrawDebugSphere(GetWorld(), SnapTransform.GetLocation(), 20.0f, 10, FColor::Green);
        }
    }

    return bSnapPointOccupied;
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
            // Reset previous buildable's material
            if (this->LocalHoveringBuildableActor)
            {
                ABuildable* PreviousHoveringBuildable = Cast<ABuildable>(this->LocalHoveringBuildableActor);
                if (PreviousHoveringBuildable)
                {
                    PreviousHoveringBuildable->ResetMaterial();
                }
            }

            UE_LOG(LogTemp, Verbose, TEXT("HandleDeleteMode: Set hovering actor to: %s"), *ActorDirectlyHit->GetName());
            this->LocalHoveringBuildableActor = ActorDirectlyHit;

            ABuildable* HoveringBuildable = Cast<ABuildable>(this->LocalHoveringBuildableActor);

            if (this->BuildableHoveringToDelete != HoveringBuildable)
            {
                if (HoveringBuildable)
                {
                    HoveringBuildable->SetMaterialInvalid();
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
