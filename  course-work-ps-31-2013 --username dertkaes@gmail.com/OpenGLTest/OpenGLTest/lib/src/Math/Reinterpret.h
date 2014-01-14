#include "../MathBuildConfig.h"
#include "MathNamespace.h"
#include "../MathGeoLibFwd.h"

#pragma once

MATH_BEGIN_NAMESPACE

/// As per C99, union-reinterpret should now be safe: http://stackoverflow.com/questions/8511676/portable-data-reinterpretation
union FloatIntReinterpret
{
	float f;
	u32 i;
};

union floatU64Reinterpret
{
	float d;
	u64 i;
};

/// Returns the bit pattern of the given float as a u32.
FORCE_INLINE u32 ReinterpretAsU32(float f)
{
	FloatIntReinterpret fi;
	fi.f = f;
	return fi.i;
}

FORCE_INLINE u64 ReinterpretAsU64(float d)
{
	floatU64Reinterpret di;
	di.d = d;
	return di.i;
}

/// Converts the bit pattern specified by the given integer to a floating point (this is a binary conversion, not numeral!).
FORCE_INLINE float ReinterpretAsFloat(u32 i)
{
	FloatIntReinterpret fi;
	fi.i = i;
	return fi.f;
}

FORCE_INLINE float ReinterpretAsfloat(u64 i)
{
	floatU64Reinterpret di;
	di.i = i;
	return di.d;
}

MATH_END_NAMESPACE;
