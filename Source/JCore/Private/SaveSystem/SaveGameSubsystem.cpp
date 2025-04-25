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

    this->CurrentSlotName = TEXT("SaveSlot_1");
}

void USaveGameSubsystem::HandleStartingNewPlayer(AController* NewPlayer)
{
    AJCorePlayerState* PlayerState = NewPlayer->GetPlayerState<AJCorePlayerState>();
    if (ensure(PlayerState))
    {
        PlayerState->LoadPlayerState(this->CurrentSaveGame);
    }
}

bool USaveGameSubsystem::OverrideSpawnTransform(AController* NewPlayer)
{
    UE_LOG(LogTemp, Warning, TEXT("Override spawn transform"))
    if (!NewPlayer)
    {
        return false;
    }

    AJCorePlayerState* PlayerState = NewPlayer->GetPlayerState<AJCorePlayerState>();
    if (!PlayerState)
    {
        return false;
    }

    APawn* MyPawn = PlayerState->GetPawn();

    if (!MyPawn)
    {
        return false;
    }

    FPlayerSaveData* FoundData = this->CurrentSaveGame->GetPlayerData(PlayerState);
    if (FoundData && FoundData->bResumeAtTransform)
    {
        UE_LOG(LogTemp, Warning, TEXT("Setting spawn transform"))
        MyPawn->SetActorLocation(FoundData->Location);
        MyPawn->SetActorRotation(FoundData->Rotation);

        AController* PC = Cast<AController>(PlayerState->GetOwner());
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
    }

    this->CurrentSaveGame->SavedPlayers.Empty();
    this->CurrentSaveGame->SavedActors.Empty();

    AGameStateBase* GameState = GetWorld()->GetGameState();
    if (GameState == nullptr)
    {
        return;
    }

    // Iterate all player states, we don't have proper ID to match yet (requires Steam or EOS)
    for (int32 i = 0; i < GameState->PlayerArray.Num(); i++)
    {
        AJCorePlayerState* PlayerState = Cast<AJCorePlayerState>(GameState->PlayerArray[i]);
        if (PlayerState)
        {
            PlayerState->SavePlayerState(CurrentSaveGame);
            break;
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
        // Find only variables with SaveGame uproperty specifier
        Ar.ArIsSaveGame = true;
        // Converts Actor's SaveGame UPROPERTIES into binary array
        Actor->Serialize(Ar);

        this->CurrentSaveGame->SavedActors.Add(ActorData);
    }

    UGameplayStatics::SaveGameToSlot(this->CurrentSaveGame, this->CurrentSlotName, 0);

    this->OnSaveGameWritten.Broadcast(this->CurrentSaveGame);
}

void USaveGameSubsystem::LoadSaveGame(FString InSlotName)
{
    // Update slot name first if specified, otherwise keeps default name
    this->SetSlotName(InSlotName);

    if (UGameplayStatics::DoesSaveGameExist(CurrentSlotName, 0))
    {
        this->CurrentSaveGame = Cast<UJCoreSaveGame>(UGameplayStatics::LoadGameFromSlot(this->CurrentSlotName, 0));
        if (this->CurrentSaveGame == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to load SaveGame Data"))
            return;
        }

        TArray<FActorSaveData> LoadedActors;

        // Iterate the entire world of actors
        for (FActorIterator It(GetWorld()); It; ++It)
        {
            AActor* Actor = *It;

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
                    LoadedActors.AddUnique(ActorData);

                    break;
                }
            }
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

    this->CurrentSaveGame->DeletedActors.AddUnique(ActorData);
}
