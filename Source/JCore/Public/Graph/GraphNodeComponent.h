// Copyright Joshua Gangl. All Rights Reserved.

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

    virtual void OnRegister() override;

    UFUNCTION(BlueprintCallable)
    UNodeBase* GetNode() const;

    UFUNCTION(BlueprintCallable)
    void SetNodeClass(TSubclassOf<UNodeBase> InNodeClass);

    UFUNCTION(BlueprintCallable)
    void SetNodeLocation(const FVector &InLocation);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    UNodeBase* Node;

    UPROPERTY(VisibleAnywhere, Transient)
    TSubclassOf<UNodeBase> NodeClass;
};
