#include "AI/BTTask_ClearFocus.h"

#include "AIController.h"

UBTTask_ClearFocus::UBTTask_ClearFocus(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    this->NodeName = "Clear Focus";
}

EBTNodeResult::Type UBTTask_ClearFocus::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* MyAIController = Cast<AAIController>(OwnerComp.GetOwner());

    if (!MyAIController)
    {
        UE_LOG(LogTemp, Error, TEXT("MyAIController is nullptr"));
        return EBTNodeResult::Failed;
    }

    MyAIController->ClearFocus(EAIFocusPriority::Gameplay);

    return EBTNodeResult::Succeeded;
}
