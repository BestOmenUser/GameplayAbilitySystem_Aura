// Copyright Lyq


#include "AbilitySystem/ExecCalc/ExecCalc_Damage.h"

#include "AbilitySystemComponent.h"
#include "AudioMixerBlueprintLibrary.h"
#include "AuraAbilitySystemLibrary.h"
#include "AuraAbilityTypes.h"
#include "AuraGameplayTags.h"
#include "NiagaraDataInterfaceLandscape.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "AbilitySystem/Data/CharacterClassInfo.h"
#include "DataInterface/NiagaraDataInterfaceStaticMesh.h"
#include "Interaction/CombatInterface.h"

struct AuraDamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(Armor);
	DECLARE_ATTRIBUTE_CAPTUREDEF(ArmorPenetration);
	DECLARE_ATTRIBUTE_CAPTUREDEF(BlockChance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalHitResistance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalHitDamage);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalHitChance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(ResistanceFire);
	DECLARE_ATTRIBUTE_CAPTUREDEF(ResistanceLightning);
	DECLARE_ATTRIBUTE_CAPTUREDEF(ResistanceArcane);
	DECLARE_ATTRIBUTE_CAPTUREDEF(ResistancePhysical);

	TMap<FGameplayTag,FGameplayEffectAttributeCaptureDefinition> TagsToCaptureDef;
	
	AuraDamageStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAuraAttributeSet,Armor,Target,false)
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAuraAttributeSet,BlockChance,Target,false)
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAuraAttributeSet,ArmorPenetration,Source,false)
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAuraAttributeSet,CriticalHitResistance,Target,false)
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAuraAttributeSet,CriticalHitChance,Source,false)
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAuraAttributeSet,CriticalHitDamage,Source,false)
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAuraAttributeSet,ResistanceFire,Target,false)
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAuraAttributeSet,ResistanceLightning,Target,false)
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAuraAttributeSet,ResistanceArcane,Target,false)
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAuraAttributeSet,ResistancePhysical,Target,false)

		TagsToCaptureDef.Add(FAuraGameplayTags::Get().Attributes_Resistance_Fire,ResistanceFireDef);
		TagsToCaptureDef.Add(FAuraGameplayTags::Get().Attributes_Resistance_Lightning,ResistanceLightningDef);
		TagsToCaptureDef.Add(FAuraGameplayTags::Get().Attributes_Resistance_Arcane,ResistanceArcaneDef);
		TagsToCaptureDef.Add(FAuraGameplayTags::Get().Attributes_Resistance_Physical,ResistancePhysicalDef);
	}
};

static const AuraDamageStatics& DamageStatics()
{
	static AuraDamageStatics DStatics;
	return DStatics;
}

UExecCalc_Damage::UExecCalc_Damage()
{
	RelevantAttributesToCapture.Add(DamageStatics().ArmorDef);
	RelevantAttributesToCapture.Add(DamageStatics().BlockChanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().ArmorPenetrationDef);
	RelevantAttributesToCapture.Add(DamageStatics().CriticalHitResistanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().CriticalHitChanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().CriticalHitDamageDef);
	RelevantAttributesToCapture.Add(DamageStatics().ResistanceFireDef);
	RelevantAttributesToCapture.Add(DamageStatics().ResistanceLightningDef);
	RelevantAttributesToCapture.Add(DamageStatics().ResistanceArcaneDef);
	RelevantAttributesToCapture.Add(DamageStatics().ResistancePhysicalDef);
}

void UExecCalc_Damage::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const UAbilitySystemComponent* SourceASC =ExecutionParams.GetSourceAbilitySystemComponent();
	const UAbilitySystemComponent* TargetASC =ExecutionParams.GetTargetAbilitySystemComponent();

	AActor* SourceAvatar = SourceASC ? SourceASC->GetAvatarActor():nullptr;
	AActor* TargetAvatar = TargetASC ? TargetASC->GetAvatarActor():nullptr;

	int32 SourcePlayerLevel = 1;
	if (SourceAvatar->Implements<UCombatInterface>())
	{
		SourcePlayerLevel = ICombatInterface::Execute_GetPlayerLevel(SourceAvatar);
	}

	int32 TargetPlayerLevel = 1;
	if (TargetAvatar->Implements<UCombatInterface>())
	{
		TargetPlayerLevel = ICombatInterface::Execute_GetPlayerLevel(TargetAvatar);
	}
	
	
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	
	FAggregatorEvaluateParameters EvaluateParams;
	EvaluateParams.SourceTags=SourceTags;
	EvaluateParams.TargetTags=TargetTags;

	// Get Damage Set by Caller
	float Damage = 0;
	for (const TTuple<FGameplayTag,FGameplayTag>& Pair : FAuraGameplayTags::Get().DamageTypeToResistance)
	{
		const FGameplayTag DamageTypeTag = Pair.Key;
		const FGameplayTag ResistanceTag = Pair.Value;
		
		checkf(AuraDamageStatics().TagsToCaptureDef.Contains(ResistanceTag),TEXT("TagsToCapture doesn't contain Tag: [%s] in ExecCalc_Damage"),*ResistanceTag.ToString());
		const FGameplayEffectAttributeCaptureDefinition CaptureDef = AuraDamageStatics().TagsToCaptureDef[ResistanceTag];

		float Resistance = 0.f;
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(CaptureDef,EvaluateParams,Resistance);
		Resistance = FMath::Clamp(Resistance,0.f,100.f);
		
		float DamageTypeValue = Spec.GetSetByCallerMagnitude(Pair.Key,false);

		DamageTypeValue *=(100.f - Resistance)/100.f;
		
		Damage+=DamageTypeValue;
	}
	// Capture BlockChance on Target, and determine if there was a successful Block

	float TargetBlockChance=0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().BlockChanceDef,EvaluateParams,TargetBlockChance);
	TargetBlockChance = FMath::Max<float>(0.f,TargetBlockChance);

	const bool bBlocked = TargetBlockChance>FMath::RandRange(1.f,100.f);

	FGameplayEffectContextHandle EffectContextHandle=Spec.GetContext();
	UAuraAbilitySystemLibrary::SetIsBlockedHit(EffectContextHandle,bBlocked);
	// If Block, halve the damage.
	if (bBlocked)
	{
		Damage*=0.5f;
	}


	float TargetArmor=0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ArmorDef,EvaluateParams,TargetArmor);
	TargetArmor= FMath::Max<float>(0.f,TargetArmor);

	float SourceArmorPenetration=0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ArmorPenetrationDef,EvaluateParams,SourceArmorPenetration);
	SourceArmorPenetration= FMath::Max<float>(0.f,SourceArmorPenetration);

	UCharacterClassInfo* CharacterClassInfo = UAuraAbilitySystemLibrary::GetCharacterClassInfo(SourceAvatar);
	FRealCurve* ArmorPenetrationCurve = CharacterClassInfo->DamageCalculationCoefficients->FindCurve(FName("ArmorPenetration"),FString());
	const float ArmorPenetrationEfficient = ArmorPenetrationCurve->Eval(SourcePlayerLevel);
	
	// ArmorPenetration ignores a percentage of the Target's Armor.
	const float EffectiveArmor = TargetArmor*=(100-SourceArmorPenetration*ArmorPenetrationEfficient)/100.f;

	FRealCurve* EffectiveArmorCurve = CharacterClassInfo->DamageCalculationCoefficients->FindCurve(FName("EffectiveArmor"),FString());
	const float EffectiveArmorCoefficient = EffectiveArmorCurve->Eval(TargetPlayerLevel);
	
	// Armor ignores a percentage of incoming Damage.
	Damage*=(100-EffectiveArmor*EffectiveArmorCoefficient)/100.f;

	// Calc Crit Hit Chance
	float SourceCriticalHitChance=0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalHitChanceDef,EvaluateParams,SourceCriticalHitChance);
	SourceCriticalHitChance = FMath::Max<float>(0.f,SourceCriticalHitChance);

	float TargetCriticalHitResistance=0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalHitResistanceDef,EvaluateParams,TargetCriticalHitResistance);
	TargetCriticalHitResistance = FMath::Max<float>(0.f,TargetCriticalHitResistance);

	
	// Hit Resistance Coefficient
	FRealCurve* CritHitResistanceCurve= CharacterClassInfo->DamageCalculationCoefficients->FindCurve(FName("ResistanceCoefficient"),FString());
	const float CritHitResistanceCoefficient =CritHitResistanceCurve->Eval(TargetPlayerLevel);
	
	const float EffectiveCritHitChance = SourceCriticalHitChance-TargetCriticalHitResistance*CritHitResistanceCoefficient;
	const bool bCritHit = EffectiveCritHitChance>FMath::RandRange(0.f,100.f);

	
	UAuraAbilitySystemLibrary::SetIsCriticalHit(EffectContextHandle,bCritHit);
	// If Critical Hit  plus Critical Hit Damage
	if (bCritHit)
	{

		float CriticalHitDamage = 0.f;
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalHitDamageDef,EvaluateParams,CriticalHitDamage);
		CriticalHitDamage = FMath::Max<float>(0.f,CriticalHitDamage);

		Damage=2*Damage+CriticalHitDamage;
	}
	
	

	
	const FGameplayModifierEvaluatedData EvaluatedData(UAuraAttributeSet::GetIncomingDamageAttribute(),EGameplayModOp::Additive,Damage);
	OutExecutionOutput.AddOutputModifier(EvaluatedData);
}
