// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float, NewHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDamageTaken, float, NewHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);

DECLARE_LOG_CATEGORY_CLASS(LogHealthComponent, Log, All);

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JCORE_API UHealthComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    //! @brief Sets default values for this component's properties
    UHealthComponent();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    //! @brief Delegate broadcasted when Health is changed
    UPROPERTY(BlueprintAssignable)
    FOnHealthChanged OnHealthChanged;

    //! @brief Delegate broadcasted when Health is removed
    UPROPERTY(BlueprintAssignable)
    FOnDamageTaken OnDamageTaken;

    //! @brief Delegate broadcasted when Health reaches 0
    UPROPERTY(BlueprintAssignable)
    FOnDeath OnDeath;

    UFUNCTION(BlueprintCallable, Server, Reliable)
    void ServerAddHealth(float AmountToAdd);

    UFUNCTION(BlueprintCallable, Server, Reliable)
    void ServerRemoveHealth(float AmountToRemove);

    UFUNCTION(BlueprintCallable, Server, Reliable)
    void ServerDie();

    UFUNCTION(BlueprintCallable, Server, Reliable)
    void ServerReset();

    UFUNCTION(NetMulticast, Reliable, BlueprintCallable)
    void MulticastPlayHitSound();

    UFUNCTION(BlueprintCallable)
    void PlayHitSound();

    UFUNCTION(BlueprintCallable)
    void TakeDamage(float DamageToTake);

protected:
    //! @brief Called when the game starts
    virtual void BeginPlay() override;

    //! @brief RepNotify which is called on clients when @ref Health is changed on the server
    UFUNCTION()
    void OnRep_Health();

    //! @brief RepNotify which is called on clients when Health is changed on the server
    UPROPERTY(EditAnywhere, Replicated)
    float MaxHealth;

    //! @brief Current amount of 'health'
    UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_Health)
    float Health;

    //! @brief Flag for being dead or not
    UPROPERTY(VisibleAnywhere, Replicated)
    bool bDead;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
    USoundBase* HitSoundBase;

public:
    UFUNCTION(BlueprintCallable, BlueprintPure)
    float GetMaxHealth() const { return this->MaxHealth; }

    UFUNCTION(BlueprintCallable, BlueprintPure)
    float GetHealth() const { return this->Health; }

    UFUNCTION(BlueprintCallable, BlueprintPure)
    float GetHealthPercent() const { return this->Health / this->MaxHealth; }

    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool IsDead() const { return this->bDead; }
};
