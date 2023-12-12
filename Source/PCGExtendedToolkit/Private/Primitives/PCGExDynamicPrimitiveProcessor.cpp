﻿// Copyright Timothé Lapetite 2023
// Released under the MIT license https://opensource.org/license/MIT/

#include "Primitives/PCGExDynamicPrimitiveProcessor.h"

#define LOCTEXT_NAMESPACE "PCGExDynamicPrimitiveProcessorElement"

UPCGExDynamicPrimitiveProcessorSettings::UPCGExDynamicPrimitiveProcessorSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

PCGExData::EInit UPCGExDynamicPrimitiveProcessorSettings::GetPointOutputInitMode() const { return PCGExData::EInit::NoOutput; }

TArray<FPCGPinProperties> UPCGExDynamicPrimitiveProcessorSettings::InputPinProperties() const
{
	TArray<FPCGPinProperties> Empty;
	return Empty;
}

PCGEX_INITIALIZE_CONTEXT(DynamicPrimitiveProcessor)

#undef LOCTEXT_NAMESPACE
