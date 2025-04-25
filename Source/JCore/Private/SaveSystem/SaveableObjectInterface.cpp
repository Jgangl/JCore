// Copyright Joshua Gangl. All Rights Reserved.

#include "JCore/Public/SaveSystem/SaveableObjectInterface.h"

void ISaveableObjectInterface::ActorDestroyed_Implementation(AActor* ActorDestroyed)
{
    if (!ActorDestroyed)
    {
        return;
    }
/*
    ASurvivalCraftGameMode* GM = Cast<ASurvivalCraftGameMode>(UGameplayStatics::GetGameMode(ActorDestroyed));
    if (GM)
    {
        UE_LOG(LogTemp, Warning, TEXT("Calling OnActorDestroyed in Interface"))
        GM->OnActorDestroyed(ActorDestroyed);
    }
*/
}