// Fill out your copyright notice in the Description page of Project Settings.


#include "BuildingComponent.h"

#include "Buildable.h"
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
        if (this->ActorClassesToSpawn.Num() == 0)
        {
            return;
        }

        ActorClassToPreview = this->ActorClassesToSpawn[0];
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
    this->ClearBuildingPreview(true);

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
    this->SetBuildMode(true);
    this->ServerStartBuildPreview(ActorToBuild);

    this->BuildingPreviewRotationOffset = FRotator::ZeroRotator;
    this->BuildingPreviewSnapIndex      = 0;
}

AActor* UBuildingComponent::GetPreviouslyCompletedBuilding()
{
    return this->PreviouslyCompletedBuilding;
}

void UBuildingComponent::IncrementSelectedActorToSpawn()
{
    if (!this->ActorClassesToSpawn.IsValidIndex(this->SelectedActorToSpawnIndex))
    {
        UE_LOG(LogBuildingComponent, Error, TEXT("IncrementSelectedActorToSpawn: SelectedActorToSpawnIndex (%d) is invalid"), this->SelectedActorToSpawnIndex)
        return;
    }

    this->SelectedActorToSpawnIndex++;

    // Wrap
    if (this->SelectedActorToSpawnIndex == this->ActorClassesToSpawn.Num())
    {
        this->SelectedActorToSpawnIndex = 0;
    }

    this->ActorClassToSpawn = this->ActorClassesToSpawn[this->SelectedActorToSpawnIndex];
}

void UBuildingComponent::SetActorClassToSpawn(TSubclassOf<AActor> InActorClassToSpawn)
{
    this->ActorClassToSpawn = InActorClassToSpawn;

    if (this->IsTryingToPlaceBuilding())
    {
        this->CurrentBuildingClassInPreview = this->ActorClassToSpawn;
    }
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
}

void UBuildingComponent::ServerTryBuild_Implementation()
{
    ABuildable* BuildableToBuild                         = this->CurrentBuildingPreview;
    const TSubclassOf<ABuildable> PreviousBuildableClass = this->CurrentBuildingClassInPreview;

    if (!BuildableToBuild)
    {
        UE_LOG(LogBuildingComponent, Verbose, TEXT("TryBuild : BuildableToBuild null"))
        return;
    }

    // Get inventory component, check if player has buildable recipe items
    // Try to remove items
    // If successful, complete building

    this->ClearBuildingPreview(false);

    BuildableToBuild->SetActorTransform(this->ServerTargetTransform);

    this->OnCompletedBuilding.Broadcast(BuildableToBuild);

    // Notify the buildable that it is finished building
    if (IBuildableInterface* BuildableInterface = Cast<IBuildableInterface>(BuildableToBuild))
    {
        BuildableInterface->CompleteBuilding();
    }

    this->PreviouslyCompletedBuilding = BuildableToBuild;

    this->ServerStartBuilding(PreviousBuildableClass);
}

bool UBuildingComponent::TryDelete()
{
    if (!this->BuildableHoveringToDelete)
    {
        return false;
    }

    this->BuildableHoveringToDelete->Destroy();
    this->BuildableHoveringToDelete = nullptr;

    return true;
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

void UBuildingComponent::HandleBuildingPreview(TArray<FHitResult>& OutHits)
{
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

    FTransform TargetTransform(this->ClientTargetTransform.GetRotation(), GridLocation, FVector::One());

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
