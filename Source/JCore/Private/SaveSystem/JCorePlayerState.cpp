// Copyright Joshua Gangl. All Rights Reserved.

#include "JCore/Public/SaveSystem/JCorePlayerState.h"
#include "JCore/Public/SaveSystem/JCoreSaveGame.h"

void AJCorePlayerState::SavePlayerState_Implementation(UJCoreSaveGame* SaveObject)
{
    if (SaveObject)
    {
        // Gather all relevant data for player
        FPlayerSaveData SaveData;
        // Stored as FString for simplicity (original Steam ID is uint64)
        SaveData.PlayerID = GetUniqueId().ToString();

        // May not be alive while we save
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
    if (SaveObject)
    {
        FPlayerSaveData* FoundData = SaveObject->GetPlayerData(this);
        if (FoundData)
        {

        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Could not find SaveGame data for player id: {playerid}."), GetPlayerId());
        }
    }
}
