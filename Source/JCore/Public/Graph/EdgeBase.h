
#pragma once
#include "NodeBase.h"

#include "EdgeBase.generated.h"

UCLASS(BlueprintType)
class JCORE_API UEdgeBase : public UObject
{
    GENERATED_BODY()
public:
    UEdgeBase();

    UPROPERTY()
    UNodeBase* Source;

    UPROPERTY()
    UNodeBase* Destination;
};
