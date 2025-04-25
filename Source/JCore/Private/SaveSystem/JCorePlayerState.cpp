// Copyright Joshua Gangl. All Rights Reserved.

#include "JCore/Public/SaveSystem/JCorePlayerState.h"
#include "JCore/Public/SaveSystem/JCoreSaveGame.h"

void AJCorePlayerState::SavePlayerState_Implementation(UJCoreSaveGame* SaveObject)
{
    if (SaveObject)
    {
        FPlayerSaveData SaveData;
        SaveData.PlayerID = GetUniqueId().ToString();

        if (APawn* MyPawn = GetPawn())
        {
            SaveData.Location = MyPawn->GetActorLocation();
            SaveData.Rotation = MyPawn->GetActorRotation();
            SaveData.bResumeAtTransform = true;
        }

        SaveObject->SavedPlayers.Add(SaveData);
    }
}

void AJCorePlayerState::LoadPlayerState_Implementation(UJCoreSaveGame* SaveObject)
{
}
