// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "InteractableComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteract, APawn*, Interactor);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class JCORE_API UInteractableComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UInteractableComponent();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(BlueprintAssignable)
    FOnInteract OnInteract;

    UFUNCTION(BlueprintCallable)
    void BeginLookAt();

    UFUNCTION(BlueprintCallable)
    void EndLookAt();

    UFUNCTION(BlueprintCallable)
    void SetCanInteract(bool CanInteract);

    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool CanInteract();

    UFUNCTION(BlueprintCallable)
    virtual void BeginFocus();

    UFUNCTION(BlueprintCallable)
    virtual void EndFocus();

    UFUNCTION(BlueprintCallable)
    virtual void Interact(APawn* Interactor);

protected:
    // Called when the game starts
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bCanInteract;
};
