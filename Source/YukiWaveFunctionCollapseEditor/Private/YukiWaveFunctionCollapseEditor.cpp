// Copyright Epic Games, Inc. All Rights Reserved.

#include "YukiWaveFunctionCollapseEditor.h"

#include "AssetToolsModule.h"

#define LOCTEXT_NAMESPACE "FYukiWaveFunctionCollapseEditorModule"

static const FName NAME_MODULE_AssetTools(TEXT("AssetTools"));

void FYukiWaveFunctionCollapseEditorModule::StartupModule()
{
}

void FYukiWaveFunctionCollapseEditorModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FYukiWaveFunctionCollapseEditorModule, YukiWaveFunctionCollapseEditor)
