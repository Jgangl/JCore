#include "AI/BTTask_SetFocus.h"

#include "AIController.h"
#include "BehaviorTree/BTFunctionLibrary.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"

UBTTask_SetFocus::UBTTask_SetFocus(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    this->NodeName  = "Set Focus";
    this->FocusType = EFocusType::Actor;
}

EBTNodeResult::Type UBTTask_SetFocus::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* MyAIController = Cast<AAIController>(OwnerComp.GetOwner());
    if (!MyAIController)
    {
        UE_LOG(LogTemp, Error, TEXT("MyAIController is nullptr"));
        return EBTNodeResult::Failed;
    }

    const UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return EBTNodeResult::Failed;
    }

    if (this->FocusType == EFocusType::Actor)
    {
        AActor* Target = Cast<AActor>(BlackboardComp->GetValue<UBlackboardKeyType_Object>(this->FocusTarget.GetSelectedKeyID()));

        if (!Target)
        {
            UE_LOG(LogTemp, Error, TEXT("FocusTarget is not set"));
            return EBTNodeResult::Failed;
        }

        MyAIController->SetFocus(Target);
    }

    if (this->FocusType == EFocusType::Location)
    {
        FVector FocalPoint = this->FocalPointTarget.GetValue(OwnerComp);

        if (FocalPoint.Equals(FVector::ZeroVector))
        {
            UE_LOG(LogTemp, Error, TEXT("FocalPointTarget is not set"));
            return EBTNodeResult::Failed;
        }

        MyAIController->SetFocalPoint(FocalPoint);
    }

    return EBTNodeResult::Succeeded;
}

void UBTTask_SetFocus::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);

    if (const UBlackboardData* BBAsset = GetBlackboardAsset())
    {
        this->FocusTarget.ResolveSelectedKey(*BBAsset);
    }
    else
    {
        this->FocusTarget.InvalidateResolvedKey();
    }
}
