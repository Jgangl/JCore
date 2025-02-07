#include "Inventory/BuildingSubsystem.h"

#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/AssetManager.h"
#include "Kismet/KismetSystemLibrary.h"

UBuildingSubsystem::UBuildingSubsystem()
{

}

void UBuildingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    this->FindBuildingRecipeAssets();

    for (UBuildingRecipeDataAsset* RecipeDataAsset : this->AllBuildingRecipeDataAssets)
    {
        if (RecipeDataAsset)
        {
            UE_LOG(LogTemp, Warning, TEXT("Recipe: '%s' loaded"), *RecipeDataAsset->GetName());
        }
    }
}

void UBuildingSubsystem::Deinitialize()
{
}

UBuildingRecipeDataAsset* UBuildingSubsystem::GetRecipe(const TSubclassOf<ABuildable> InBuildableClass) const
{
    if (!InBuildableClass)
    {
        UE_LOG(LogTemp, Error, TEXT("%hs : InBuildableClass is nullptr"), __FUNCTION__);
        return nullptr;
    }

    for (UBuildingRecipeDataAsset* RecipeDataAsset : this->AllBuildingRecipeDataAssets)
    {
        if (!RecipeDataAsset) continue;

        if (!RecipeDataAsset->GetOutBuilding()) continue;

        if (RecipeDataAsset->GetOutBuilding()->GetBuildableClass() == InBuildableClass)
        {
            return RecipeDataAsset;
        }
    }

    return nullptr;
}

void UBuildingSubsystem::FindBuildingRecipeAssets()
{
    UAssetManager& AssetManager = UAssetManager::Get();
    IAssetRegistry& AssetRegistry = AssetManager.GetAssetRegistry();

    const FName PackageName             = TEXT("/Script/JCore");
    const FName BuildingRecipeAssetName = TEXT("BuildingRecipeDataAsset");

    const FTopLevelAssetPath TopLevelAssetPath(PackageName, BuildingRecipeAssetName);

    TArray<FAssetData> AssetData;

    AssetRegistry.GetAssetsByClass(TopLevelAssetPath, AssetData);

    for (const FAssetData& Asset : AssetData)
    {
        TSoftObjectPtr<UBuildingRecipeDataAsset> AssetSoftObjectPtr(Asset.GetSoftObjectPath());

        // TODO: Don't use Blocking load
        UBuildingRecipeDataAsset* RecipeDataAsset = Cast<UBuildingRecipeDataAsset>(UKismetSystemLibrary::LoadAsset_Blocking(AssetSoftObjectPtr));

        this->AllBuildingRecipeDataAssets.Add(RecipeDataAsset);
    }
}
