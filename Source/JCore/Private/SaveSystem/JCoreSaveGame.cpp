// Copyright Joshua Gangl. All Rights Reserved.

#include "JCore/Public/SaveSystem/JCoreSaveGame.h"

FPlayerSaveData* UJCoreSaveGame::GetPlayerData(APlayerState* PlayerState)
{
    if (PlayerState == nullptr)
    {
        return nullptr;
    }

    // Will not give unique ID while PIE so we skip that step while testing in editor.
    // UObjects don't have access to UWorld, so we grab it via PlayerState instead
    if (PlayerState->GetWorld()->IsPlayInEditor())
    {
        UE_LOG(LogTemp, Warning, TEXT("During PIE we cannot use PlayerID to retrieve Saved Player data. Using first entry in array if available."));

        if (this->SavedPlayers.IsValidIndex(0))
        {
            return &this->SavedPlayers[0];
        }

        // No saved player data available
        return nullptr;
    }

    // Easiest way to deal with the different IDs is as FString (original Steam id is uint64)
    // Keep in mind that GetUniqueId() returns the online id, where GetUniqueID() is a function from UObject (very confusing...)
    FString PlayerID = PlayerState->GetUniqueId().ToString();
    // Iterate the array and match by PlayerID (eg. unique ID provided by Steam)
    return this->SavedPlayers.FindByPredicate([&](const FPlayerSaveData& Data) { return Data.PlayerID == PlayerID; });
}
