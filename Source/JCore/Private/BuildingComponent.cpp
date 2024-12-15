// Fill out your copyright notice in the Description page of Project Settings.


#include "BuildingComponent.h"

#include "Buildable.h"
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
}

void UBuildingComponent::AddCurrentBuildableOffset(FVector& InLocation) const
{
    if (ABuildable* Buildable = Cast<ABuildable>(this->ActorClassToSpawn->GetDefaultObject()))
    {
        InLocation += Buildable->GetBuildingOffset();
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
    FHitResult TargetHitResult = OutHits[0];
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


    // TODO: I think we only want to get inverse (Add 180) to the forward axis of the connection component

    FRotator OppositeForwardRotatorWorld = ClosestTransform.GetRotation().Rotator() + FRotator(0.0f, 180.0f, 0.0f);
    //OppositeForwardRotator.Vec

    TArray<FTransform> OutBuildingPreviewSnapTransforms;
    this->CurrentBuildingPreview->GetPipeSnapTransforms(OutBuildingPreviewSnapTransforms);

    if (bSnapping && OutBuildingPreviewSnapTransforms.Num() > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("X: %f,  Y: %f,  Z: %f "), OppositeForwardRotatorWorld.Euler().X, OppositeForwardRotatorWorld.Euler().Y, OppositeForwardRotatorWorld.Euler().Z);
        //UE_LOG(LogTemp, Warning, TEXT("Transform X: %f,  Y: %f,  Z: %f "), ClosestTransform.GetRotation().Euler().X, ClosestTransform.GetRotation().Euler().Y, ClosestTransform.GetRotation().Euler().Z);
        // TODO: Calculate object rotation so that the connection components are facing each other

        FVector End = ClosestTransform.GetLocation() + ClosestTransform.GetRotation().Vector() * 100.0f;
        FVector InverseEnd = ClosestTransform.GetLocation() + OppositeForwardRotatorWorld.Vector() * 100.0f;
        //DrawDebugLine(GetWorld(), ClosestTransform.GetLocation(), End, FColor::Purple, false, -1, 0, 3);
        //DrawDebugSphere(GetWorld(), End, 25.0f, 8, FColor::Purple, false, -1, 0, 2);

        DrawDebugLine(GetWorld(), ClosestTransform.GetLocation(), InverseEnd, FColor::Green, false, -1, 0, 3);
        DrawDebugSphere(GetWorld(), InverseEnd, 25.0f, 8, FColor::Green, false, -1, 0, 2);


        // TODO: We need to utilize the chosen building preview snap rotation as well
        FTransform BuildingPreviewSnapTransform = OutBuildingPreviewSnapTransforms[0];

        FRotator BuildingPreviewRelativeRotator = BuildingPreviewSnapTransform.GetRelativeTransform(this->CurrentBuildingPreview->GetTransform()).Rotator();

        //FRotator RotationOffset = BuildingPreviewRelativeRotator - ClosestSnapRelativeRotator;

        FRotator TargetRotation = OppositeForwardRotatorWorld + BuildingPreviewRelativeRotator;


        // Add Offset
        //TargetRotation = TargetRotation + RotationOffset;

        FVector Loc = BuildingPreviewSnapTransform.GetLocation();
        FVector RelativePreviewSnapLoc = this->CurrentBuildingPreview->GetTransform().InverseTransformPosition(Loc);

        this->CurrentBuildingPreview->SetActorRotation(TargetRotation);

        // THIS ISN'T NEEDED ANYMORE, WHY YOU ASK? I DON'T KNOW
        // Rotate vector to align coordinate spaces
        FRotator Rotator = this->CurrentBuildingPreview->GetActorRotation();
        RelativePreviewSnapLoc = Rotator.RotateVector(RelativePreviewSnapLoc);

        FVector CalculatedLocation = ClosestTransform.GetLocation() - RelativePreviewSnapLoc;



        TargetTransform.SetRotation(TargetRotation.Quaternion());
        TargetTransform.SetLocation(CalculatedLocation);

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
