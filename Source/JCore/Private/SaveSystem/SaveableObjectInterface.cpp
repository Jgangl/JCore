// Copyright Joshua Gangl. All Rights Reserved.

#include "JCore/Public/SaveSystem/SaveableObjectInterface.h"

void ISaveableObjectInterface::ActorDestroyed_Implementation(AActor* ActorDestroyed)
{
    if (!ActorDestroyed)
    {
        return;
    }
}