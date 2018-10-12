// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

BE_NAMESPACE_BEGIN

/*
-------------------------------------------------------------------------------

    Hammersley-Halton Sequence

-------------------------------------------------------------------------------
*/

template <int N>
class Hammersley {
public:
    Hammersley();

                    /// Casts this to a C array.
                    /// This function simply returns a C pointer view to this data structure.
    const float *   Ptr() const { return (const float *)&samples[0].x; }
    float *         Ptr() { return (float *)&samples[0].x; }

    int             NumSamples() const { return N; }

    Vec2            GetSample(int index) const;

private:
    float           RadicalInverse(int index, int base) const;

    Vec2            samples[N];
};

template <int N>
BE_INLINE Hammersley<N>::Hammersley() {
    float rndX = Math::Random(0.0f, 1.0f);
    float rndY = Math::Random(0.0f, 1.0f);

    for (int sampleIndex = 0; sampleIndex < N; sampleIndex++) {
        samples[sampleIndex].x = Math::Fract(RadicalInverse(sampleIndex, 2) + rndX);
        samples[sampleIndex].y = Math::Fract(RadicalInverse(sampleIndex, 7) + rndY);
    }
}

// Radical Inverse function for Hammersley and Halton squences
template <int N>
BE_INLINE float Hammersley<N>::RadicalInverse(int index, int base) const {
    float radix = (float)base;
    float x = 0.0f;

    while (index) {
        int a = index % base;
        x += a / radix;
        index = (int)((float)index / (float)base);
        radix *= base;
    }

    return x;
}

template <int N>
BE_INLINE Vec2 Hammersley<N>::GetSample(int index) const {
    return samples[index % N];
}

BE_NAMESPACE_END