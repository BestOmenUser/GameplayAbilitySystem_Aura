// Copyright Lyq

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "HitMessageComponent.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UHitMessageComponent : public UWidgetComponent
{
	GENERATED_BODY()
public:	
	UFUNCTION(BlueprintImplementableEvent,BlueprintCallable)
	void SetHitMessage(bool bBlockedHit,bool bCriticalHit);

};
