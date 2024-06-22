#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Interface.h"

#include "InteractableComponentInterface.generated.h"

class UInteractableComponent;

/** Interface for actors that expose access to an interactable component */
UINTERFACE(MinimalAPI)
class UInteractableComponentInterface : public UInterface
{
    GENERATED_UINTERFACE_BODY()
};

class JCORE_API IInteractableComponentInterface
{
    GENERATED_IINTERFACE_BODY()

    /** Returns the interactable component to use for this actor. */
    virtual UInteractableComponent* GetInteractableComponent() const = 0;
};
