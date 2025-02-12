#pragma once

#include "SaveableObjectInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class USaveableObjectInterface : public UInterface
{
    GENERATED_BODY()
};

class JCORE_API ISaveableObjectInterface
{
    GENERATED_BODY()

public:

    /* Called after the Actor state was restored from a SaveGame file. */
    UFUNCTION(BlueprintNativeEvent)
    void OnActorLoaded();

    UFUNCTION(BlueprintNativeEvent)
    void ActorDestroyed(AActor* ActorDestroyed);

    UFUNCTION()
    virtual void ActorDestroyed_Implementation(AActor* ActorDestroyed);
};