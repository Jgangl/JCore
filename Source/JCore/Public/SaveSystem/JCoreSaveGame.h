// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SaveGame.h"

#include "JCoreSaveGame.generated.h"

USTRUCT()
struct FActorSaveData
{
    GENERATED_BODY()

    /* Identifier for which Actor this belongs to */
    UPROPERTY()
    FName ActorName;

    /* UClass of the saved actor*/
    UPROPERTY()
    UClass* ActorClass;

    /* For movable Actors, keep location,rotation,scale. */
    UPROPERTY()
    FTransform Transform;

    /* Contains all 'SaveGame' marked variables of the Actor */
    UPROPERTY()
    TArray<uint8> ByteData;

    FORCEINLINE bool operator==(const FActorSaveData &Other) const
    {
        return ActorName == Other.ActorName;
    }
};

USTRUCT()
struct FPlayerSaveData
{
    GENERATED_BODY()

    FPlayerSaveData()
    {
        Location = FVector::ZeroVector;
        Rotation = FRotator::ZeroRotator;
        bResumeAtTransform = true;
    }

    /* Player Id defined by the online sub system (such as Steam) converted to FString for simplicity  */
    UPROPERTY()
    FString PlayerID;

    /* Location if player was alive during save */
    UPROPERTY()
    FVector Location;

    /* Orientation if player was alive during save */
    UPROPERTY()
    FRotator Rotation;

    /* We don't always want to restore location, and may just resume player at specific respawn point in world. */
    UPROPERTY()
    bool bResumeAtTransform;
};

/**
 *
 */
UCLASS()
class JCORE_API UJCoreSaveGame : public USaveGame
{
    GENERATED_BODY()

public:

    UPROPERTY()
    TArray<FPlayerSaveData> SavedPlayers;

    /* Actors stored from a level (currently does not support a specific level and just assumes the demo map) */
    UPROPERTY()
    TArray<FActorSaveData> SavedActors;

    UPROPERTY()
    TArray<FActorSaveData> DeletedActors;

    FPlayerSaveData* GetPlayerData(APlayerState* PlayerState);
};
