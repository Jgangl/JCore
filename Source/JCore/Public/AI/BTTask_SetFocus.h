// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "BehaviorTree/ValueOrBBKey.h"
#include "Runtime/AIModule/Classes/BehaviorTree/BTTaskNode.h"
#include "BTTask_SetFocus.generated.h"

UENUM(BlueprintType)
enum class EFocusType : uint8
{
    Actor,
    Location
};

/**
 * Set Focus task node.
 * A task node that calls SetFocus() or SetFocalPoint() on this Pawn's AIController when executed.
 */
UCLASS()
class JCORE_API UBTTask_SetFocus : public UBTTaskNode
{
    GENERATED_UCLASS_BODY()

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

    virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

    /** Type of focus to use, actor or location. */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    EFocusType FocusType;

    /** Blackboard key of the Actor to focus. */
    UPROPERTY(EditAnywhere, Category = "Blackboard", meta = (EditCondition = "FocusType == EFocusType::Actor", EditConditionHides))
    FBlackboardKeySelector FocusTarget;

    /** Value or Blackboard key of the Location to focus. */
    UPROPERTY(EditAnywhere, Category = "Blackboard", meta = (EditCondition = "FocusType == EFocusType::Location", EditConditionHides))
    FValueOrBBKey_Vector FocalPointTarget;
};
