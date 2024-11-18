// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GraphBase.h"

#include "SceneManagement.h"

#include "GraphDebugger.generated.h"

UCLASS(BlueprintType)
class JCORE_API AGraphDebugger : public AActor
{
    GENERATED_BODY()

public:
    AGraphDebugger();

    virtual void Tick(float DeltaSeconds) override;

    void DrawGraph();

    UFUNCTION(BlueprintCallable)
    void SetGraph(UGraphBase* InGraph);



protected:
    UPROPERTY(EditAnywhere)
    UGraphBase* Graph;

    UPROPERTY(EditAnywhere)
    bool bEnabled;
};
