// Copyright Joshua Gangl. All Rights Reserved.

#pragma once

#include "Net/UnrealNetwork.h"

#include "BuildingSnapType.h"
#include "Inventory/BuildingRecipeDataAsset.h"

#include "BuildingComponent.generated.h"

class ABuildable;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCompletedBuilding, AActor*, Building);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeletingStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeletingCanceled);

DECLARE_LOG_CATEGORY_CLASS(LogBuildingComponent, Log, All)

UENUM(BlueprintType)
enum class EBuildModeState : uint8
{
    None,
    Initial,
    InProcess
};

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

    UFUNCTION(BlueprintCallable)
    void RotateBuildObject(bool bClockwise);

    UFUNCTION(BlueprintCallable)
    void SetDeleteMode(bool InDeleteMode);

    UFUNCTION(Server, Reliable, BlueprintCallable)
    void ServerSetDeleteMode(bool InDeleteMode);

    UFUNCTION(BlueprintCallable)
    void SetBuildMode(bool InBuildMode);

    UFUNCTION(BlueprintCallable)
    void SetBuildOnWorldGrid(bool InBuildOnWorldGrid);

    UFUNCTION(Server, Reliable, BlueprintCallable)
    void ServerSetBuildableHoveringToDelete(ABuildable* NewBuildable);

    UFUNCTION(Server, Reliable, BlueprintCallable)
    void ServerStartBuilding(TSubclassOf<AActor> ActorToBuild);

    UFUNCTION(Server, Reliable, BlueprintCallable)
    void ServerStartBuildingFromRecipe(UBuildingRecipeDataAsset* RecipeDataAsset);

    UFUNCTION(Server, Reliable, BlueprintCallable)
    void ServerStartBuildPreview(TSubclassOf<AActor> ActorToPreview);

    UFUNCTION(BlueprintCallable)
    void ClearBuildingPreview(bool bDestroy);

    UFUNCTION(Server, Reliable, BlueprintCallable)
    void ServerTryBuild();

    UFUNCTION(BlueprintCallable)
    void StartDeleting();

    UFUNCTION(BlueprintCallable)
    void CancelDeleting();

    UFUNCTION(BlueprintCallable)
    void FinishDeleting();

    UFUNCTION(BlueprintCallable, Server, Reliable)
    void ServerFinishDeleting();

    UFUNCTION(BlueprintCallable)
    AActor* GetPreviouslyCompletedBuilding();

    UFUNCTION(BlueprintCallable)
    void SetActorClassToSpawn(TSubclassOf<AActor> InActorClassToSpawn);

    UFUNCTION(BlueprintCallable)
    void SetCurrentBuildingRecipe(UBuildingRecipeDataAsset* InBuildingRecipe);

    void AddCurrentBuildableOffset(FVector& InLocation) const;

    UFUNCTION(BlueprintCallable)
    void StartCopyBuilding();

    UFUNCTION(BlueprintCallable, BlueprintPure)
    const FTimerHandle& GetDeleteTimerHandle() const;

    UPROPERTY(BlueprintAssignable)
    FOnCompletedBuilding OnCompletedBuilding;

    UPROPERTY(BlueprintAssignable)
    FOnDeletingStarted OnDeletingStarted;

    UPROPERTY(BlueprintAssignable)
    FOnDeletingCanceled OnDeletingCanceled;

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

private:
    float TargetLocationRepTimer;

protected:
    UFUNCTION(Server, Unreliable)
    void ServerSetTargetTransform(const FTransform& TargetTransform);

    virtual void HandleDeleteMode(TArray<FHitResult>& OutHits);

    virtual void HandleBuildingPreview(TArray<FHitResult>& OutHits);

    void IncrementBuildingPreviewSnapIndex();

    const FTransform GetClosestBuildableSnapTransform(const FVector& Center, EBuildingSnapType SnapType) const;

    UFUNCTION(BlueprintCallable)
    bool IsSnapPointAvailable(const FTransform& SnapTransform, const FVector& Extents) const;

    UPROPERTY(BlueprintReadOnly)
    FTransform ClientTargetTransform;

    UPROPERTY(BlueprintReadOnly, Replicated)
    FTransform ServerTargetTransform;

    UPROPERTY(BlueprintReadOnly)
    TSubclassOf<ABuildable> CurrentBuildingClassInPreview;

    UPROPERTY(BlueprintReadOnly)
    UBuildingRecipeDataAsset* CurrentBuildingPreviewRecipe;

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

    UPROPERTY()
    bool bIsSnapping;

    TPair<UBuildingConnectionComponent*, UBuildingConnectionComponent*> BuildingPreviewSnapConnections;

    UPROPERTY(Transient)
    int32 BuildingPreviewSnapIndex;

    UPROPERTY(Transient)
    FRotator BuildingPreviewRotationOffset;

    UPROPERTY()
    float DeleteHoldTime;

    UPROPERTY(EditAnywhere)
    bool bRequireItemsToBuild;

    UPROPERTY(EditAnywhere)
    bool bBuildOnWorldGrid;

private:
    const FVector GetClosestGridLocationFromCamera() const;

    const FVector GetClosestGridLocationToCursor() const;

    const FVector GetGridLocation(const FVector& InLocation) const;

    TSubclassOf<ABuildable> GetCurrentRecipeBuildingClass() const;

    UPROPERTY()
    FTimerHandle DeleteTimerHandle;
};
