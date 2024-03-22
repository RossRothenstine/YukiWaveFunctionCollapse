// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "Engine/DataAsset.h"
#include "YukiWaveFunctionCollapseModel.generated.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Border);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Empty);

/**
 * EYDWaveFunctionDirection
 *
 * Direction of the adjacency rule. 
 */
UENUM(BlueprintType)
enum class EYDWaveFunctionDirection
{
	XPlus UMETA(DisplayName = "X+ Right"),
	XMinus UMETA(DisplayName = "X- Left"),
	YPlus UMETA(DisplayName = "Y+ Forward"),
	YMinus UMETA(DisplayName = "Y- Back"),
	ZPlus UMETA(DisplayName = "Z+ Up"),
	ZMinus UMETA(DisplayName = "Z- Down"),
	MAX UMETA(Hidden)
};

USTRUCT(BlueprintType)
struct FYukiWaveFunctionCollapseTileModel
{
	GENERATED_BODY()

public:
	/**
	 * TileActor is the actor that will be spawned when this tile is selected.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftClassPtr<AActor> TileActor;

	/**
	 * Optional Rotation, to prevent needing to duplicate actors.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRotator Rotation = FRotator::ZeroRotator;

	/**
	 * Scaling if needed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Scale = FVector::OneVector;

	/**
	 * Weight of the cell. When greater than 1.0, will be favored more heavily to be selected.
	 * Cells with a weight of 0.0 will always be selected first.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Weight = 1.0;

	/**
	 * Max Count of this cell, -1 for infinite.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int MaxCount = -1;

	/**
	 * Brush Texture for the Minimap
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UTexture2D> BrushTexture;

	/**
	 * Neighbor Options. Determines compatibility with other tags in specific directions.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<EYDWaveFunctionDirection, FGameplayTagContainer> Options = {
		{EYDWaveFunctionDirection::XPlus, FGameplayTagContainer()},
		{EYDWaveFunctionDirection::XMinus, FGameplayTagContainer()},
		{EYDWaveFunctionDirection::YPlus, FGameplayTagContainer()},
		{EYDWaveFunctionDirection::YMinus, FGameplayTagContainer()},
		{EYDWaveFunctionDirection::ZPlus, FGameplayTagContainer()},
		{EYDWaveFunctionDirection::ZMinus, FGameplayTagContainer()},
	};

	/**
	 * Directions that can be reached from this cell.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSet<EYDWaveFunctionDirection> WalkDirections;
};

UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew)
class UYukiWaveFunctionCollapseSolverDecorator : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	virtual void OnCellCollapsed(int Cell, const FGameplayTag& Tag, UYukiWaveFunctionCollapseSolver* Solver)
	{
	}
};

/**
 * When a cell is collapsed with a given tag, all other tags from possibilities are removed.
 */
UCLASS(BlueprintType, Blueprintable)
class UYukiWaveFunctionCollapseSolverDecorator_MutuallyExclusive : public UYukiWaveFunctionCollapseSolverDecorator
{
	GENERATED_BODY()

public:
	virtual void OnCellCollapsed(int Cell, const FGameplayTag& Tag, UYukiWaveFunctionCollapseSolver* Solver) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ExposeOnSpawn))
	FGameplayTagContainer Tags;
};

UCLASS(Blueprintable, BlueprintType)
class YUKIWAVEFUNCTIONCOLLAPSERUNTIME_API UYukiWaveFunctionCollapseModel : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FGameplayTag, FYukiWaveFunctionCollapseTileModel> Tiles;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<EYDWaveFunctionDirection, FGameplayTagContainer> Borders;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int CellSize;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced)
	TArray<TObjectPtr<UYukiWaveFunctionCollapseSolverDecorator>> Decorators;

#if WITH_EDITORONLY_DATA
	/**
	 * Fixes any contradictions found, will replicate patterns in the opposite direction.
	 */
	UFUNCTION(CallInEditor)
	void SolveContradictions();
#endif
};

USTRUCT(BlueprintType)
struct FYukiWaveFunctionCollapseCell
{
	GENERATED_BODY()

public:
	/**
	 * A list of all available options for this cell.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer Options;
};

UCLASS(BlueprintType)
class YUKIWAVEFUNCTIONCOLLAPSERUNTIME_API UYukiWaveFunctionCollapseSolver : public UObject
{
	GENERATED_BODY()

public:
	UYukiWaveFunctionCollapseSolver();

	UFUNCTION(BlueprintCallable)
	void Init(UYukiWaveFunctionCollapseModel* InModel, FIntVector InSize, FRandomStream InRandom);

	void CheckContradictions();
	UFUNCTION(BlueprintCallable)
	// Continues to do a SingleIteration until solving is finished.
	void SolveFully();

	UFUNCTION(BlueprintCallable)
	// Does a single iteration of solving.
	void SingleIteration();

	/**
	 * Current state of cells.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FYukiWaveFunctionCollapseCell> Cells;

	/**
	 * Model that will be solved.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UYukiWaveFunctionCollapseModel* Model;

	/**
	 * Size of the grid as X*Y*Z.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntVector Size;

	// Returns Cells that contain a tag.
	UFUNCTION(BlueprintCallable)
	TArray<int> GetCellsByTag(const FGameplayTag& Tag) const;
	// Returns Cells that contain any of the given tags.
	UFUNCTION(BlueprintCallable)
	TArray<int> GetCellsByAnyTags(const FGameplayTagContainer& Tags) const;
	// Returns Cells that contain all of the given tags.
	UFUNCTION(BlueprintCallable)
	TArray<int> GetCellsByAllTags(const FGameplayTagContainer& Tags) const;

	UFUNCTION(BlueprintCallable)
	bool RemoveTag(int Index, const FGameplayTag& Tag);
	// Remove and optionally collapse all cells that contain the given tag.
	UFUNCTION(BlueprintCallable)
	void RemoveTagFromUncollapsedCells(const FGameplayTag& Tag);

	// Returns the distance between two cells by their index.
	UFUNCTION(BlueprintCallable)
	float CellHorizontalDistanceSquared(int IndexA, int IndexB) const;

	// Returns the walkable distance between two cells, or -1 if the cell is not reachable.
	UFUNCTION(BlueprintCallable)
	int CellWalkingDistance(int From, int To) const;

	TArray<TTuple<EYDWaveFunctionDirection, int>> GetCollapsedNeighbors(int Index) const;

	UFUNCTION(BlueprintCallable)
	bool IsCellCollapsed(int Index) const;

	UFUNCTION(BlueprintCallable)
	TArray<int> GetWalkableNeighbors(int Index) const;

	UFUNCTION(BlueprintCallable)
	int CountCellsWithTag(const FGameplayTag& Tag) const;

	UFUNCTION(BlueprintCallable)
	FGameplayTagContainer GetTagsForIndex(int Index) const;

	UFUNCTION(BlueprintCallable)
	void RemoveTagsFromUncollapsed(const FGameplayTagContainer& Tags);

protected:
	// Returns true if the solver is solved.
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool IsSolved() const;

	int GetMinimumEntropyCellIndex() const;
	void CollapseAt(int Index);
	void PropagateFrom(int Index);

	FGameplayTag SelectTag(int Index) const;
	TArray<TTuple<EYDWaveFunctionDirection, int>> ValidDirections(int Index) const;
	FGameplayTagContainer GetValidNeighbors(int Index, EYDWaveFunctionDirection Direction);
	TArray<EYDWaveFunctionDirection> ValidBorders(int Index) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRandomStream Random;
};
