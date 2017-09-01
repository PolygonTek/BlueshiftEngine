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

/*
-------------------------------------------------------------------------------

    Spherical Harmonics

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

class SphericalHarmonics {
public:
                    // Evaluates n'th order SH basis function with the given z-up direction
    static void     EvalBasis(int n, const Vec3 &dir, float *basisProj);

                    // Returns sqrt(4PI/(2l + 1)) * projected ZH Lambert function cos(theta) / Pi 
    static double   Lambert_Al_Evaluator(int l);
};

BE_NAMESPACE_END
