﻿// Copyright Timothé Lapetite 2023
// Released under the MIT license https://opensource.org/license/MIT/

#include "Misc/PCGExPartitionByValues.h"
#include "GenericPlatform/GenericPlatformMath.h"
#include "Data/PCGSpatialData.h"
#include "Data/PCGPointData.h"
#include "PCGContext.h"
#include "Elements/Metadata/PCGMetadataElementCommon.h"

#define LOCTEXT_NAMESPACE "PCGExDummyElement"

namespace PCGExPartitionByValues
{
	const FName SourceLabel = TEXT("Source");
}

FPCGElementPtr UPCGExPartitionByValuesSettings::CreateElement() const { return MakeShared<FPCGExPartitionByValuesElement>(); }

PCGEx::EIOInit UPCGExPartitionByValuesSettings::GetPointOutputInitMode() const { return PCGEx::EIOInit::NoOutput; }

bool FPCGExSplitByValuesContext::ValidatePointDataInput(UPCGPointData* PointData)
{
	UPCGExPartitionByValuesSettings* Settings = const_cast<UPCGExPartitionByValuesSettings*>(GetInputSettings<UPCGExPartitionByValuesSettings>());
	return Settings->PartitioningRules.Validate(PointData);
}

FPCGContext* FPCGExPartitionByValuesElement::Initialize(
	const FPCGDataCollection& InputData,
	TWeakObjectPtr<UPCGComponent> SourceComponent,
	const UPCGNode* Node)
{
	FPCGExSplitByValuesContext* Context = new FPCGExSplitByValuesContext();
	InitializeContext(Context, InputData, SourceComponent, Node);
	return Context;
}

void FPCGExPartitionByValuesElement::InitializeContext(
	FPCGExPointsProcessorContext* InContext,
	const FPCGDataCollection& InputData,
	TWeakObjectPtr<UPCGComponent> SourceComponent,
	const UPCGNode* Node) const
{
	FPCGExPointsProcessorElementBase::InitializeContext(InContext, InputData, SourceComponent, Node);
	FPCGExSplitByValuesContext* Context = static_cast<FPCGExSplitByValuesContext*>(InContext);

	const UPCGExPartitionByValuesSettings* Settings = InContext->GetInputSettings<UPCGExPartitionByValuesSettings>();
	check(Settings);

	Context->Partitions = NewObject<UPCGExPointIOGroup>();
	Context->PartitionKeyName = Settings->KeyAttributeName;
	Context->bWritePartitionKey = Settings->bWriteKeyToAttribute;
	Context->PartitionsMap.Empty();
	Context->PartitionRule = Settings->PartitioningRules;

	// ...
}


bool FPCGExPartitionByValuesElement::ExecuteInternal(FPCGContext* InContext) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGExBucketEntryElement::Execute);

	FPCGExSplitByValuesContext* Context = static_cast<FPCGExSplitByValuesContext*>(InContext);

	if (Context->IsCurrentOperation(PCGEx::EOperation::Setup))
	{
		if (Context->Points->IsEmpty())
		{
			PCGE_LOG(Error, GraphAndLog, LOCTEXT("MissingPoints", "Missing Input Points."));
			return true;
		}

		const UPCGExPartitionByValuesSettings* Settings = Context->GetInputSettings<UPCGExPartitionByValuesSettings>();
		check(Settings);

		if (Settings->bWriteKeyToAttribute && !PCGEx::Common::IsValidName(Settings->KeyAttributeName))
		{
			PCGE_LOG(Error, GraphAndLog, LOCTEXT("MalformedAttributeName", "Output Attribute name is invalid."));
			return true;
		}

		Context->SetOperation(PCGEx::EOperation::ReadyForNextPoints);
	}

	////

	bool bProcessingAllowed = false;
	if (Context->IsCurrentOperation(PCGEx::EOperation::ReadyForNextPoints))
	{
		// Start processing all the things
		bProcessingAllowed = true;
	}

	auto InitializeForIO = [&Context](UPCGExPointIO* IO)
	{
		FWriteScopeLock ScopeLock(Context->RulesLock);

		PCGExPartition::FRule& IORule = Context->Rules.Emplace_GetRef(Context->PartitionRule);
		IORule.PrepareForPointData(IO->In);
		Context->RuleMap.Add(IO, &IORule);
		Context->SetOperation(PCGEx::EOperation::ProcessingPoints);
	};

	auto ProcessPoint = [&Context](const FPCGPoint& Point, const int32 Index, UPCGExPointIO* IO)
	{
		FReadScopeLock ScopeLock(Context->RulesLock);
		DistributePoint(Context, IO, Point, (*(Context->RuleMap.Find(IO)))->GetValue(Point));
	};

	if (Context->IsCurrentOperation(PCGEx::EOperation::ProcessingPoints) || bProcessingAllowed)
	{
		if (Context->Points->InputsParallelProcessing(Context, InitializeForIO, ProcessPoint, Context->ChunkSize))
		{
			Context->SetOperation(PCGEx::EOperation::Done);
		}
	}

	if (Context->IsDone())
	{
		Context->Partitions->OutputTo(Context, true);
		return true;
	}

	return false;
}

void FPCGExPartitionByValuesElement::DistributePoint(
	FPCGExSplitByValuesContext* Context,
	UPCGExPointIO* IO,
	const FPCGPoint& Point,
	const double InValue)
{
	const int64 Key = Filter(InValue, Context->PartitionRule);
	UPCGExPointIO* Partition = nullptr;

	{
		FReadScopeLock ScopeLock(Context->PartitionsLock);
		if (UPCGExPointIO** PartitionPtr = Context->PartitionsMap.Find(Key)) { Partition = *PartitionPtr; }
	}

	FPCGMetadataAttribute<int64>* KeyAttribute = nullptr;

	if (!Partition)
	{
		FWriteScopeLock ScopeLock(Context->PartitionsLock);
		Partition = Context->Partitions->Emplace_GetRef(*IO, PCGEx::EIOInit::NewOutput);
		Context->PartitionsMap.Add(Key, Partition);

		if (Context->bWritePartitionKey)
		{
			KeyAttribute = PCGMetadataElementCommon::ClearOrCreateAttribute<int64>(Partition->Out->Metadata, Context->PartitionKeyName, 0);
			if (KeyAttribute) { Context->KeyAttributeMap.Add(Key, KeyAttribute); } //Cache attribute for this partition
		}
	}
	else
	{
		if (Context->bWritePartitionKey)
		{
			FReadScopeLock ScopeLock(Context->PartitionsLock);
			FPCGMetadataAttribute<int64>** KeyAttributePtr = Context->KeyAttributeMap.Find(Key);
			KeyAttribute = KeyAttributePtr ? *KeyAttributePtr : nullptr;
		}
	}

	FWriteScopeLock ScopeLock(Context->PointsLock);
	TArray<FPCGPoint>& Points = Partition->Out->GetMutablePoints();
	FPCGPoint& NewPoint = Points.Add_GetRef(Point);

	if (KeyAttribute)
	{
		Partition->Out->Metadata->InitializeOnSet(NewPoint.MetadataEntry);
		KeyAttribute->SetValue(NewPoint.MetadataEntry, Key);
	}
}

int64 FPCGExPartitionByValuesElement::Filter(const double InValue, const PCGExPartition::FRule& Rule)
{
	const double Upscaled = static_cast<double>(InValue) * Rule.Upscale;
	const double Filtered = (Upscaled - FGenericPlatformMath::Fmod(Upscaled, Rule.FilterSize)) / Rule.FilterSize;
	return static_cast<int64>(Filtered);
}


#undef LOCTEXT_NAMESPACE
#undef PCGEX_PROPERTY_CASE
#undef PCGEX_COMPARE_PROPERTY_CASE
