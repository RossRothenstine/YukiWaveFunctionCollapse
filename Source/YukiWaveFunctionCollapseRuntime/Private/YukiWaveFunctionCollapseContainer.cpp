// Fill out your copyright notice in the Description page of Project Settings.


#include "YukiWaveFunctionCollapseContainer.h"

#include "YukiWaveFunctionCollapseLog.h"

// Sets default values
AYukiWaveFunctionCollapseContainer::AYukiWaveFunctionCollapseContainer()
{
	SetRootComponent(CreateDefaultSubobject<USceneComponent>("SceneComponent"));
}
void AYukiWaveFunctionCollapseContainer::InitWithSolver(UYukiWaveFunctionCollapseSolver* Solver)
{
	ClearTiles();
	Size = Solver->Size;
	CellSize = Solver->Model->CellSize;
	for (int i = 0; i < Solver->Cells.Num(); i++)
	{
		const FYukiWaveFunctionCollapseCell& Cell = Solver->Cells[i];
		if (Cell.Options.Num() != 1)
		{
			continue;
		}
		const FGameplayTag& Option = Cell.Options.GetByIndex(0);
		if (Option == TAG_Empty)
		{
			continue;
		}
		const FYukiWaveFunctionCollapseTileModel& TileModel = Solver->Model->Tiles[Option];

		int X = i % Size.X;
		int Y = (i / Size.X) % Size.Y;
		int Z = (i / (Size.X * Size.Y) % Size.Z);

		FVector BaseLocation = FVector(X * CellSize, Y * CellSize, Z * CellSize);
		FRotator Rotator = TileModel.Rotation;
		FTransform Transform = FTransform(Rotator, BaseLocation);

		UChildActorComponent* TileActor = NewObject<UChildActorComponent>(this);
		TileActor->SetChildActorClass(TileModel.TileActor.LoadSynchronous());
		AddInstanceComponent(TileActor);
		FinishAddComponent(TileActor, false, Transform);
		Tiles.Add(TileActor);
	}
}

void AYukiWaveFunctionCollapseContainer::ClearTiles()
{
	for (auto It = Tiles.CreateIterator(); It; ++It)
	{
		UActorComponent* Component = *It;
		RemoveInstanceComponent(Component);
		Component->DestroyComponent();
		It.RemoveCurrent();
	}
}
