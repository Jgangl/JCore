// Fill out your copyright notice in the Description page of Project Settings.

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