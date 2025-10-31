// Copyright Joshua Gangl. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

#include "JCoreUtils.generated.h"

UCLASS()
class JCORE_API UJCoreUtils : public UBlueprintFunctionLibrary
{
     GENERATED_BODY()

public:
    /**
     *  Gets a FVector that is closest to the TargetLocation from the given InLocations, in world space.
     *
     *  @param TargetLocation  The location to calculate from
     *  @param InLocations  Array of FVectors
     *
     *  @return The FVector in InLocations that is closest to the given TargetLocation
     */
    UFUNCTION(BlueprintCallable, Category="Utility")
    static const FVector GetClosestLocationToPoint(const FVector         &TargetLocation,
                                                   const TArray<FVector> &InLocations);

    /**
     *  Gets a FTransform that is closest to the TargetLocation from the given InTransforms, in world space.
     *
     *  @param TargetLocation  The location to calculate from
     *  @param InTransforms  Array of FTransforms
     *
     *  @return The FTransform in InTransforms that is closest to the given TargetLocation
     */
    static const FTransform GetClosestTransformToPoint(const FVector            &TargetLocation,
                                                       const TArray<FTransform> &InTransforms);

    /**
     *  Gets a SceneComponent that is closest to the TargetLocation from the given InSceneComponents, in world space.
     *
     *  @param TargetLocation  The location to calculate from
     *  @param InSceneComponents  Array of SceneComponents
     *
     *  @return The SceneComponent in InSceneComponents that is closest to the given TargetLocation
     */
    static USceneComponent* GetClosestSceneComponentToPoint(const FVector                  &TargetLocation,
                                                            const TArray<USceneComponent*> &InSceneComponents);

    /**
     *  Gets an array of FHitResult's from under the cursor
     *
     *  @param OutHits  Output Array of FHitResult's.
     *  @param InWorld  The world.
     */
    static void GetHitResultsUnderCursor(TArray<FHitResult>& OutHits,
                                         UWorld*             InWorld);

    /**
     *  Gets an array of FHitResult's from under the cursor
     *
     *  @param OutHits  Output Array of FHitResult's.
     *  @param InWorld  The world.
     *  @param ExtraActorsToIgnore  Array of AActor's to be ignored from trace.
     */
    static void GetHitResultsUnderCursor(TArray<FHitResult>& OutHits,
                                         UWorld*             InWorld,
                                         TArray<AActor*>&    ExtraActorsToIgnore);

    /**
     *  Gets an array of FHitResult's from the player camera's center of view
     *
     *  @param OutHits  Output Array of FHitResult's.
     *  @param InWorld  The world, used for line trace.
     */
    static void GetFirstPersonHitResults(TArray<FHitResult>& OutHits,
                                         UWorld*             InWorld);

    /**
     *  Gets an array of FHitResult's from the player camera's center of view
     *
     *  @param OutHits  Output Array of FHitResult's.
     *  @param InWorld  The world.
     *  @param ExtraActorsToIgnore  Array of AActor's to be ignored from trace.
     */
    static void GetFirstPersonHitResults(TArray<FHitResult>& OutHits,
                                         UWorld*             InWorld,
                                         TArray<AActor*>&    ExtraActorsToIgnore);

    /**
     *  Gets a Subsystem of the given type
     *
     *  @param InWorld  The world.
     *
     *  @return The subsystem of the given type
     */
    template <typename TSubsystemClass>
    static TSubsystemClass* GetSubsystem(UWorld* InWorld)
    {
        UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(InWorld);
        ensure(GameInstance);

        return GameInstance->GetSubsystem<TSubsystemClass>();
    }
};
