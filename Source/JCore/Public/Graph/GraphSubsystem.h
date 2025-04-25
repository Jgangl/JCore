// Copyright Joshua Gangl. All Rights Reserved.

#pragma once

#include "GraphBase.h"

#include "GraphSubsystem.generated.h"

UCLASS()
class JCORE_API UGraphSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UGraphSubsystem();

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable)
    UGraphBase* CreateGraph();

    UFUNCTION(BlueprintCallable)
    UGraphBase* GetGraph();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<UGraphBase*> Graphs;
};
