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

#include "Containers/HashMap.h"
#include "Skeleton.h"

namespace BE1 {
class CmdArgs;
}

/*
---------------------------------------------------------------------------------------------------

    Skeleton Manager

---------------------------------------------------------------------------------------------------
*/

class SkeletonManager {
    friend class Skeleton;

public:
    void                            Init();
    void                            Shutdown();

    void                            UpdateFrame();

    SharedPtr<Skeleton>             AllocSkeleton(const char *name);
    SharedPtr<Skeleton>             FindSkeleton(const char *name) const;
    SharedPtr<Skeleton>             GetSkeleton(const char *name, bool async = true);

    void                            RenameSkeleton(Skeleton *skeleton, const BE1::Str &newName);

    SharedPtr<Skeleton>             defaultSkeleton;

private:
    void                            CleanExpiredSkeletons();

    static void                     Cmd_ListSkeletons(const BE1::CmdArgs &args);
    static void                     Cmd_ReloadSkeleton(const BE1::CmdArgs &args);

    BE1::StrIHashMap<WeakPtr<Skeleton>> skeletonHashMap;
    int                             expiredSkeletonHashMapCursor = 0;
};

extern SkeletonManager              skeletonManager;
