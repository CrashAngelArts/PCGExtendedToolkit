﻿// Copyright Timothé Lapetite 2023
// Released under the MIT license https://opensource.org/license/MIT/

#pragma once

#include "CoreMinimal.h"

#include "PCGExGraphProcessor.h"

#include "PCGExConsolidateGraph.generated.h"

/**
 * Calculates the distance between two points (inherently a n*n operation)
 */
UCLASS(BlueprintType, ClassGroup = (Procedural), Category="PCGEx|Graph")
class PCGEXTENDEDTOOLKIT_API UPCGExConsolidateGraphSettings : public UPCGExGraphProcessorSettings
{
	GENERATED_BODY()

public:
	//~Begin UPCGSettings interface
#if WITH_EDITOR
	PCGEX_NODE_INFOS(ConsolidateGraph, "Consolidate Graph", "Repairs and consolidate graph indices after points have been removed post graph-building.");
#endif

	/** Compute edge types internally. If you don't need edge types, set it to false to save some cycles.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings)
	bool bConsolidateEdgeType = true;

protected:
	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings interface

	virtual int32 GetPreferredChunkSize() const override;

	virtual PCGExPointIO::EInit GetPointOutputInitMode() const override;

private:
	friend class FPCGExConsolidateGraphElement;
};

struct PCGEXTENDEDTOOLKIT_API FPCGExConsolidateGraphContext : public FPCGExGraphProcessorContext
{
	friend class FPCGExConsolidateGraphElement;

public:
	bool bConsolidateEdgeType;

	TMap<int64, int64> IndicesRemap;
	mutable FRWLock IndicesLock;
};


class PCGEXTENDEDTOOLKIT_API FPCGExConsolidateGraphElement : public FPCGExGraphProcessorElement
{
public:
	virtual FPCGContext* Initialize(
		const FPCGDataCollection& InputData,
		TWeakObjectPtr<UPCGComponent> SourceComponent,
		const UPCGNode* Node) override;

protected:
	virtual bool ExecuteInternal(FPCGContext* InContext) const override;
#if WITH_EDITOR
	static int64 GetFixedIndex(FPCGExConsolidateGraphContext* Context, int64 InIndex);
#endif
};
