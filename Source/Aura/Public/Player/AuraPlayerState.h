// Copyright Lyq

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/Data/LevelUpInfo.h"
#include "GameFramework/PlayerState.h"
#include "AuraPlayerState.generated.h"


class UAbilitySystemComponent;
class UAttributeSet;


DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerStateChanged, int32 /*StatVlue*/);
/**
 * 
 */
UCLASS()
class AURA_API AAuraPlayerState : public APlayerState,public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AAuraPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UAttributeSet* GetAttributeSet() const {return AttributeSet;};

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<ULevelUpInfo> LevelUpInfo;
	
	FOnPlayerStateChanged OnXPChangedDelegate;
	
	FOnPlayerStateChanged OnLevelChangedDelegate;

	FOnPlayerStateChanged OnAttributePointsChangedDelegate;

	FOnPlayerStateChanged OnSpellPointsChangedDelegate;

	FORCEINLINE int32 GetPlayerLevel() const {return Level;};
	FORCEINLINE int32 GetXP() const {return XP;};
	FORCEINLINE int32 GetAttributePoints() const {return AttributePoints;};
	FORCEINLINE int32 GetSpellPoints() const {return SpellPoints;};
	UFUNCTION(BlueprintCallable)
	void SetXP(int32 NewXP);

	UFUNCTION(BlueprintCallable)
	void AddToXP(int32 GainedXP);

	void SetLevel(int32 InLevel);
	
	UFUNCTION(BlueprintCallable)
	void AddToLevel(int32 InLevel);

	UFUNCTION(BlueprintCallable)
	void SetAttributePoints(int32 InAttributePoints);

	UFUNCTION(BlueprintCallable)
	void AddToAttributePoints(int32 InAttributePoints);

	UFUNCTION(BlueprintCallable)
	void SetSpellPoints(int32 InSpellPoints);

	UFUNCTION(BlueprintCallable)
	void AddToSpellPoints(int32 InSpellPoints);
	
protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
    
	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;

private:
	UPROPERTY(VisibleAnywhere,ReplicatedUsing=OnRep_Level)
	int32 Level=1;

	UPROPERTY(VisibleAnywhere,ReplicatedUsing=OnRep_XP)
	int32 XP;

	UPROPERTY(VisibleAnywhere,ReplicatedUsing=OnRep_AttributePoints)
	int32 AttributePoints=0;

	UPROPERTY(VisibleAnywhere,ReplicatedUsing=OnRep_SpellPoints)
	int32 SpellPoints=0;

	UFUNCTION()
	void OnRep_Level(int32 OldLevel);

	UFUNCTION()
	void OnRep_XP(int32 OldXP);

	UFUNCTION()
	void OnRep_AttributePoints(int32 OldAttributePoints);

	UFUNCTION()
	void OnRep_SpellPoints(int32 OldSpellPoints);
};
