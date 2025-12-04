// Copyright Lyq


#include "AbilitySystem/Data/LevelUpInfo.h"

int32 ULevelUpInfo::FindLevelForXP(int32 Exp) const
{
	int Level = 1;
	bool bSearching = true;

	while (bSearching)
	{
		if (LevelUpInformation.Num() <= Level)	return Level;
		
		if (Exp >= LevelUpInformation[Level].LevelUpRequirement)
		{
			++Level;
		}
		else
		{
			bSearching = false;
		}
	}
	
	return Level;
}
