// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "InteractableComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteract, APawn*, Interactor);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class JCORE_API UInteractableComponent : public UWidgetComponent
{
public:
    UPROPERTY(BlueprintAssignable)
    FOnInteract OnInteract;

private:
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UInteractableComponent();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;
    virtual void BeginPlay() override;
    virtual void OnComponentCreated() override;
    virtual void OnRegister() override;

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

    UFUNCTION(BlueprintCallable)
    void SetDisplayText(const FString& InText);

    void SetInteractSound(USoundBase* InteractSound);

    UFUNCTION(BlueprintCallable, BlueprintPure)
    const FString& GetDisplayText();

    UFUNCTION(BlueprintCallable, BlueprintPure)
    USoundBase* GetInteractSound();

protected:
    UFUNCTION(BlueprintCallable, BlueprintPure)
    FBoxSphereBounds GetStaticMeshComponentsBounds();

    UPROPERTY()
    TArray<UMeshComponent*> MeshComponents;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bCanInteract;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ZOffset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString DisplayText;

    // TODO: Data driven format for interactable object data

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
    USoundBase* InteractSound;
};
