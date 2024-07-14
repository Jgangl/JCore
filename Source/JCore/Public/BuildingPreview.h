// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "BuildingPreview.generated.h"

/**
 *
 */

DECLARE_LOG_CATEGORY_CLASS(LogBuildingPreview, Log, All);

UCLASS(BlueprintType)
class JCORE_API ABuildingPreview : public AActor
{
public:
    ABuildingPreview();
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable)
    void UpdatePlacementValid();

    UFUNCTION(BlueprintCallable)
    void SetMaterials(UStaticMeshComponent* StaticMeshComponent, TArray<UMaterialInterface*> NewMaterials);

    UFUNCTION(BlueprintCallable)
    void SetValidPreviewMaterial(UMaterialInterface* NewValidPreviewMaterial);

    UFUNCTION(BlueprintCallable)
    void SetInvalidPreviewMaterial(UMaterialInterface* NewInvalidPreviewMaterial);

    UFUNCTION(BlueprintCallable)
    void AddStaticMesh(UStaticMeshComponent* ReferenceMeshComponent);

    UFUNCTION(BlueprintCallable)
    FORCEINLINE bool IsPlacementValid() { return this->bPlacementValid; }

private:
    GENERATED_BODY()

protected:
    UPROPERTY()
    AActor* CurrentPreviewActor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
    UMaterialInterface* ValidPreviewMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
    UMaterialInterface* InvalidPreviewMaterial;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_bPlacementValid)
    bool bPlacementValid;

    UFUNCTION()
    void OnRep_bPlacementValid();

    UFUNCTION()
    void OnRep_StaticMeshComponents();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_StaticMeshComponents)
    TArray<UStaticMeshComponent*> StaticMeshComponents;

    void SetMaterial(UMaterialInterface* NewMaterial);

    /** Used as a root component to be able to offset static mesh components */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    USceneComponent* RootSceneComponent;
};
