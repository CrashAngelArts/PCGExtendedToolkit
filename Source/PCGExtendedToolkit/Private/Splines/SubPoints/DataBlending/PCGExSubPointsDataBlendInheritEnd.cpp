﻿// Copyright Timothé Lapetite 2023
// Released under the MIT license https://opensource.org/license/MIT/


#include "Splines/SubPoints/DataBlending/PCGExSubPointsDataBlendInheritEnd.h"

void UPCGExSubPointsDataBlendInheritEnd::ProcessSubPoints(const FPCGPoint& StartPoint, const FPCGPoint& EndPoint, TArrayView<FPCGPoint>& SubPoints, const double PathLength) const
{
	const int32 NumPoints = SubPoints.Num();
	for (int i = 0; i < NumPoints; i++)
	{
		FPCGPoint& Point = SubPoints[i];
		const FVector Location = Point.Transform.GetLocation();
		
		PCGExMath::Copy(EndPoint, Point);
		AttributeMap->SetCopy(EndPoint.MetadataEntry, Point.MetadataEntry);
		
		Point.Transform.SetLocation(Location); // !
	}
}
