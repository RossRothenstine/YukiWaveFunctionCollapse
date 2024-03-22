// Fill out your copyright notice in the Description page of Project Settings.


#include "YukiWaveFunctionCollapseStatics.h"

#include "NavigationTestingActor.h"
#include "YukiWaveFunctionCollapseContainer.h"
#include "Components/WidgetComponent.h"

UYukiWaveFunctionCollapseSolver* UYukiWaveFunctionCollapseStatics::CreateSolverFromModel(UYukiWaveFunctionCollapseModel* Model, FIntVector Size, int32 Seed)
{
	UYukiWaveFunctionCollapseSolver* Solver = NewObject<UYukiWaveFunctionCollapseSolver>();
	Solver->Init(Model, Size, Seed);
	return Solver;
}

AActor* UYukiWaveFunctionCollapseStatics::SpawnActorFromSolver(UObject* WorldContextObject, UYukiWaveFunctionCollapseSolver* Solver)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return nullptr;
	}

	AYukiWaveFunctionCollapseContainer* ContainerActor = World->SpawnActor<AYukiWaveFunctionCollapseContainer>();
	ContainerActor->InitWithSolver(Solver);

	return ContainerActor;
}
