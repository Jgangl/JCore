// Copyright Joshua Gangl. All Rights Reserved.

#include "JCore/Public/SaveSystem/SaveGameSubsystem.h"

#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

#include "SaveSystem/JCorePlayerState.h"
#include "SaveSystem/JCoreSaveGame.h"
#include "SaveSystem/SaveableObjectInterface.h"

void USaveGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    //const USSaveGameSettings* SGSettings = GetDefault<USSaveGameSettings>();
    // Access defaults from DefaultGame.ini
    this->CurrentSlotName = TEXT("SaveSlot01"); //SGSettings->SaveSlotName;

    // Make sure it's loaded into memory .Get() only resolves if already loaded previously elsewhere in code
    //UDataTable* DummyTable = SGSettings->DummyTablePath.LoadSynchronous();
    //DummyTable->GetAllRows() // We don't need this table for anything, just an content reference example
}

void USaveGameSubsystem::HandleStartingNewPlayer(AController* NewPlayer)
{
    AJCorePlayerState* PS = NewPlayer->GetPlayerState<AJCorePlayerState>();
    if (ensure(PS))
    {
        PS->LoadPlayerState(this->CurrentSaveGame);
    }
}

bool USaveGameSubsystem::OverrideSpawnTransform(AController* NewPlayer)
{
    UE_LOG(LogTemp, Warning, TEXT("Override spawn transform"))
    if (!NewPlayer)
    {
        return false;
    }

    AJCorePlayerState* PS = NewPlayer->GetPlayerState<AJCorePlayerState>();
    if (!PS)
    {
        return false;
    }

    APawn* MyPawn = PS->GetPawn();

    if (!MyPawn)
    {
        return false;
    }

    FPlayerSaveData* FoundData = this->CurrentSaveGame->GetPlayerData(PS);
    if (FoundData && FoundData->bResumeAtTransform)
    {
        UE_LOG(LogTemp, Warning, TEXT("Setting spawn transform"))
        MyPawn->SetActorLocation(FoundData->Location);
        MyPawn->SetActorRotation(FoundData->Rotation);

        // PlayerState owner is a (Player)Controller
        AController* PC = Cast<AController>(PS->GetOwner());
        // Set control rotation to change camera direction, setting Pawn rotation is not enough
        PC->SetControlRotation(FoundData->Rotation);

        return true;
    }


    return false;
}

void USaveGameSubsystem::SetSlotName(FString NewSlotName)
{
    // Ignore empty name
    if (NewSlotName.Len() == 0)
    {
        return;
    }

    CurrentSlotName = NewSlotName;
}

void USaveGameSubsystem::WriteSaveGame()
{
    if (!this->CurrentSaveGame)
    {
        this->CurrentSaveGame = Cast<UJCoreSaveGame>(UGameplayStatics::CreateSaveGameObject(UJCoreSaveGame::StaticClass()));

        UE_LOG(LogTemp, Warning, TEXT("Created New SaveGame Data."));
    }

    // Clear arrays, may contain data from previously loaded SaveGame
    this->CurrentSaveGame->SavedPlayers.Empty();
    this->CurrentSaveGame->SavedActors.Empty();
    //CurrentSaveGame->DeletedActors.Empty();

    AGameStateBase* GameState = GetWorld()->GetGameState();
    if (GameState == nullptr)
    {
        // Warn about failure to save?
        return;
    }

    // Iterate all player states, we don't have proper ID to match yet (requires Steam or EOS)
    for (int32 i = 0; i < GameState->PlayerArray.Num(); i++)
    {
        AJCorePlayerState* PS = Cast<AJCorePlayerState>(GameState->PlayerArray[i]);
        if (PS)
        {
            UE_LOG(LogTemp, Warning, TEXT("Save player state"));

            PS->SavePlayerState(CurrentSaveGame);
            break; // single player only at this point
        }
    }

    // Iterate the entire world of actors
    for (FActorIterator It(GetWorld()); It; ++It)
    {
        AActor* Actor = *It;
        // Only interested in our 'saveable actors', skip actors that are being destroyed
        if (!IsValid(Actor) || !Actor->Implements<USaveableObjectInterface>())
        {
            continue;
        }

        UE_LOG(LogTemp, Warning, TEXT("Saving object: %s"), *Actor->GetName());

        FActorSaveData ActorData;
        ActorData.ActorName = Actor->GetFName();
        ActorData.ActorClass = Actor->GetClass();
        ActorData.Transform = Actor->GetActorTransform();

        // Pass the array to fill with data from Actor
        FMemoryWriter MemWriter(ActorData.ByteData);

        FObjectAndNameAsStringProxyArchive Ar(MemWriter, true);
        // Find only variables with UPROPERTY(SaveGame)
        Ar.ArIsSaveGame = true;
        // Converts Actor's SaveGame UPROPERTIES into binary array
        Actor->Serialize(Ar);

        this->CurrentSaveGame->SavedActors.Add(ActorData);
    }

    UGameplayStatics::SaveGameToSlot(this->CurrentSaveGame, this->CurrentSlotName, 0);

    this->OnSaveGameWritten.Broadcast(this->CurrentSaveGame);
}

void USaveGameSubsystem::LoadSaveGame(FString InSlotName /*= ""*/)
{
    // Update slot name first if specified, otherwise keeps default name
    SetSlotName(InSlotName);

    if (UGameplayStatics::DoesSaveGameExist(CurrentSlotName, 0))
    {
        UE_LOG(LogTemp, Warning, TEXT("Found save game data"));

        this->CurrentSaveGame = Cast<UJCoreSaveGame>(UGameplayStatics::LoadGameFromSlot(CurrentSlotName, 0));
        if (this->CurrentSaveGame == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to load SaveGame Data"))
            return;
        }

        UE_LOG(LogTemp, Log, TEXT("Loaded SaveGame Data."));

        TArray<FActorSaveData> LoadedActors;

        // Iterate the entire world of actors
        for (FActorIterator It(GetWorld()); It; ++It)
        {
            AActor* Actor = *It;
            UE_LOG(LogTemp, Warning, TEXT("Actor: %s"), *Actor->GetName());
            // Only interested in our 'gameplay actors'
            if (!Actor->Implements<USaveableObjectInterface>())
            {
                continue;
            }

            UE_LOG(LogTemp, Warning, TEXT("Actor implements Saveable object interface"));

            bool bLoaded = false;

            for (FActorSaveData ActorData : this->CurrentSaveGame->SavedActors)
            {
                if (ActorData.ActorName == Actor->GetFName())
                {
                    Actor->SetActorTransform(ActorData.Transform);

                    FMemoryReader MemReader(ActorData.ByteData);

                    FObjectAndNameAsStringProxyArchive Ar(MemReader, true);
                    Ar.ArIsSaveGame = true;
                    // Convert binary array back into actor's variables
                    Actor->Serialize(Ar);

                    ISaveableObjectInterface::Execute_OnActorLoaded(Actor);

                    bLoaded = true;

                    // Add actor data to list of loaded actors
                    UE_LOG(LogTemp, Warning, TEXT("Adding loaded actor"));
                    LoadedActors.AddUnique(ActorData);

                    break;
                }
            }

            FActorSaveData SaveData;
            SaveData.ActorName = Actor->GetFName();
            UE_LOG(LogTemp, Warning, TEXT("Actor: %s"), *Actor->GetFName().ToString())
            if (this->CurrentSaveGame->DeletedActors.Contains(SaveData))
            {

                //Actor->Destroy();
            }


            // Need to delete actors that were originally in map

            // Delete saveable actors that were deleted at runtime
            if (!bLoaded)
            {
                //Actor->Destroy();
            }
        }

        for (FActorSaveData SaveData : this->CurrentSaveGame->DeletedActors)
        {
            //LOG_WARN("Deleted actor: %s", *SaveData.ActorName.ToString())
        }

        for (FActorSaveData ActorData : LoadedActors)
        {
            UE_LOG(LogTemp, Warning, TEXT("Loaded actor: %s"), *ActorData.ActorName.ToString())
        }

        for (FActorSaveData ActorData : this->CurrentSaveGame->SavedActors)
        {
            AActor* SpawnedActor = GetWorld()->SpawnActor(ActorData.ActorClass, &ActorData.Transform);

            if (!SpawnedActor)
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to spawn actor {%s} from save"), *ActorData.ActorName.ToString());
                continue;
            }

            FMemoryReader MemReader(ActorData.ByteData);

            FObjectAndNameAsStringProxyArchive Ar(MemReader, true);
            Ar.ArIsSaveGame = true;
            // Convert binary array back into actor's variables
            SpawnedActor->Serialize(Ar);

            ISaveableObjectInterface::Execute_OnActorLoaded(SpawnedActor);
        }

        this->OnSaveGameLoaded.Broadcast(this->CurrentSaveGame);
    }
    else
    {
        this->CurrentSaveGame = Cast<UJCoreSaveGame>(UGameplayStatics::CreateSaveGameObject(UJCoreSaveGame::StaticClass()));

        UE_LOG(LogTemp, Warning, TEXT("Created New SaveGame Data."));
    }
}

void USaveGameSubsystem::OnActorDestroyed(AActor* ActorDestroyed)
{
    if (!this->CurrentSaveGame)
    {
        return;
    }

    FActorSaveData ActorData;
    ActorData.ActorName = ActorDestroyed->GetFName();

    UE_LOG(LogTemp, Warning, TEXT("Adding deleted actor: %s"), *ActorDestroyed->GetName());

    this->CurrentSaveGame->DeletedActors.AddUnique(ActorData);
}
