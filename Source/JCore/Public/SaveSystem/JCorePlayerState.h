// Copyright Joshua Gangl. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"

#include "JCorePlayerState.generated.h"

class UJCoreSaveGame;

/**
 *
 */
UCLASS()
class JCORE_API AJCorePlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent)
    void SavePlayerState(UJCoreSaveGame* SaveObject);

    UFUNCTION(BlueprintNativeEvent)
    void LoadPlayerState(UJCoreSaveGame* SaveObject);

};
