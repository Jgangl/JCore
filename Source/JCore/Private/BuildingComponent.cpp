// Fill out your copyright notice in the Description page of Project Settings.


#include "BuildingComponent.h"

#include "Buildable.h"
#include "BuildableInterface.h"
#include "Kismet/GameplayStatics.h"
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
    this->SetBuildMode(false);
    this->ClearBuildingPreview();
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

    this->CurrentBuildingClassInPreview = ActorClassToPreview;
    this->CurrentBuildingPreview = BuildingPreview;

    if (!this->CurrentBuildingPreview)
    {
        UE_LOG(LogBuildingComponent, Error, TEXT("Spawn Build Ghost failed"))
        return;
    }

    this->CurrentBuildingPreview->UpdateMesh(this->CurrentBuildingClassInPreview);
}

void UBuildingComponent::ServerRotateBuildObject_Implementation(const FRotator &DeltaRotation)
{
    if (!this->CurrentBuildingPreview)
        return;

    FRotator NewRotation    = this->ServerTargetTransform.Rotator() + DeltaRotation;
    FTransform NewTransform = this->ServerTargetTransform;
    NewTransform.SetRotation(NewRotation.Quaternion());

    this->ServerSetTargetTransform(NewTransform);
}

void UBuildingComponent::RotateBuildObject(bool bClockwise)
{
    const FRotator DeltaRotation = bClockwise
                                       ? FRotator(0.0f, this->RotationGridSnapValue, 0.0f)
                                       : FRotator(0.0f, -this->RotationGridSnapValue, 0.0f);

    this->ClientTargetTransform.SetRotation((this->ClientTargetTransform.GetRotation().Rotator() + DeltaRotation).Quaternion());
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

    if (this->CurrentBuildingPreview)
    {
        this->CurrentBuildingPreview->UpdateMesh(this->CurrentBuildingClassInPreview);
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

void UBuildingComponent::ClearBuildingPreview()
{
    if (this->CurrentBuildingPreview)
    {
        this->CurrentBuildingPreview->Destroy();
        this->CurrentBuildingPreview = nullptr;

        this->CurrentBuildingClassInPreview = nullptr;
    }
}

bool UBuildingComponent::TryBuild()
{
    if (!this->CurrentBuildingPreview)
    {
        UE_LOG(LogBuildingComponent, Verbose, TEXT("TryBuild : CurrentBuildingPreview null"))
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

    this->OnCompletedBuilding.Broadcast(SpawnedActor);

    // Notify the buildable that it is finished building
    if (IBuildableInterface* BuildableInterface = Cast<IBuildableInterface>(SpawnedActor))
    {
        BuildableInterface->CompleteBuilding();
    }

    this->PreviouslyCompletedBuilding = SpawnedActor;

    return true;
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
        UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_GameTraceChannel2),
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

    bool bSnapping = false;
    FVector ClosestSnapLocation = FVector::Zero();

    // TODO: Get best hit result that is closest to snap location?

    FHitResult TargetHitResult = OutHits[0];
    for (const FHitResult& HitResult : OutHits)
    {
        IBuildableInterface* BuildableInterface = Cast<IBuildableInterface>(HitResult.GetActor());

        if (!BuildableInterface)
        {
            continue;
        }

        TArray<FVector> PipeSnapLocations;

        BuildableInterface->GetPipeSnapLocations(PipeSnapLocations);

        float ClosestSnapLocationDistance = 10000000.0f;

        // Get closest snap location
        for (const FVector SnapLocation : PipeSnapLocations)
        {
            if (this->bDebug)
            {
                //DrawDebugSphere(GetWorld(), SnapLocation, 20.0f, 8, FColor::Black, false);
            }

            float Distance = FVector::Distance(HitLocation, SnapLocation);

            if (Distance < ClosestSnapLocationDistance)
            {
                ClosestSnapLocation         = SnapLocation;
                ClosestSnapLocationDistance = Distance;
            }
        }

        bSnapping = true;
        TargetHitResult = HitResult;
        break;
    }

    if (bSnapping)
    {
        //HitLocation = TargetHitResult.Component->GetComponentLocation();
        TargetTransform.SetLocation(ClosestSnapLocation);
        //TargetTransform.SetRotation(TargetHitResult.Component->GetComponentRotation().Quaternion());

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
