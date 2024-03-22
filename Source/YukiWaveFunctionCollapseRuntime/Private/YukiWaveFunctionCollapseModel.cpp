// Fill out your copyright notice in the Description page of Project Settings.
#include "YukiWaveFunctionCollapseModel.h"

#include "AssetViewUtils.h"
#include "YukiWaveFunctionCollapseLog.h"
#include "NativeGameplayTags.h"
#include "ScopedTransaction.h"
#include "Subsystems/EditorAssetSubsystem.h"
#include "UObject/PropertyAccessUtil.h"

#define LOCTEXT_NAMESPACE "YukiWaveFunctionCollapseModel"

UE_DEFINE_GAMEPLAY_TAG(TAG_Border, "WFC.Constraints.Border")
UE_DEFINE_GAMEPLAY_TAG(TAG_Empty, "WFC.Constraints.Empty")

// Returns a Neighboring cell given an index and a direction. Will return false if the index is at a boundary of that direction.
bool GetNeighborCell(FIntVector Size, int Index, EYDWaveFunctionDirection Direction, int& OutNeighborIndex)
{
	// The theory behind this math is that given an array of size N, a 3D array would be N*N*N.
	// For example, a grid of 3 would be laid out in this order:
	//0: [0, 1, 2], 1: [9,  10, 11], 2: [18, 19, 20]
	//   [3, 4, 5],    [12, 13, 14],	[21, 22, 23]
	//   [6, 7, 8]	   [15, 16, 17],	[24, 25, 26]

	// For an odd lengthed vector of A*B*C elements, it can be visualized like:
	// 0: [0, 1, 2, 3]    1: [12, 13, 14, 15],
	//    [4, 5, 6, 7]       [16, 17, 18, 19],
	//    [8, 9, 10, 11]     [20, 21, 22, 23]

	// Index = X + (Y * #X) + (Z * #X * #Y)

	int X = Index % Size.X;
	int Y = (Index / Size.X) % Size.Y;
	int Z = (Index / (Size.X * Size.Y) % Size.Z);

	switch (Direction)
	{
		case EYDWaveFunctionDirection::XPlus:
			if (X == Size.X - 1)
			{
				return false;
			}
			OutNeighborIndex = Index + 1;
			break;
		case EYDWaveFunctionDirection::XMinus:
			if (X == 0)
			{
				return false;
			}
			OutNeighborIndex = Index - 1;
			break;
		case EYDWaveFunctionDirection::YPlus:
			if (Y == Size.Y - 1)
			{
				return false;
			}
			OutNeighborIndex = Index + Size.X;
			break;
		case EYDWaveFunctionDirection::YMinus:
			if (Y == 0)
			{
				return false;
			}
			OutNeighborIndex = Index - Size.X;
			break;
		case EYDWaveFunctionDirection::ZPlus:
			if (Z == Size.Z - 1)
			{
				return false;
			}
			OutNeighborIndex = Index + (Size.X * Size.Y);
			break;
		case EYDWaveFunctionDirection::ZMinus:
			if (Z == 0)
			{
				return false;
			}
			OutNeighborIndex = Index - (Size.X * Size.Y);
			break;
		default:
			return false;
	}
	return true;
}

EYDWaveFunctionDirection GetOppositeDirection(EYDWaveFunctionDirection Direction)
{
	switch (Direction)
	{
		case EYDWaveFunctionDirection::XPlus:
			return EYDWaveFunctionDirection::XMinus;
		case EYDWaveFunctionDirection::XMinus:
			return EYDWaveFunctionDirection::XPlus;
		case EYDWaveFunctionDirection::YPlus:
			return EYDWaveFunctionDirection::YMinus;
		case EYDWaveFunctionDirection::YMinus:
			return EYDWaveFunctionDirection::YPlus;
		case EYDWaveFunctionDirection::ZPlus:
			return EYDWaveFunctionDirection::ZMinus;
		case EYDWaveFunctionDirection::ZMinus:
			return EYDWaveFunctionDirection::ZPlus;
		default:
			checkNoEntry();
			return EYDWaveFunctionDirection::MAX;
	}
}

#if WITH_EDITORONLY_DATA
void UYukiWaveFunctionCollapseModel::SolveContradictions()
{
	FScopedTransaction Transaction(LOCTEXT("SolveContradictions", "Solve Contradictions"));
	Modify();
	for (const auto& Tile : Tiles)
	{
		const FGameplayTag& Tag = Tile.Key;
		const FYukiWaveFunctionCollapseTileModel& TileData = Tile.Value;

		// A contradiction occurs if we claim to have a neighbor but they claim to not have
		// us in the opposite direction.
		for (const auto& Option : TileData.Options)
		{
			const EYDWaveFunctionDirection Direction = Option.Key;
			const FGameplayTagContainer& Tags = Option.Value;
			const EYDWaveFunctionDirection OppositeDirection = GetOppositeDirection(Direction);
			FGameplayTagContainer RemovalTags;
			for (const auto& Neighbor : Tags)
			{
				if (!Tiles.Contains(Neighbor))
				{
					UE_LOG(LogWFC, Log, TEXT("Tile %s has a neighbor %s that does not exist.. removing it."), *Tag.ToString(), *Neighbor.ToString());
					RemovalTags.AddTag(Neighbor);
					continue;
				}
				FYukiWaveFunctionCollapseTileModel& NeighborData = Tiles[Neighbor];
				if (!NeighborData.Options.Contains(OppositeDirection))
				{
					NeighborData.Options[OppositeDirection] = FGameplayTagContainer(Tag);
				}
				else if (!NeighborData.Options[OppositeDirection].HasTag(Tag))
				{
					UE_LOG(LogWFC, Log, TEXT("Tile %s has a neighbor %s that does not have us in the opposite direction %s"), *Tag.ToString(), *Neighbor.ToString(), *UEnum::GetValueAsString(OppositeDirection));
					NeighborData.Options[OppositeDirection].AddTag(Tag);
				}
			}
			Tiles[Tag].Options[Direction].RemoveTags(RemovalTags);
		}
	}
	UEditorAssetSubsystem* AssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
	AssetSubsystem->SaveAsset(GetPathName());
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyEditorModule.NotifyCustomizationModuleChanged();
}
#endif
UYukiWaveFunctionCollapseSolver::UYukiWaveFunctionCollapseSolver()
{
	Random = FRandomStream(FPlatformTime::Cycles());
}

void UYukiWaveFunctionCollapseSolver::Init(UYukiWaveFunctionCollapseModel* InModel, FIntVector InSize, FRandomStream InRandom)
{
	Model = InModel;
	Size = InSize;
	Random = InRandom;
	UE_LOG(LogWFC, Log, TEXT("Init Solver with Size: %s and Seed: %d"), *Size.ToString(), InRandom.GetCurrentSeed());
	Cells.Empty();
	Cells.Reserve(Size.X * Size.Y * Size.Z);

	FGameplayTagContainer AllTags;
	TArray<FGameplayTag> AllTagsArray;
	Model->Tiles.GetKeys(AllTagsArray);
	for (const auto& GameplayTag : AllTagsArray)
	{
		AllTags.AddTag(GameplayTag);
	}

	for (int i = 0; i < Cells.Max(); i++)
	{
		FYukiWaveFunctionCollapseCell Cell;
		Cell.Options.AppendTags(AllTags);
		Cells.Add(Cell);
	}
	for (int i = 0; i < Cells.Num(); i++)
	{
		FYukiWaveFunctionCollapseCell& Cell = Cells[i];
		TArray<EYDWaveFunctionDirection> ValidDirections = ValidBorders(i);
		if (ValidDirections.Num() > 0)
		{
			for (const auto& Border : ValidBorders(i))
			{
				if (Model->Borders.Contains(Border) && Model->Borders[Border].Num() > 0)
				{
					Cell.Options = Cell.Options.Filter(Model->Borders[Border]);
					PropagateFrom(i);
				}
			}
		}
	}
}
void UYukiWaveFunctionCollapseSolver::CheckContradictions()
{
	for (const auto& Tile : Model->Tiles)
	{
		const FGameplayTag& Tag = Tile.Key;
		const FYukiWaveFunctionCollapseTileModel& TileData = Tile.Value;

		// A contradiction occurs if we claim to have a neighbor but they claim to not have
		// us in the opposite direction.
		for (const auto& Option : TileData.Options)
		{
			const EYDWaveFunctionDirection Direction = Option.Key;
			const FGameplayTagContainer& Tags = Option.Value;
			const EYDWaveFunctionDirection OppositeDirection = GetOppositeDirection(Direction);
			for (const auto& Neighbor : Tags)
			{
				ensureMsgf(Model->Tiles.Contains(Neighbor), TEXT("Tile %s has a neighbor %s that does not exist."), *Tag.ToString(), *Neighbor.ToString());
				const FYukiWaveFunctionCollapseTileModel& NeighborData = Model->Tiles[Neighbor];
				ensureMsgf(NeighborData.Options.Contains(OppositeDirection), TEXT("Tile %s has a neighbor %s that does not have a %s direction."), *Tag.ToString(), *Neighbor.ToString(), *UEnum::GetValueAsString(OppositeDirection));
				ensureMsgf(NeighborData.Options[OppositeDirection].HasTag(Tag), TEXT("Tile %s has a connection from %s to %s but neighbor does not have it from %s"), *Tag.ToString(), *UEnum::GetValueAsString(OppositeDirection), *Neighbor.ToString(), *UEnum::GetValueAsString(Direction));
			}
		}
	}
}
void UYukiWaveFunctionCollapseSolver::SolveFully()
{
	CheckContradictions();

	// Edge case, solved even before single iteration. Probably not possible, may be is.
	if (IsSolved())
	{
		return;
	}

	int NumIterations = 0;
	const int MaxIterations = Size.X * Size.Y * Size.Z;
	do
	{
		if (NumIterations > MaxIterations)
		{
			Random = FRandomStream(FPlatformTime::Cycles());
			Init(Model, Size, Random);
			NumIterations = 0;
		}
		SingleIteration();
		++NumIterations;
	}
	while (!IsSolved());
}
void UYukiWaveFunctionCollapseSolver::SingleIteration()
{
	const int Index = GetMinimumEntropyCellIndex();
	CollapseAt(Index);
	PropagateFrom(Index);
}

TArray<int> UYukiWaveFunctionCollapseSolver::GetCellsByTag(const FGameplayTag& Tag) const
{
	TArray<int> OutCells;
	for (int i = 0; i < Cells.Num(); i++)
	{
		if (Cells[i].Options.HasTag(Tag))
		{
			OutCells.Add(i);
		}
	}
	return OutCells;
}

TArray<int> UYukiWaveFunctionCollapseSolver::GetCellsByAnyTags(const FGameplayTagContainer& Tags) const
{
	TArray<int> OutCells;
	for (int i = 0; i < Cells.Num(); i++)
	{
		if (Cells[i].Options.HasAny(Tags))
		{
			OutCells.Add(i);
		}
	}
	return OutCells;
}

TArray<int> UYukiWaveFunctionCollapseSolver::GetCellsByAllTags(const FGameplayTagContainer& Tags) const
{
	TArray<int> OutCells;
	for (int i = 0; i < Cells.Num(); i++)
	{
		if (Cells[i].Options.HasAll(Tags))
		{
			OutCells.Add(i);
		}
	}
	return OutCells;	
}

bool UYukiWaveFunctionCollapseSolver::IsSolved() const
{
	for (const auto& It : Cells)
	{
		if (It.Options.Num() == 0 || It.Options.Num() > 1)
		{
			return false;
		}
	}
	return true;
}
int UYukiWaveFunctionCollapseSolver::GetMinimumEntropyCellIndex() const
{
	TArray<TTuple<int, int>> EntropyList;
	for (int i = 0; i < Cells.Num(); i++)
	{
		if (Cells[i].Options.Num() != 1)
		{
			EntropyList.Add(TTuple<int, int>(i, Cells[i].Options.Num()));
		}
	}
	EntropyList.Sort([](const TTuple<int, int>& A, const TTuple<int, int>& B) { return A.Get<1>() < B.Get<1>(); });
	// Return a random value between the range of least entropious.
	const int MinEntropy = EntropyList[0].Get<1>();
	TArray<TTuple<int, int>> FilteredList = EntropyList.FilterByPredicate([MinEntropy](const TTuple<int, int>& A) { return A.Get<1>() == MinEntropy; });
	const int RandomIndex = Random.RandRange(0, FilteredList.Num() - 1);
	return FilteredList[RandomIndex].Get<0>();
}

void UYukiWaveFunctionCollapseSolver::CollapseAt(int Index)
{
	FGameplayTag SelectedTag = SelectTag(Index);
	Cells[Index].Options.Reset();
	Cells[Index].Options.AddTag(SelectedTag);
}

FGameplayTag UYukiWaveFunctionCollapseSolver::SelectTag(int Index) const
{
	if (Cells[Index].Options.Num() == 0)
	{
		UE_LOG(LogWFC, Warning, TEXT("SelectTag called on a cell (%d) with no options."), Index)
		return FGameplayTag();
	}
	// If the cell has a tag with a weight of 0.0, we will collapse that one specifically.
	for (const auto& It : Cells[Index].Options)
	{
		if (Model->Tiles[It].Weight == 0.0f)
		{
			return It;
		}
	}

	// Normalize the weights across all cells.
	float TotalWeight = 0.0f;
	TArray<TTuple<FGameplayTag, float>> WeightedOptions;
	for (const auto& It : Cells[Index].Options)
	{
		TotalWeight += Model->Tiles[It].Weight;
	}
	for (const auto& It : Cells[Index].Options)
	{
		WeightedOptions.Add(TTuple<FGameplayTag, float>(It, Model->Tiles[It].Weight / TotalWeight));
	}
	// Sort the weighted options.
	WeightedOptions.Sort([](const TTuple<FGameplayTag, float>& A, const TTuple<FGameplayTag, float>& B) { return A.Get<1>() < B.Get<1>(); });
	// Get the maximum weight.
	const float MaxWeight = WeightedOptions.Last().Get<1>();
	// Select a value between min and max.
	const float RandomWeight = Random.FRandRange(WeightedOptions[0].Get<1>(), MaxWeight);
	// Find all that clear the threshold, select a random one from them.
	TArray<TTuple<FGameplayTag, float>> FilteredList = WeightedOptions.FilterByPredicate([RandomWeight](const TTuple<FGameplayTag, float>& A) { return A.Get<1>() >= RandomWeight; });
	const int RandomIndex = Random.RandRange(0, FilteredList.Num() - 1);
	UE_LOG(LogWFC, Log, TEXT("For cell %d, From Tags %s Selecting tag %s"), Index, *Cells[Index].Options.ToString(), *FilteredList[RandomIndex].Get<0>().ToString(), FilteredList[RandomIndex].Get<1>());
	return FilteredList[RandomIndex].Get<0>();
}

void UYukiWaveFunctionCollapseSolver::PropagateFrom(int Index)
{
	TArray<int> Stack;
	Stack.Push(Index);

	while (Stack.Num() > 0)
	{
		int NextIndex = Stack.Pop();
		for (const auto& It : ValidDirections(NextIndex))
		{
			EYDWaveFunctionDirection Direction = It.Get<0>();
			int NeighborIndex = It.Get<1>();

			ensureMsgf((NeighborIndex > -1 && NeighborIndex < Cells.Num()), TEXT("NeighborIndex is out of range: %d"), NeighborIndex);

			FGameplayTagContainer ValidNeighbors = GetValidNeighbors(NextIndex, Direction);
			FGameplayTagContainer& Neighbors = Cells[NeighborIndex].Options;
			int PrevNumNeighbors = Neighbors.Num();
			Neighbors = Neighbors.FilterExact(ValidNeighbors);
			int CurrNumNeighbors = Neighbors.Num();
			if (CurrNumNeighbors != PrevNumNeighbors)
			{
				if (!Stack.Contains(NeighborIndex))
				{
					Stack.Push(NeighborIndex);
				}
			}
		}
	}
}
TArray<TTuple<EYDWaveFunctionDirection, int>> UYukiWaveFunctionCollapseSolver::ValidDirections(int Index) const
{
	// Return all valid directions from the given Index.
	TArray<TTuple<EYDWaveFunctionDirection, int>> ValidDirections;
	for (int i = 0; i < (int) EYDWaveFunctionDirection::MAX; i++)
	{
		int NeighborIndex;
		if (GetNeighborCell(Size, Index, (EYDWaveFunctionDirection) i, NeighborIndex))
		{
			ValidDirections.Add(TTuple<EYDWaveFunctionDirection, int>((EYDWaveFunctionDirection) i, NeighborIndex));
		}
	}
	return ValidDirections;
}

FGameplayTagContainer UYukiWaveFunctionCollapseSolver::GetTagsForIndex(int Index) const
{
	FGameplayTagContainer OptionsCopy = Cells[Index].Options;
	return OptionsCopy;
}

void UYukiWaveFunctionCollapseSolver::RemoveTagsFromUncollapsed(const FGameplayTagContainer& Tags)
{
	for (int i = 0; i < Cells.Num(); i++)
	{
		if (Cells[i].Options.Num() > 1 && Cells[i].Options.HasAny(Tags))
		{
			Cells[i].Options.RemoveTags(Tags);
			PropagateFrom(i);
		}
	}
}

FGameplayTagContainer UYukiWaveFunctionCollapseSolver::GetValidNeighbors(int Index, EYDWaveFunctionDirection Direction)
{
	FGameplayTagContainer Container;
	for (const auto& GameplayTag : Cells[Index].Options)
	{
		Container.AppendTags(Model->Tiles[GameplayTag].Options[Direction]);
	}
	// Remove neighbors that are past the count.
	FGameplayTagContainer TagsToRemove;
	for (const FGameplayTag& GameplayTag : Container)
	{
		int MaxCount = Model->Tiles[GameplayTag].MaxCount;
		if (MaxCount != -1 && CountCellsWithTag(GameplayTag) >= MaxCount)
		{
			TagsToRemove.AddTag(GameplayTag);
		}
	}
	Container.RemoveTags(TagsToRemove);
	return Container;
}
bool UYukiWaveFunctionCollapseSolver::RemoveTag(int Index, const FGameplayTag& Tag)
{
	Cells[Index].Options.RemoveTag(Tag);
	return Cells[Index].Options.Num() > 0;
}

void UYukiWaveFunctionCollapseSolver::RemoveTagFromUncollapsedCells(const FGameplayTag& Tag)
{
	for (int i = 0; i < Cells.Num(); i++)
	{
		if (Cells[i].Options.Num() == 1)
		{
			continue;
		}
		if (Cells[i].Options.HasTag(Tag))
		{
			Cells[i].Options.RemoveTag(Tag);
			PropagateFrom(i);
		}
	}
}
float UYukiWaveFunctionCollapseSolver::CellHorizontalDistanceSquared(int IndexA, int IndexB) const
{
	int AX = IndexA % Size.X;
	int AY = (IndexA / Size.X) % Size.Y;
	int BX = IndexB % Size.X;
	int BY = (IndexB / Size.X) % Size.Y;
	return ((AX-BX)*(AX-BX)) + ((AY-BY)*(AY-BY));
}

int UYukiWaveFunctionCollapseSolver::CellWalkingDistance(int From, int To) const
{
	TArray<bool> Visited;
	Visited.Reserve(Cells.Num());
	for (int i = 0; i < Cells.Num(); i++)
	{
		Visited.Add(false);
	}
	// Current traversal path.
	TArray<int> Path;
	// Walking path, not necessarily the traversal path.
	TArray<int> Stack;

	// Starting with From, visit the cell and then push all neighboring cells onto the stack. If we end up
	// in a situation from a cell where no path is walkable, then we backtrack to the last cell that has not been walked.
	Visited[From] = true;

	while (Stack.Num() > 0)
	{
		int Location = Stack.Pop();
		if (Location == To)
		{
			// We made it!
			return Path.Num();
		}
		Path.Push(Location);
		Visited[Location] = true;

		TArray<int> WalkableNeighbors = GetWalkableNeighbors(Location);
		WalkableNeighbors = WalkableNeighbors.FilterByPredicate([&Visited](auto Neighbor) { return !Visited[Neighbor]; });
		if (WalkableNeighbors.Num() == 0)
		{
			// Rewind the Path until we find a cell that has unvisited neighbors.
			while (Path.Num() > 0)
			{
				int LastLocation = Path.Pop();
				TArray<int> LastWalkableNeighbors = GetWalkableNeighbors(LastLocation);
				LastWalkableNeighbors = LastWalkableNeighbors.FilterByPredicate([&Visited](auto Neighbor) { return !Visited[Neighbor]; });
				if (LastWalkableNeighbors.Num() > 0)
				{
					// We found a cell that has unvisited neighbors. Push it back onto the stack and continue.
					Stack.Push(LastLocation);
					break;
				}
			}
		}
		else
		{
			// Choose a random neighbor and traverse down that direction.
			int RandomNeighbor = WalkableNeighbors[Random.RandRange(0, WalkableNeighbors.Num() - 1)];
			Stack.Push(RandomNeighbor);
		}
	}
	// Ran out of places to walk.
	return -1;
}

TArray<TTuple<EYDWaveFunctionDirection, int>> UYukiWaveFunctionCollapseSolver::GetCollapsedNeighbors(int Index) const
{
	// Return all valid directions from the given Index.
	TArray<TTuple<EYDWaveFunctionDirection, int>> ValidNeighbors;
	for (int i = 0; i < (int) EYDWaveFunctionDirection::MAX; i++)
	{
		int NeighborIndex;
		if (GetNeighborCell(Size, Index, (EYDWaveFunctionDirection) i, NeighborIndex))
		{
			if (IsCellCollapsed(NeighborIndex))
			{
				ValidNeighbors.Add(TTuple<EYDWaveFunctionDirection, int>((EYDWaveFunctionDirection)i, NeighborIndex));
			}
		}
	}
	return ValidNeighbors;	
}
bool UYukiWaveFunctionCollapseSolver::IsCellCollapsed(int Index) const
{
	return Cells[Index].Options.Num() == 1;
}
TArray<int> UYukiWaveFunctionCollapseSolver::GetWalkableNeighbors(int Index) const
{
	if (!IsCellCollapsed(Index))
	{
		return TArray<int>{};
	}
	TArray<TTuple<EYDWaveFunctionDirection, int>> Neighbors = GetCollapsedNeighbors(Index);
	FGameplayTag Tag = Cells[Index].Options.GetByIndex(0);
	TSet<EYDWaveFunctionDirection> WalkableDirections = Model->Tiles[Tag].WalkDirections;
	TArray<int> WalkableNeighbors;
	for (const auto& Neighbor : Neighbors)
	{
		if (WalkableDirections.Contains(Neighbor.Get<0>()))
		{
			WalkableNeighbors.Add(Neighbor.Get<1>());
		}
	}
	return WalkableNeighbors;
}

TArray<EYDWaveFunctionDirection> UYukiWaveFunctionCollapseSolver::ValidBorders(int Index) const
{
	TArray<EYDWaveFunctionDirection> ValidBorders;
	for (int i = 0; i < (int) EYDWaveFunctionDirection::MAX; i++)
	{
		int NeighborIndex;
		if (!GetNeighborCell(Size, Index, (EYDWaveFunctionDirection) i, NeighborIndex))
		{
			// Direction is a border.
			ValidBorders.Add((EYDWaveFunctionDirection) i);
		}
	}
	return ValidBorders;
}

int UYukiWaveFunctionCollapseSolver::CountCellsWithTag(const FGameplayTag& Tag) const
{
	int OutCount = 0;
	for (int i = 0; i < Cells.Num(); i++)
	{
		if (!IsCellCollapsed(i))
		{
			continue;
		}
		if (Cells[i].Options.HasTag(Tag))
		{
			OutCount++;
		}
	}
	return OutCount;
}

void UYukiWaveFunctionCollapseSolverDecorator_MutuallyExclusive::OnCellCollapsed(int Cell, const FGameplayTag& Tag, UYukiWaveFunctionCollapseSolver* Solver)
{
	if (Tags.HasTag(Tag))
	{
		Solver->RemoveTagsFromUncollapsed(Tags);
	}
}

#undef LOCTEXT_NAMESPACE
