
#include "JCoreUtils.h"

#include "Engine/World.h"

const FVector UJCoreUtils::GetClosestLocationToPoint(const FVector&         TargetLocation,
                                                    const TArray<FVector>& InLocations)
{
    float ClosestLocationDistSquared = FLT_MAX;
    FVector ClosestLocation = FVector::Zero();

    for (const FVector& Location : InLocations)
    {
        const float DistSquared = FVector::DistSquared(TargetLocation, Location);
        if (DistSquared < ClosestLocationDistSquared)
        {
            ClosestLocation            = Location;
            ClosestLocationDistSquared = DistSquared;
        }
    }

    return ClosestLocation;
}

const FTransform UJCoreUtils::GetClosestTransformToPoint(const FVector&            TargetLocation,
                                                        const TArray<FTransform>& InTransforms)
{
    float ClosestLocationDistSquared = FLT_MAX;
    FTransform ClosestTransform      = FTransform::Identity;

    for (const FTransform& Transform : InTransforms)
    {
        const float DistSquared = FVector::DistSquared(TargetLocation, Transform.GetLocation());
        if (DistSquared < ClosestLocationDistSquared)
        {
            ClosestTransform           = Transform;
            ClosestLocationDistSquared = DistSquared;
        }
    }

    return ClosestTransform;
}

USceneComponent* UJCoreUtils::GetClosestSceneComponentToPoint(const FVector&                  TargetLocation,
                                                             const TArray<USceneComponent*>& InSceneComponents)
{
    float ClosestLocationDistSquared = FLT_MAX;
    USceneComponent* ClosestComp     = nullptr;

    for (USceneComponent* SceneComponent : InSceneComponents)
    {
        if (!SceneComponent) continue;

        const float DistSquared = FVector::DistSquared(TargetLocation, SceneComponent->GetComponentLocation());
        if (DistSquared < ClosestLocationDistSquared)
        {
            ClosestLocationDistSquared = DistSquared;
            ClosestComp = SceneComponent;
        }
    }

    return ClosestComp;
}

void UJCoreUtils::GetHitResultsUnderCursor(TArray<FHitResult>& OutHits,
                                          UWorld*             InWorld)
{
    TArray<AActor*> EmptyActorArray;

    UJCoreUtils::GetHitResultsUnderCursor(OutHits, InWorld, EmptyActorArray);
}

void UJCoreUtils::GetHitResultsUnderCursor(TArray<FHitResult>& OutHits,
                                          UWorld*             InWorld,
                                          TArray<AActor*>&    ExtraActorsToIgnore)
{
    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(InWorld, 0);
    if (!PlayerController)
    {
        return;
    }

    FVector MouseLocation;
    FVector MouseRotation;

    PlayerController->DeprojectMousePositionToWorld(MouseLocation, MouseRotation);

    float Distance = 5000.0f;
    FVector End    = MouseLocation + MouseRotation * Distance;

    TArray<AActor*> ActorsToIgnore = {PlayerController->GetPawn()};

    if (!ExtraActorsToIgnore.IsEmpty())
    {
        ActorsToIgnore.Append(ExtraActorsToIgnore);
    }

    UKismetSystemLibrary::LineTraceMulti(
        InWorld,
        MouseLocation,
        End,
        UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_Visibility),
        false,
        ActorsToIgnore,
        EDrawDebugTrace::None,
        OutHits,
        true);
}

void UJCoreUtils::GetFirstPersonHitResults(TArray<FHitResult>& OutHits,
                                          UWorld*             InWorld)
{
    TArray<AActor*> EmptyActorArray;

    UJCoreUtils::GetFirstPersonHitResults(OutHits, InWorld, EmptyActorArray);
}

void UJCoreUtils::GetFirstPersonHitResults(TArray<FHitResult>& OutHits,
                                          UWorld*             InWorld,
                                          TArray<AActor*>&    ExtraActorsToIgnore)
{
    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(InWorld, 0);
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

    TArray<AActor*> ActorsToIgnore = {PlayerController->GetPawn()};

    if (!ExtraActorsToIgnore.IsEmpty())
    {
        ActorsToIgnore.Append(ExtraActorsToIgnore);
    }

    UKismetSystemLibrary::SphereTraceMulti(
        InWorld,
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
