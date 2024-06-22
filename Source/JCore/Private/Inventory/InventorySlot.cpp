// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/InventorySlot.h"
/*
UInventorySlot::UInventorySlot()
{
	CurrentStackSize = 0;

	Item = nullptr;
}

UItemDataAsset* UInventorySlot::GetItem()
{
	return Item;
}

void UInventorySlot::SetItem(UItemDataAsset* NewItem)
{
	this->Item = NewItem;
}

void UInventorySlot::SetItem(UItemDataAsset* NewItem, int32 Amount)
{
	SetItem(NewItem);

	if (!Item)
	{
		UE_LOG(LogTemp, Warning, TEXT("ITEM NULL"))
		return;
	}

	this->CurrentStackSize = FMath::Min(Amount, this->Item->GetMaxStackSize());
}

void UInventorySlot::AddAmount(int32 AmountToAdd)
{
	if (!Item)
	{
		return;
	}

	this->CurrentStackSize = FMath::Min(this->CurrentStackSize += AmountToAdd, this->Item->GetMaxStackSize());
}
*/
