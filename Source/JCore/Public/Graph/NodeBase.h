
#pragma once

#include "NodeBase.generated.h"

UCLASS(BlueprintType)
class JCORE_API UNodeBase : public UObject
{
    GENERATED_BODY()
public:
    UNodeBase();

    UFUNCTION(BlueprintCallable)
    void SetLocation(const FVector &InLocation);

    UFUNCTION(BlueprintCallable)
    const FVector& GetLocation() const;

    UFUNCTION()
    virtual void PostEdgesAdded();

protected:
    int32 MaxConnections;

    UPROPERTY()
    FVector Location;
};
