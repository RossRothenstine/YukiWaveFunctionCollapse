// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "YukiWaveFunctionCollapseSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class YUKIWAVEFUNCTIONCOLLAPSERUNTIME_API UYukiWaveFunctionCollapseSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	FORCEINLINE static UYukiWaveFunctionCollapseSubsystem* GetSubsystem(UWorld* World)
	{
		check(World);
		return World->GetSubsystem<UYukiWaveFunctionCollapseSubsystem>();
	}
};
