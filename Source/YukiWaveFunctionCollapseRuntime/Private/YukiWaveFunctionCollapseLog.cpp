// Fill out your copyright notice in the Description page of Project Settings.


#include "YukiWaveFunctionCollapseLog.h"

DEFINE_LOG_CATEGORY(LogWFC);

#define LOG_WFG(LogVerbosity, Format, ...) UE_LOG(LogWFC, LogVerbosity, TEXT(Format), ##__VA_ARGS__)
