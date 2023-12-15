﻿// Copyright Timothé Lapetite 2023
// Released under the MIT license https://opensource.org/license/MIT/

#pragma once

#include "CoreMinimal.h"
#include "PCGExGraphProcessor.h"
#include "Promotions/PCGExEdgePromotion.h"
#include "PCGExWriteEdgeExtras.generated.h"

UCLASS(BlueprintType, ClassGroup = (Procedural), Category="PCGEx|Graph")
class PCGEXTENDEDTOOLKIT_API UPCGExWriteEdgeExtrasSettings : public UPCGExGraphProcessorSettings
{
	GENERATED_BODY()

public:
	UPCGExWriteEdgeExtrasSettings(const FObjectInitializer& ObjectInitializer);

	//~Begin UPCGSettings interface
#if WITH_EDITOR
	PCGEX_NODE_INFOS(WriteEdgeExtras, "Promote Edges", "Promote graph edges to points.");
	virtual FLinearColor GetNodeTitleColor() const override { return PCGEx::NodeColorPathfinding; }
#endif

	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;

protected:
	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings interface

	virtual FName GetMainOutputLabel() const override;

public:
	virtual int32 GetPreferredChunkSize() const override;
	virtual PCGExData::EInit GetMainOutputInitMode() const override;

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(Bitmask, BitmaskEnum="/Script/PCGExtendedToolkit.EPCGExEdgeType"))
	uint8 EdgeType = static_cast<uint8>(EPCGExEdgeType::Complete);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, Instanced)
	UPCGExEdgePromotion* Promotion;

private:
	friend class FPCGExWriteEdgeExtrasElement;
};

struct PCGEXTENDEDTOOLKIT_API FPCGExWriteEdgeExtrasContext : public FPCGExGraphProcessorContext
{
	friend class FPCGExWriteEdgeExtrasElement;

public:
	EPCGExEdgeType EdgeType;
	int32 MaxPossibleEdgesPerPoint = 0;
	TSet<uint64> UniqueEdges;
	TArray<PCGExGraph::FUnsignedEdge> Edges;

	mutable FRWLock EdgeLock;

	UPCGExEdgePromotion* Promotion;
};

class PCGEXTENDEDTOOLKIT_API FPCGExWriteEdgeExtrasElement : public FPCGExGraphProcessorElement
{
public:
	virtual FPCGContext* Initialize(
		const FPCGDataCollection& InputData,
		TWeakObjectPtr<UPCGComponent> SourceComponent,
		const UPCGNode* Node) override;

protected:
	virtual bool Boot(FPCGContext* InContext) const override;
	virtual bool ExecuteInternal(FPCGContext* InContext) const override;
};
