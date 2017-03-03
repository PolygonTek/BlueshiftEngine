//----------------------------------------------------------------------------------
// File:        jni/nv_math/nv_math.cpp
// SDK Version: v10.14 
// Email:       tegradev@nvidia.com
// Site:        http://developer.nvidia.com/
//
// Copyright (c) 2007-2012, NVIDIA CORPORATION.  All rights reserved.
//
// TO  THE MAXIMUM  EXTENT PERMITTED  BY APPLICABLE  LAW, THIS SOFTWARE  IS PROVIDED
// *AS IS*  AND NVIDIA AND  ITS SUPPLIERS DISCLAIM  ALL WARRANTIES,  EITHER  EXPRESS
// OR IMPLIED, INCLUDING, BUT NOT LIMITED  TO, IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL  NVIDIA OR ITS SUPPLIERS
// BE  LIABLE  FOR  ANY  SPECIAL,  INCIDENTAL,  INDIRECT,  OR  CONSEQUENTIAL DAMAGES
// WHATSOEVER (INCLUDING, WITHOUT LIMITATION,  DAMAGES FOR LOSS OF BUSINESS PROFITS,
// BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS)
// ARISING OUT OF THE  USE OF OR INABILITY  TO USE THIS SOFTWARE, EVEN IF NVIDIA HAS
// BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
//
//
//----------------------------------------------------------------------------------

#include "nv_math.h"

static const double a = 16807.0;
static const double m = 2147483647.0;

static double nextSeed(double seed) {
	double t = a * seed;
	return t - m * (double)((int)(t/m));
}

NvF32 NvRandf()
{
    static double seed = 1.0;
    seed = nextSeed(seed);
    return (NvF32) (seed * (1/m));
}

NvF32 NvClockDiffInSecs(long long int newTime, long long int oldTime)
{
    // kdGetTimeUST is never supposed to decrease (i.e. that
    // means it can't wrap, either.  IIRC, it will wrap in
    // 593 years).  So we ignore that option...

    // Used for frame-to-frame diffs, this will fit in 32b
    // as long as the times are < ~4.3s apart
	long long int diffTime = newTime - oldTime;

    // Need to find a better way to do this.
    // However, I believe we will be uSec precise (not run
    // out of mantissa bits for uSecs) as long as the diff
    // is < ~16s  (1e-6 * (2^24))
    return ((NvF32)diffTime) / 1.0e9f;
}
