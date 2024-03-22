// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "YukiWaveFunctionCollapseModel.h"
#include "GameFramework/Actor.h"
#include "YukiWaveFunctionCollapseContainer.generated.h"

UCLASS()
class YUKIWAVEFUNCTIONCOLLAPSERUNTIME_API AYukiWaveFunctionCollapseContainer : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AYukiWaveFunctionCollapseContainer();

	void ClearTiles();
	void InitWithSolver(UYukiWaveFunctionCollapseSolver* Solver);

	UPROPERTY()
	FIntVector Size;
	UPROPERTY()
	int CellSize;

	TArray<TObjectPtr<UActorComponent>> Tiles;
};
