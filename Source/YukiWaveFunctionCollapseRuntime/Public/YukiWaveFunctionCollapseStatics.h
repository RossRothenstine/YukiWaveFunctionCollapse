// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "YukiWaveFunctionCollapseModel.h"
#include "UObject/Object.h"
#include "YukiWaveFunctionCollapseStatics.generated.h"

/**
 * 
 */
UCLASS()
class YUKIWAVEFUNCTIONCOLLAPSERUNTIME_API UYukiWaveFunctionCollapseStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "WaveFunctionCollapse")
	static UYukiWaveFunctionCollapseSolver* CreateSolverFromModel(UYukiWaveFunctionCollapseModel* Model, FIntVector Size, int32 Seed = 0);

	UFUNCTION(BlueprintCallable, Category = "WaveFunctionCollapse", meta = (WorldContext = "WorldContextObject"))
	static AActor* SpawnActorFromSolver(UObject* WorldContextObject, UYukiWaveFunctionCollapseSolver* Solver);
};
