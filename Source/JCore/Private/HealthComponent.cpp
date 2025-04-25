// Copyright Joshua Gangl. All Rights Reserved.

#include "HealthComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

UHealthComponent::UHealthComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

    this->MaxHealth = 100.0f;
    this->Health = this->MaxHealth;
    this->bDead = false;

    this->SetIsReplicatedByDefault(true);
}

void UHealthComponent::BeginPlay()
{
    Super::BeginPlay();

    this->Health = this->MaxHealth;
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UHealthComponent, MaxHealth);
    DOREPLIFETIME(UHealthComponent, Health);
    DOREPLIFETIME(UHealthComponent, bDead);
    DOREPLIFETIME_CONDITION(UHealthComponent, HitSoundBase, COND_InitialOnly);
}

void UHealthComponent::ServerAddHealth_Implementation(float AmountToAdd)
{
    this->Health = FMath::Clamp(this->Health + AmountToAdd, 0.0f, this->MaxHealth);

    this->OnHealthChanged.Broadcast(this->Health);
}

void UHealthComponent::ServerRemoveHealth_Implementation(float AmountToRemove)
{
    // Don't do anything if already dead
    if (this->bDead)
    {
        return;
    }

    AActor* Owner = this->GetOwner();

    if (Owner)
    {
        UE_LOG(LogHealthComponent, Verbose, TEXT("%s took %f damage"), *Owner->GetName(), AmountToRemove)
    }


    this->Health = FMath::Clamp(this->Health - AmountToRemove, 0.0f, this->MaxHealth);

    this->OnHealthChanged.Broadcast(this->Health);
    this->OnDamageTaken.Broadcast(this->Health);

    this->MulticastPlayHitSound();

    if (this->Health == 0.0f)
    {
        this->ServerDie();
    }
}

void UHealthComponent::ServerDie_Implementation()
{
    AActor* Owner = this->GetOwner();

    if (Owner)
    {
        UE_LOG(LogHealthComponent, Verbose, TEXT("%s died."), *Owner->GetName())
    }

    this->bDead = true;
    this->OnDeath.Broadcast();
}

void UHealthComponent::ServerReset_Implementation()
{
    this->Health = this->MaxHealth;
    this->bDead  = false;

    this->OnHealthChanged.Broadcast(this->Health);
}

void UHealthComponent::MulticastPlayHitSound_Implementation()
{
    this->PlayHitSound();
}

void UHealthComponent::PlayHitSound()
{
    AActor* Owner = this->GetOwner();

    if (!Owner)
    {
        UE_LOG(LogHealthComponent, Error, TEXT("Owner is nullptr."))
        return;
    }

    if (!this->HitSoundBase)
    {
        UE_LOG(LogHealthComponent, Error, TEXT("HitSoundBase is nullptr."))
        return;
    }

    UGameplayStatics::PlaySoundAtLocation(GetWorld(), this->HitSoundBase, Owner->GetActorLocation());
}

void UHealthComponent::TakeDamage(float DamageToTake)
{
    this->ServerRemoveHealth(DamageToTake);
}

void UHealthComponent::OnRep_Health()
{
    this->OnHealthChanged.Broadcast(this->Health);
}
