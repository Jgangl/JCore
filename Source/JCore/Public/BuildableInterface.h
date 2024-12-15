#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Interface.h"
#include "Containers/Array.h"

#include "BuildableInterface.generated.h"

/** Interface for actors that expose access to an interactable component */
UINTERFACE(MinimalAPI)
class UBuildableInterface : public UInterface
{
    GENERATED_UINTERFACE_BODY()
};

class JCORE_API IBuildableInterface
{
    GENERATED_IINTERFACE_BODY()

    /** Called by the BuildingComponent when this buildable is finished building */
    virtual void CompleteBuilding() = 0;

    virtual void GetPipeSnapTransforms(TArray<FTransform>& OutSnapTransforms) const = 0;
};
