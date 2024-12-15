// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"

#include "NodeBase.h"

#include "GraphNodeComponent.generated.h"

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JCORE_API UGraphNodeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGraphNodeComponent();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable)
    UNodeBase* GetNode() const;

    void SetNodeClass(TSubclassOf<UNodeBase> InNodeClass);

    void SetNodeLocation(const FVector &InLocation);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    UNodeBase* Node;

    UPROPERTY(VisibleAnywhere, Transient)
    TSubclassOf<UNodeBase> NodeClass;
};
