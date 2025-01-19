// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

#include "BuildableInterface.h"
#include "SteamFactory/PipeConnectionComponent.h"

#include "Buildable.generated.h"

DECLARE_LOG_CATEGORY_CLASS(LogBuildable, Log, All)

UCLASS(Abstract, Blueprintable)
class JCORE_API ABuildable : public AActor, public IBuildableInterface
{
    GENERATED_BODY()

public:
    ABuildable();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual void OnConstruction(const FTransform& Transform) override;

    UFUNCTION(BlueprintCallable)
    FString GetDisplayName() const;

    UFUNCTION(BlueprintCallable)
    void SetMaterial(UMaterialInterface* NewMaterial);

    UFUNCTION(BlueprintCallable)
    void SetMaterialInvalid();

    UFUNCTION(BlueprintCallable)
    void ResetMaterial();

    const FVector& GetBuildingOffset() const;

    virtual void CompleteBuilding() override;

    virtual void GetPipeSnapTransforms(TArray<FTransform>& OutSnapTransforms) const override;

    void SetIsPreviewing(bool InIsPreviewing);

    void SetCollisionProfileName(const FName InCollisionProfileName);

protected:
    virtual void BeginPlay() override;

    virtual void Destroyed() override;

    bool RemoveGraphNode();

    void UpdatePreviewing();

    UFUNCTION()
    void OnRep_IsPreviewing();

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

    UPROPERTY(ReplicatedUsing=OnRep_IsPreviewing)
    bool bIsPreviewing;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
    UMaterialInterface* ValidPreviewMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
    UMaterialInterface* InvalidPreviewMaterial;

    UPROPERTY(EditAnywhere)
    TArray<UPipeConnectionComponent*> PipeConnectionComponents;
};
