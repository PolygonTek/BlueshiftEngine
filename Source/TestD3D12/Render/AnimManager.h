// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "Containers/HashIndex.h"
#include "Containers/HashMap.h"
#include "Containers/StrArray.h"
#include "Anim.h"

/*
---------------------------------------------------------------------------------------------------

    Anim Manager

---------------------------------------------------------------------------------------------------
*/

class AnimManager {
public:
    void                            Init();
    void                            Shutdown();

    void                            UpdateFrame();

    SharedPtr<Anim>                 AllocAnim(const char *hashName);
    SharedPtr<Anim>                 FindAnim(const char *hashName) const;
    SharedPtr<Anim>                 GetAnim(const char *hashName, bool async = true);
    SharedPtr<Anim>                 GetDefaultAnim(const char *hashName, const Skeleton *skeleton);

    int                             JointIndexByName(const char *jointName);
    const char *                    JointNameByIndex(int index) const;

private:
    void                            CleanExpiredAnims();

    BE1::StrIHashMap<WeakPtr<Anim>> animHashMap;
    int                             expiredAnimHashMapCursor = 0;

    BE1::StrArray                   jointNames;
    BE1::HashIndex                  jointNameHash;
};

extern AnimManager                  animManager;
