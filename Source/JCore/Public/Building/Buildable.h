// Copyright Joshua Gangl. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"

#include "SaveSystem/SaveableObjectInterface.h"

#include "BuildableInterface.h"
#include "BuildingSnapType.h"
#include "Building/BuildingConnectionComponent.h"

#include "Buildable.generated.h"

DECLARE_LOG_CATEGORY_CLASS(LogBuildable, Log, All)

UCLASS(Abstract, Blueprintable)
class JCORE_API ABuildable : public AActor, public IBuildableInterface, public ISaveableObjectInterface
{
    GENERATED_BODY()

public:
    ABuildable();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual void OnConstruction(const FTransform& Transform) override;

    virtual void Tick(float DeltaSeconds) override;

    virtual void PostInitializeComponents() override;

    UFUNCTION(BlueprintCallable)
    FString GetDisplayName() const;

    UFUNCTION(BlueprintCallable)
    UStaticMeshComponent* GetStaticMeshComponent() const;

    UFUNCTION(BlueprintCallable)
    const FVector& GetSize() const;

    UFUNCTION(BlueprintCallable)
    const FVector& GetOriginOffset() const;

    UFUNCTION(BlueprintCallable)
    void SetMaterial(UMaterialInterface* NewMaterial);

    UFUNCTION(BlueprintCallable)
    void SetMaterialInvalid();

    UFUNCTION(BlueprintCallable)
    void ResetMaterial();

    UFUNCTION(BlueprintCallable, NetMulticast, Reliable)
    void MulticastSetMaterialInvalid();

    UFUNCTION(BlueprintCallable, NetMulticast, Reliable)
    void MulticastResetMaterial();

    UFUNCTION(BlueprintCallable)
    bool RequiresOverlapCheck();

    const FVector& GetBuildingOffset() const;

    virtual void CompleteBuilding(UBuildingConnectionComponent* FromSnapConnection, UBuildingConnectionComponent* ToSnapConnection) override;

    virtual void GetOpenConnectionComponents(TArray<UBuildingConnectionComponent*>& OutConnectionComponents) const override;

    virtual void GetConnectionSnapTransforms(TArray<FTransform>& OutSnapTransforms) const override;

    virtual UBuildingConnectionComponent* GetClosestConnectionToLocation(const FVector& InLocation) const override;

    virtual bool HasOpenConnections() const override;

    void GetNeighborSnapLocations(TArray<FVector>& OutSnapLocations);

    void SetIsPreviewing(bool InIsPreviewing);

    void SetCollisionProfileName(const FName InCollisionProfileName);

    bool IsPlacementValid() const;

    UFUNCTION(BlueprintCallable)
    EBuildingSnapType GetSnapType() const;

    UFUNCTION(BlueprintCallable)
    void SetSnapTransformsOfType(EBuildingSnapType InSnapType, const TArray<FTransform>& InSnapTransforms);

    UFUNCTION(BlueprintCallable)
    virtual void GetSnapTransformsOfType(EBuildingSnapType InSnapType, TArray<FTransform>& OutSnapTransforms) const;

    virtual void OnActorLoaded_Implementation() override;

protected:
    virtual void BeginPlay() override;

    virtual void Destroyed() override;

    bool RemoveGraphNode();

    void UpdatePreviewing();

    UFUNCTION()
    virtual void OnConnectionConnected(UBuildingConnectionComponent* FromConnectedConnection,
                                       UBuildingConnectionComponent* ToConnectedConnection);

    UFUNCTION()
    virtual void OnConnectionDisconnected(UBuildingConnectionComponent* DisconnectedConnection);

    UFUNCTION()
    void OnRep_IsPreviewing();

    UFUNCTION(BlueprintCallable)
    void UpdatePlacementValidity();

    UFUNCTION()
    void OnRep_bPlacementValid();

    UPROPERTY(EditAnywhere)
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    UStaticMeshComponent* StaticMeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<UMeshComponent*> MeshComponents;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<UMaterialInterface*> OriginalMaterials;

    UPROPERTY(EditAnywhere)
    FVector BuildingOffset;

    UPROPERTY(EditAnywhere)
    FVector OriginOffset;

    UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_IsPreviewing)
    bool bIsPreviewing;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_bPlacementValid)
    bool bValidPlacement;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
    UMaterialInterface* ValidPreviewMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
    UMaterialInterface* InvalidPreviewMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRequireOverlapCheck;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Size;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EBuildingSnapType SnapType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Debug")
    bool bDebug;

    TMap<EBuildingSnapType, TArray<FTransform>> SnapTransforms;

    UPROPERTY(EditAnywhere)
    TArray<UBuildingConnectionComponent*> BuildingConnectionComponents;
};
