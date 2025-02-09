
#include "JcoreUtils.h"

const FVector JCoreUtils::GetClosestLocationToPoint(const FVector&         TargetLocation,
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

const FTransform JCoreUtils::GetClosestTransformToPoint(const FVector&            TargetLocation,
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
