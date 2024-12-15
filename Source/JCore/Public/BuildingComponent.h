// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Net/UnrealNetwork.h"

#include "BuildingPreview.h"

#include "BuildingComponent.generated.h"

class ABuildable;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCompletedBuilding, AActor*, Building);

DECLARE_LOG_CATEGORY_CLASS(LogBuildingComponent, Log, All)

/**
 *
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JCORE_API UBuildingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UBuildingComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool IsTryingToPlaceBuilding() const;

    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool IsInDeleteMode() const;

    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool IsInBuildMode() const;

    UFUNCTION(Server, Reliable, BlueprintCallable)
    void ServerCancelBuilding();

    UFUNCTION(Server, Unreliable, BlueprintCallable)
    void ServerRotateBuildObject(const FRotator &DeltaRotation);

    UFUNCTION(BlueprintCallable)
    void RotateBuildObject(bool bClockwise);

    UFUNCTION(BlueprintCallable)
    void SetDeleteMode(bool InDeleteMode);

    UFUNCTION(Server, Reliable, BlueprintCallable)
    void ServerSetDeleteMode(bool InDeleteMode);

    UFUNCTION(BlueprintCallable)
    void SetBuildMode(bool InBuildMode);

    UFUNCTION(Server, Reliable, BlueprintCallable)
    void ServerSetBuildableHoveringToDelete(ABuildable* NewBuildable);

    UFUNCTION(Server, Reliable, BlueprintCallable)
    void ServerStartBuilding(TSubclassOf<AActor> ActorToBuild);

    UFUNCTION(Server, Reliable, BlueprintCallable)
    void ServerStartBuildPreview(TSubclassOf<AActor> ActorToPreview);

    UFUNCTION(BlueprintCallable)
    void ClearBuildingPreview(bool bDestroy);

    UFUNCTION(Server, Reliable, BlueprintCallable)
    void ServerTryBuild();

    UFUNCTION(BlueprintCallable)
    bool TryDelete();

    UFUNCTION(BlueprintCallable)
    AActor* GetPreviouslyCompletedBuilding();

    UFUNCTION(BlueprintCallable)
    void IncrementSelectedActorToSpawn();

    UFUNCTION(BlueprintCallable)
    void SetActorClassToSpawn(TSubclassOf<AActor> InActorClassToSpawn);

    void AddCurrentBuildableOffset(FVector& InLocation) const;

    UPROPERTY(BlueprintAssignable)
    FOnCompletedBuilding OnCompletedBuilding;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<TSubclassOf<AActor>> ActorClassesToSpawn;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TSubclassOf<AActor> ActorClassToSpawn;

    UPROPERTY(VisibleAnywhere)
    uint32 SelectedActorToSpawnIndex;

    UPROPERTY(EditAnywhere)
    float GridTileSizeX;

    UPROPERTY(EditAnywhere)
    float GridTileSizeY;

    UPROPERTY(EditAnywhere)
    float GridTileSizeZ;

    UPROPERTY(EditAnywhere)
    float GridOffsetX;

    UPROPERTY(EditAnywhere)
    float GridOffsetY;

    UPROPERTY(EditAnywhere)
    float GridOffsetZ;

    UPROPERTY(EditAnywhere)
    bool bDebug;

    UPROPERTY(BlueprintReadWrite, Replicated)
    float TargetLocationRepFrequency;

    UPROPERTY(EditAnywhere)
    bool bFirstPersonInteraction;

    // TODO: ADD A SETTING FOR WHETHER OR NOT TO USE OVERLAPS TO CHECK FOR PLACEMENT VALIDITY (WE DON'T NEED OVERLAPS WHEN USING GRID)

private:
    float TargetLocationRepTimer;

protected:
    UFUNCTION(Server, Unreliable)
    void ServerSetTargetTransform(const FTransform& TargetTransform);

    virtual void HandleDeleteMode(TArray<FHitResult>& OutHits);

    virtual void HandleBuildingPreview(TArray<FHitResult>& OutHits);

    UPROPERTY(BlueprintReadOnly)
    FTransform ClientTargetTransform;

    UPROPERTY(BlueprintReadOnly, Replicated)
    FTransform ServerTargetTransform;

    UPROPERTY(BlueprintReadOnly)
    TSubclassOf<ABuildable> CurrentBuildingClassInPreview;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
    ABuildable* CurrentBuildingPreview;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RotationGridSnapValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float BuildingPreviewInterpSpeed;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
    bool bInDeleteMode;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
    bool bInBuildMode;

    UPROPERTY(Replicated)
    ABuildable* BuildableHoveringToDelete;

    UPROPERTY()
    AActor* LocalHoveringBuildableActor;

    UPROPERTY()
    AActor* PreviouslyCompletedBuilding;

private:
    void GetHitResultsUnderCursor(TArray<FHitResult>& OutHits) const;

    void GetFirstPersonHitResults(TArray<FHitResult>& OutHits) const;

    const FVector GetClosestGridLocationToCursor() const;

    const FVector GetGridLocation(const FVector& InLocation) const;

    const FTransform GetClosestConnectionTransform(const FVector &Location, const TArray<FTransform> &ConnectionTransforms);
};
