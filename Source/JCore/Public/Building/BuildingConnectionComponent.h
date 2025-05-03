// Copyright Joshua Gangl. All Rights Reserved.

#pragma once

#include "Components/ArrowComponent.h"

#include "Graph/NodeBase.h"

#include "BuildingConnectionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnConnectionConnected, UBuildingConnectionComponent*, OwnConnectedConnection, UBuildingConnectionComponent*, OtherConnection);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConnectionDisconnected, UBuildingConnectionComponent*, OwnDisconnectedConnection);

DECLARE_LOG_CATEGORY_CLASS(LogBuildingConnectionComponent, Log, All);

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JCORE_API UBuildingConnectionComponent: public UArrowComponent
{
    GENERATED_BODY()

public:
    UBuildingConnectionComponent();

    virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

    UFUNCTION(BlueprintCallable, BlueprintPure)
    UBuildingConnectionComponent* GetConnectedComponent();

    UFUNCTION(BlueprintCallable)
    void SetConnectedComponent(UBuildingConnectionComponent* InConnectionComponent);

    UFUNCTION(BlueprintCallable)
    bool IsConnected();

    void DisconnectConnections();

    const FTransform& GetSnapTransform() const;

    UFUNCTION(BlueprintCallable)
    bool IsInput();

    UFUNCTION(BlueprintCallable)
    void SetIsInput(bool InbIsInput);

    UPROPERTY(BlueprintAssignable)
    FOnConnectionConnected OnConnectionConnected;

    UPROPERTY(BlueprintAssignable)
    FOnConnectionDisconnected OnConnectionDisconnected;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bIsInput;

private:
    UPROPERTY(EditAnywhere)
    UBuildingConnectionComponent* ConnectedComponent;
};
