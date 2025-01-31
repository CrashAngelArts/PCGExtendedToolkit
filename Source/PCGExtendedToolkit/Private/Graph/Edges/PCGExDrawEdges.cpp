﻿// Copyright Timothé Lapetite 2023
// Released under the MIT license https://opensource.org/license/MIT/

#include "Graph/Edges/PCGExDrawEdges.h"

#include "Data/Blending/PCGExMetadataBlender.h"
#include "Graph/Edges/Promoting/PCGExEdgePromoteToPoint.h"

#define LOCTEXT_NAMESPACE "PCGExEdgesToPaths"
#define PCGEX_NAMESPACE DrawEdges

UPCGExDrawEdgesSettings::UPCGExDrawEdgesSettings(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TArray<FPCGPinProperties> UPCGExDrawEdgesSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> NoPins;
	return NoPins;
}

PCGExData::EInit UPCGExDrawEdgesSettings::GetMainOutputInitMode() const { return PCGExData::EInit::NoOutput; }
PCGExData::EInit UPCGExDrawEdgesSettings::GetEdgeOutputInitMode() const { return PCGExData::EInit::NoOutput; }


PCGEX_INITIALIZE_ELEMENT(DrawEdges)

FPCGExDrawEdgesContext::~FPCGExDrawEdgesContext()
{
	PCGEX_TERMINATE_ASYNC
}


bool FPCGExDrawEdgesElement::Boot(FPCGContext* InContext) const
{
	if (!FPCGExEdgesProcessorElement::Boot(InContext)) { return false; }

	PCGEX_CONTEXT_AND_SETTINGS(DrawEdges)

	return true;
}

bool FPCGExDrawEdgesElement::ExecuteInternal(
	FPCGContext* InContext) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGExDrawEdgesElement::Execute);

#if WITH_EDITOR

	PCGEX_CONTEXT_AND_SETTINGS(DrawEdges)

	if (Context->IsSetup())
	{
		if (!Settings->bDebug) { return true; }
		if (!Boot(Context)) { return true; }
		if (!PCGExDebug::NotifyExecute(InContext))
		{
			PCGE_LOG(Error, GraphAndLog, FTEXT("Could not find a PCGEx Debug Manager node in your graph."));
			return true;
		}
		Context->SetState(PCGExMT::State_ReadyForNextPoints);
	}

	if (Context->IsState(PCGExMT::State_ReadyForNextPoints))
	{
		if (!Context->AdvanceAndBindPointsIO()) { Context->Done(); }
		else
		{
			if (!Context->BoundEdges->IsValid())
			{
				PCGE_LOG(Warning, GraphAndLog, FTEXT("Some input points have no bound edges."));
				Context->SetState(PCGExMT::State_ReadyForNextPoints);
			}
			else
			{
				Context->SetState(PCGExGraph::State_ReadyForNextEdges);
			}
		}
	}

	if (Context->IsState(PCGExGraph::State_ReadyForNextEdges))
	{
		while (Context->AdvanceEdges())
		{
			for (const PCGExMesh::FIndexedEdge& Edge : Context->CurrentMesh->Edges)
			{
				if (!Edge.bValid) { continue; }
				FVector Start = Context->CurrentMesh->Vertices[Edge.Start].Position;
				FVector End = Context->CurrentMesh->Vertices[Edge.End].Position;
				DrawDebugLine(Context->World, Start, End, Settings->Color, true, -1, Settings->DepthPriority, Settings->Thickness);
			}
		}

		Context->SetState(PCGExMT::State_ReadyForNextPoints);
	}

	return Context->IsDone();

#endif

	return true;
}

#undef LOCTEXT_NAMESPACE
#undef PCGEX_NAMESPACE
