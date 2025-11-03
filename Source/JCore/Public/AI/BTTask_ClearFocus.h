// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Runtime/AIModule/Classes/BehaviorTree/BTTaskNode.h"
#include "BTTask_ClearFocus.generated.h"

/**
 * Clear Focus task node.
 * A task node that calls ClearFocus() on this Pawn's AIController when executed.
 */
UCLASS()
class JCORE_API UBTTask_ClearFocus : public UBTTaskNode
{
    GENERATED_UCLASS_BODY()

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
