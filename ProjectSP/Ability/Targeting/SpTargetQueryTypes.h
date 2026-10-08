#pragma once

#include "CoreMinimal.h"
#include "SpTargetQueryTypes.generated.h"

class ASpUnit;

// ==================================================

UENUM(BlueprintType)
enum class ESpTargetShape : uint8
{
	Circle,
	Line,
	Sector,
	Box,
	Segment,
};

UENUM(BlueprintType)
enum class ESpTargetRelation : uint8
{
	Hostile,
	Friendly,
	Neutral,
	Self,
	Any,
};

UENUM(BlueprintType)
enum class ESpTargetSort : uint8
{
	None,
	Distance,
	SegmentHitTime,
};

// ------------------------------------------------

USTRUCT(BlueprintType)
struct FSpTargetQuery
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MJ - Targeting")
	ESpTargetShape Shape = ESpTargetShape::Circle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MJ - Targeting")
	ESpTargetRelation Relation = ESpTargetRelation::Hostile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MJ - Targeting")
	ESpTargetSort Sort = ESpTargetSort::Distance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MJ - Targeting")
	FVector Origin = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MJ - Targeting")
	FVector Direction = FVector::ForwardVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MJ - Targeting")
	FVector SegmentEnd = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MJ - Targeting", meta=(ClampMin="0.0"))
	float Radius = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MJ - Targeting", meta=(ClampMin="0.0"))
	float Length = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MJ - Targeting", meta=(ClampMin="0.0"))
	float Width = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MJ - Targeting", meta=(ClampMin="0.0", ClampMax="360.0"))
	float AngleDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MJ - Targeting", meta=(ClampMin="0"))
	int32 MaxTargets = 0; // 0 이하는 무한으로 취급

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MJ - Targeting")
	bool bIncludeTargetCollisionRadius = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MJ - Targeting")
	bool bIncludeSource = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MJ - Targeting")
	bool bRequireTargetable = true;

	// ------------------------------------------------
	
	bool IsValid() const
	{
		switch (Shape)
		{
			case ESpTargetShape::Circle:
				return Radius >= 0.0f;

			case ESpTargetShape::Line:
				return Length > 0.0f && Radius >= 0.0f;

			case ESpTargetShape::Sector:
				return Radius > 0.0f && AngleDegrees > 0.0f;

			case ESpTargetShape::Box:
				return Length > 0.0f && Width > 0.0f;

			case ESpTargetShape::Segment:
				return FVector::DistSquared2D(Origin, SegmentEnd) > UE_KINDA_SMALL_NUMBER && Radius >= 0.0f;
		}

		return false;
	}
};
