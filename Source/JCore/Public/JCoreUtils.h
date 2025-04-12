#pragma once

#include "CoreMinimal.h"
#include "Kismet/GameplayStatics.h"

class JCoreUtils
{
public:
    /** Return location closest to the TargetLocation from the given InLocations, in world space. */
    static const FVector GetClosestLocationToPoint(const FVector &TargetLocation, const TArray<FVector> &InLocations);

    /** Return FTransform closest to the TargetLocation from the given InTransforms, in world space. */
    static const FTransform GetClosestTransformToPoint(const FVector &TargetLocation, const TArray<FTransform> &InTransforms);

    /** Return FTransform closest to the TargetLocation from the given InTransforms, in world space. */
    static USceneComponent* GetClosestSceneComponentToPoint(const FVector &TargetLocation, const TArray<USceneComponent*> &InSceneComponents);

    template <typename TSubsystemClass>
    static TSubsystemClass* GetSubsystem(UWorld* InWorld)
    {
        UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(InWorld);
        ensure(GameInstance);

        return GameInstance->GetSubsystem<TSubsystemClass>();
    }
};
