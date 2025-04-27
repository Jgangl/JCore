#include "Misc/AutomationTest.h"

#include "Inventory/InventoryComponent.h"

BEGIN_DEFINE_SPEC(FInventoryComponentSpec, "JCore.Inventory",
                  EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

UItemDataAsset* TestItemAsset;
UInventoryComponent* TestInventoryComponent;

END_DEFINE_SPEC(FInventoryComponentSpec)

void FInventoryComponentSpec::Define()
{
    BeforeEach([this]()
    {
        if (!TestItemAsset)
        {
            TestItemAsset = LoadObject<UItemDataAsset>(nullptr, TEXT("/Script/JCore.ItemDataAsset'/JCore/Testing/DA_TestingItem.DA_TestingItem'"));
        }

        TestInventoryComponent = NewObject<UInventoryComponent>();
        TestInventoryComponent->SetNumberOfSlots(10);
        TestInventoryComponent->InitializeInventorySlots();
    });

    Describe("ContainsItem", [this]()
    {
        It("Return -1 when input item is nullptr", [this]()
        {
            AddExpectedError(TEXT("ContainsItem"), EAutomationExpectedErrorFlags::Contains, 1);

            int32 ReturnVal = TestInventoryComponent->ContainsItem(nullptr);

            TestEqual(TEXT("Return 0"), ReturnVal, 0);
        });

        It("Output number of items in 1 stack", [this]()
        {
            TestInventoryComponent->GetInventorySlots()[5] = FInventorySlot(TestItemAsset, 4);

            int32 ReturnVal = TestInventoryComponent->ContainsItem(TestItemAsset);

            TestEqual(TEXT("Return correct amount"), ReturnVal, 4);
        });
    });

    Describe("HasAvailableSpaceForItem", [this]()
    {
        It("Return False when input is nullptr", [this]()
        {
            AddExpectedError(TEXT("nullptr"), EAutomationExpectedErrorFlags::Contains, 1);

            bool ReturnVal = TestInventoryComponent->HasAvailableSpaceForItem(nullptr);

            TestFalse(TEXT("Return false"), ReturnVal);
        });

        It("Return False when slots are full", [this]()
        {
            TestInventoryComponent->SetNumberOfSlots(1);
            TestInventoryComponent->InitializeInventorySlots();
            TestInventoryComponent->GetInventorySlots()[0] = FInventorySlot(TestItemAsset, TestItemAsset->GetMaxStackSize());

            bool ReturnVal = TestInventoryComponent->HasAvailableSpaceForItem(TestItemAsset);

            TestFalse(TEXT("Return false"), ReturnVal);
        });

        It("Return False when no available slots", [this]()
        {
            TestInventoryComponent->SetNumberOfSlots(0);
            TestInventoryComponent->InitializeInventorySlots();

            bool ReturnVal = TestInventoryComponent->HasAvailableSpaceForItem(TestItemAsset);

            TestFalse(TEXT("Return false"), ReturnVal);
        });

        It("Return True when available slot", [this]()
        {
            TestInventoryComponent->SetNumberOfSlots(1);
            TestInventoryComponent->InitializeInventorySlots();

            bool ReturnVal = TestInventoryComponent->HasAvailableSpaceForItem(TestItemAsset);

            TestTrue(TEXT("Return true"), ReturnVal);
        });

        It("Return True when available slot for amount", [this]()
        {
            TestInventoryComponent->SetNumberOfSlots(1);
            TestInventoryComponent->InitializeInventorySlots();

            TestInventoryComponent->GetInventorySlots()[0] = FInventorySlot(TestItemAsset, 2);

            bool ReturnVal = TestInventoryComponent->HasAvailableSpaceForItem(TestItemAsset, TestItemAsset->GetMaxStackSize() - 2);

            TestTrue(TEXT("Return true"), ReturnVal);
        });
    });
};
