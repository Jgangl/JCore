
#include "Building/BuildingComponent.h"

#include "Building/Buildable.h"
#include "JCoreUtils.h"
#include "Inventory/BuildingSubsystem.h"
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
    this->bBuildOnWorldGrid          = false;

    this->GridTileSizeX = 100.0f;
    this->GridTileSizeY = 100.0f;
    this->GridTileSizeZ = 100.0f;

    this->GridOffsetX = 50.0f;
    this->GridOffsetY = 50.0f;
    this->GridOffsetZ = 0.0f;

    this->bDebug = false;

    this->bFirstPersonInteraction = true;

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
        this->CurrentBuildingPreview->SetActorLocation(this->ServerTargetTransform.GetLocation());
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

    TArray<AActor*> ActorsToIgnore = {this->CurrentBuildingPreview};

    if (this->bFirstPersonInteraction)
    {
        UJCoreUtils::GetFirstPersonHitResults(OutHits, GetWorld(), ActorsToIgnore);
    }
    else
    {
        UJCoreUtils::GetHitResultsUnderCursor(OutHits, GetWorld(), ActorsToIgnore);
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

    if (this->ConveyorBuildModeState == EBuildModeState::InProcess)
    {
        TArray<FTransform> StraightConveyorTransforms;


        //TArray<FTransform> StraightConveyorTransforms;
/*
        FVector EndConveyorLocation = this->GetGridLocation(OutHits[0].Location);

        if (AConveyor* Conveyor = Cast<AConveyor>(this->CurrentBuildingPreview))
        {
            Conveyor->CreateBaseInstances(this->InitialConveyorBuildLocation, EndConveyorLocation);
        }
*/
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

    this->ConveyorBuildModeState = EBuildModeState::None;
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
    const float RotationAmount = bClockwise ? this->RotationGridSnapValue : -this->RotationGridSnapValue;

    if (this->bIsSnapping)
    {
        DeltaRotation = FRotator(0.0f, 0.0f, RotationAmount);
    }
    else
    {
        DeltaRotation = FRotator(0.0f, RotationAmount, 0.0f);
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
            this->ServerCancelBuilding();
        }
        else
        {
            this->LocalHoveringBuildableActor = nullptr;

            this->ServerSetBuildableHoveringToDelete(nullptr);

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

void UBuildingComponent::SetBuildOnWorldGrid(bool InBuildOnWorldGrid)
{
    this->bBuildOnWorldGrid = InBuildOnWorldGrid;
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

void UBuildingComponent::StartCopyBuilding()
{
    TArray<FHitResult> OutHits;
    UJCoreUtils::GetFirstPersonHitResults(OutHits, GetWorld());

    if (OutHits.Num() == 0)
    {
        return;
    }

    if (ABuildable* BuildableUnderCursor = Cast<ABuildable>(OutHits[0].GetActor()))
    {
        const TSubclassOf<ABuildable> CopiedBuildableClass = BuildableUnderCursor->GetClass();

        UBuildingSubsystem* BuildingSubsystem = UJCoreUtils::GetSubsystem<UBuildingSubsystem>(GetWorld());
        ensure(BuildingSubsystem);

        this->ServerStartBuildingFromRecipe(BuildingSubsystem->GetRecipe(CopiedBuildableClass));
    }
}

const FTimerHandle& UBuildingComponent::GetDeleteTimerHandle() const
{
    return this->DeleteTimerHandle;
}

void UBuildingComponent::ServerSetBuildableHoveringToDelete_Implementation(ABuildable* NewBuildable)
{
    ABuildable* PreviousHoveringBuildable = this->BuildableHoveringToDelete;

    if (this->BuildableHoveringToDelete == NewBuildable)
    {
        return;
    }

    this->BuildableHoveringToDelete = NewBuildable;

    // Hovering a buildable to delete
    if (this->BuildableHoveringToDelete)
    {
        this->BuildableHoveringToDelete->MulticastSetMaterialInvalid();
    }

    if (PreviousHoveringBuildable)
    {
        PreviousHoveringBuildable->MulticastResetMaterial();
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
        if (!PlayerInventory->TryRemoveItems(this->CurrentBuildingPreviewRecipe->GetInItems()))
        {
            UE_LOG(LogBuildingComponent, Warning, TEXT("ServerTryBuild : Removing items failed, this is not good, should be fixed if this is reached"));
            return;
        }
    }

    if (BuildableToBuild->GetSnapType() == EBuildingSnapType::Conveyor &&
        this->ConveyorBuildModeState == EBuildModeState::None)
    {
        this->ConveyorBuildModeState = EBuildModeState::InProcess;
        BuildableToBuild->SetActorTransform(this->ServerTargetTransform);

        // Save initial position to be able to calculate a path from
        this->InitialConveyorBuildLocation = BuildableToBuild->GetActorLocation();

        return;
    }
    else if (BuildableToBuild->GetSnapType() == EBuildingSnapType::Conveyor &&
             this->ConveyorBuildModeState == EBuildModeState::InProcess)
    {
        this->ConveyorBuildModeState = EBuildModeState::None;

        FTransform NewTransform = this->ServerTargetTransform;
        NewTransform.SetLocation(this->InitialConveyorBuildLocation);
        BuildableToBuild->SetActorTransform(NewTransform);

        /*
        if (AConveyor* Conveyor = Cast<AConveyor>(this->CurrentBuildingPreview))
        {
            Conveyor->CreateBaseInstances(this->InitialConveyorBuildLocation, this->GetGridLocation(this->GetClosestGridLocationFromCamera()));
        }
        */

        this->InitialConveyorBuildLocation = FVector::Zero();
    }
    else
    {
        BuildableToBuild->SetActorTransform(this->ServerTargetTransform);
    }

    this->ClearBuildingPreview(false);

    this->OnCompletedBuilding.Broadcast(BuildableToBuild);

    // Notify the buildable that it is finished building
    if (IBuildableInterface* BuildableInterface = Cast<IBuildableInterface>(BuildableToBuild))
    {
        // We should pass in the building or connection we are snapping to
        BuildableInterface->CompleteBuilding(this->BuildingPreviewSnapConnections.Key, this->BuildingPreviewSnapConnections.Value);
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

    if (!World)
    {
        return;
    }

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
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    World->GetTimerManager().ClearTimer(this->DeleteTimerHandle);

    this->ServerFinishDeleting();
}

void UBuildingComponent::ServerFinishDeleting_Implementation()
{
    if (!this->BuildableHoveringToDelete)
    {
        return;
    }

    if (this->bRequireItemsToBuild)
    {
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

        UBuildingSubsystem* BuildingSubsystem = UJCoreUtils::GetSubsystem<UBuildingSubsystem>(GetWorld());
        ensure(BuildingSubsystem);

        UBuildingRecipeDataAsset* Recipe = BuildingSubsystem->GetRecipe(this->BuildableHoveringToDelete->GetClass());
        if (!Recipe)
        {
            UE_LOG(LogTemp, Error, TEXT("%hs : Recipe was not found"), __FUNCTION__);
            return;
        }

        // Add items back to inventory
        if (!PlayerInventory->TryAddItems(Recipe->GetInItems()))
        {
            UE_LOG(LogBuildingComponent, Warning, TEXT("FinishDeleting : Adding items failed. Most likely due to full inventory."));
            return;
        }
    }

    this->BuildableHoveringToDelete->Destroy();
    this->BuildableHoveringToDelete = nullptr;
}

void UBuildingComponent::ServerSetTargetTransform_Implementation(const FTransform& TargetTransform)
{
    this->ServerTargetTransform = TargetTransform;
}

const FVector UBuildingComponent::GetClosestGridLocationFromCamera() const
{
    TArray<FHitResult> OutHits;
    UJCoreUtils::GetFirstPersonHitResults(OutHits, GetWorld());

    if (OutHits.Num() == 0)
    {
        return FVector::Zero();
    }

    return this->GetGridLocation(OutHits[0].Location);
}

const FVector UBuildingComponent::GetClosestGridLocationToCursor() const
{
    TArray<FHitResult> OutHits;
    UJCoreUtils::GetHitResultsUnderCursor(OutHits, GetWorld());

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

    FHitResult TargetHitResult                = OutHits[0];
    IBuildableInterface* BuildableUnderCursor = Cast<IBuildableInterface>(TargetHitResult.GetActor());

    FVector HitLocation  = TargetHitResult.ImpactPoint;
    FVector GridLocation = this->GetGridLocation(HitLocation);
    this->AddCurrentBuildableOffset(GridLocation);

    if (this->bDebug)
    {
        DrawDebugSphere(GetWorld(), HitLocation, 20.0f, 8, FColor::Blue, false);
        DrawDebugSphere(GetWorld(), GridLocation, 20.0f, 8, FColor::Green, false);
    }

    FTransform TargetTransform(this->ClientTargetTransform.GetRotation(), HitLocation, this->CurrentBuildingPreview->GetTransform().GetScale3D());

    if (this->bBuildOnWorldGrid)
    {
        TargetTransform.SetLocation(GridLocation);
    }

    if (this->CurrentBuildingPreview->GetSnapType() == EBuildingSnapType::Wall ||
        this->CurrentBuildingPreview->GetSnapType() == EBuildingSnapType::Floor)
    {
        FTransform SnapTransform = this->GetClosestBuildableSnapTransform(HitLocation, EBuildingSnapType::Wall);

        if (!SnapTransform.Equals(FTransform::Identity))
        {
            TargetTransform.SetLocation(SnapTransform.GetLocation());
            TargetTransform.SetRotation(SnapTransform.GetRotation());

            this->ServerSetTargetTransform(TargetTransform);

            return;
        }
    }

    UBuildingConnectionComponent* ConnectionToSnapTo = nullptr;

    if (BuildableUnderCursor)
    {
        ConnectionToSnapTo = BuildableUnderCursor->GetClosestConnectionToLocation(HitLocation);
    }

    if (ConnectionToSnapTo && this->CurrentBuildingPreview->HasOpenConnections())
    {
        this->bIsSnapping = true;

        TArray<UBuildingConnectionComponent*> BuildingPreviewOpenConnections;
        this->CurrentBuildingPreview->GetOpenConnectionComponents(BuildingPreviewOpenConnections);

        if (!BuildingPreviewOpenConnections.IsValidIndex(this->BuildingPreviewSnapIndex))
        {
            UE_LOG(LogTemp, Error, TEXT("BuildingPreviewSnapIndex is invalid"));
            return;
        }

        const FTransform ClosestTransform = ConnectionToSnapTo->GetSnapTransform();

        // Calculate the rotation of where we want the snapped connection to be
        FRotator OppositeRotatorWorld = (ClosestTransform.GetLocation() + ClosestTransform.GetUnitAxis(EAxis::X) * -50.0f - ClosestTransform.GetLocation()).Rotation();

        if (this->bDebug)
        {
            FVector End = ClosestTransform.GetLocation() + OppositeRotatorWorld.Vector() * 100.0f;
            DrawDebugLine(GetWorld(), ClosestTransform.GetLocation(), End, FColor::Green);
            DrawDebugSphere(GetWorld(), End, 20.0f, 10, FColor::Green);
        }

        //UBuildingConnectionComponent* BuildingPreviewConnection = BuildingPreviewOpenConnections[this->BuildingPreviewSnapIndex];
        UBuildingConnectionComponent* BuildingPreviewConnection = this->CurrentBuildingPreview->GetOppositeTypeConnection(ConnectionToSnapTo->IsInput());

        if (!BuildingPreviewConnection)
        {
            UE_LOG(LogTemp, Error, TEXT("BuildingPreviewConnection of the opposite type was not found"));
            return;
        }

        FTransform PreviewSnapTransformWorld    = BuildingPreviewConnection->GetSnapTransform();
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

        // Update which connections are being snapped to be used when finishing building
        this->BuildingPreviewSnapConnections = {BuildingPreviewConnection, ConnectionToSnapTo};

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
        this->CurrentBuildingPreview->GetConnectionSnapTransforms(SnapTransforms);

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

        TArray<FTransform> BuildableSnapTransforms;
        OverlappedBuildable->GetSnapTransformsOfType(this->CurrentBuildingPreview->GetSnapType(), BuildableSnapTransforms);

        PossibleSnapTransforms.Append(BuildableSnapTransforms);
    }

    // TODO: Put this in ABuildable as a property?
    FVector OverlapCushion = this->CurrentBuildingPreview->GetSize() * 0.25f;
    FVector OverlapCheckHalfExtents = (this->CurrentBuildingPreview->GetSize() - OverlapCushion) / 2.0f;

    for (const FTransform& PossibleSnapTransform : PossibleSnapTransforms)
    {
        if (this->IsSnapPointAvailable(PossibleSnapTransform, OverlapCheckHalfExtents))
        {
            AvailableSnapTransforms.Add(PossibleSnapTransform);
        }
    }

    return UJCoreUtils::GetClosestTransformToPoint(Center, AvailableSnapTransforms);
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
    FVector SnapOverlapLocation = SnapTransform.GetLocation() + this->CurrentBuildingPreview->GetOriginOffset();

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
        DrawDebugBox(GetWorld(), SnapOverlapLocation, RotatedExtents, FColor::Black);

        if (!bSnapPointOccupied)
        {
            DrawDebugSphere(GetWorld(), SnapTransform.GetLocation(), 30.0f, 10, FColor::Green);
        }
    }

    return !bSnapPointOccupied;
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

            this->LocalHoveringBuildableActor = ActorDirectlyHit;

            ABuildable* HoveringBuildable = Cast<ABuildable>(this->LocalHoveringBuildableActor);

            if (this->BuildableHoveringToDelete != HoveringBuildable)
            {
                this->ServerSetBuildableHoveringToDelete(HoveringBuildable);
            }
        }
    }
    else
    {
        if (this->LocalHoveringBuildableActor)
        {
            this->ServerSetBuildableHoveringToDelete(nullptr);

            this->LocalHoveringBuildableActor = nullptr;
        }
    }
}
