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

#include "Precompiled.h"
#include "Input/InputSystem.h"
#include "Game/InputUtils.h"
#include "Game/Entity.h"
#include "Components/ComScript.h"

BE_NAMESPACE_BEGIN

bool InputUtils::ProcessMousePointerInput(PointerState &mousePointerState, const std::function<Entity *(const Point &)> &rayCastFunc) {
    if (!inputSystem.IsMouseExist()) {
        return false;
    }

    Entity *hitTestEntity = rayCastFunc(inputSystem.GetMousePos());

    if (inputSystem.IsKeyUp(KeyCode::Mouse1)) {
        Entity *captureEntity = (Entity *)Entity::FindInstance(mousePointerState.captureEntityGuid);
        if (captureEntity) {
            ComponentPtrArray scriptComponents = captureEntity->GetComponents(&ComScript::metaObject);

            for (int i = 0; i < scriptComponents.Count(); i++) {
                ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();

                scriptComponent->OnPointerUp();

                if (hitTestEntity == captureEntity) {
                    scriptComponent->OnPointerClick();
                }
            }
        }

        mousePointerState.captureEntityGuid = Guid::zero;
    }

    Entity *oldHitEntity = (Entity *)Entity::FindInstance(mousePointerState.oldHitEntityGuid);

    if (oldHitEntity) {
        ComponentPtrArray scriptComponents = oldHitEntity->GetComponents(&ComScript::metaObject);

        for (int i = 0; i < scriptComponents.Count(); i++) {
            ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();

            if (oldHitEntity == hitTestEntity) {
                scriptComponent->OnPointerOver();
            } else {
                scriptComponent->OnPointerExit();
            }
        }
    }

    if (hitTestEntity) {
        ComponentPtrArray scriptComponents = hitTestEntity->GetComponents(&ComScript::metaObject);

        for (int i = 0; i < scriptComponents.Count(); i++) {
            ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();

            if (hitTestEntity != oldHitEntity) {
                scriptComponent->OnPointerEnter();
            }

            if (inputSystem.IsKeyDown(KeyCode::Mouse1)) {
                scriptComponent->OnPointerDown();
            } else if (inputSystem.IsKeyPressed(KeyCode::Mouse1)) {
                scriptComponent->OnPointerDrag();
            }
        }

        if (inputSystem.IsKeyDown(KeyCode::Mouse1)) {
            mousePointerState.captureEntityGuid = hitTestEntity->GetGuid();
        }

        mousePointerState.oldHitEntityGuid = hitTestEntity->GetGuid();
    } else {
        mousePointerState.oldHitEntityGuid = Guid::zero;
    }

    return (hitTestEntity || !mousePointerState.captureEntityGuid.IsZero());
}

bool InputUtils::ProcessTouchPointerInput(HashTable<int32_t, PointerState> &touchPointerStateTable, const std::function<Entity *(const Point &)> &rayCastFunc) {
    bool processed = false;

    for (int touchIndex = 0; touchIndex < inputSystem.GetTouchCount(); touchIndex++) {
        InputSystem::Touch touch = inputSystem.GetTouch(touchIndex);

        Entity *hitTestEntity = nullptr;

        if (touch.phase == InputSystem::Touch::Phase::Started ||
            touch.phase == InputSystem::Touch::Phase::Ended ||
            touch.phase == InputSystem::Touch::Phase::Moved) {
            hitTestEntity = rayCastFunc(touch.position);
        }

        if (touch.phase == InputSystem::Touch::Phase::Started) {
            if (hitTestEntity) {
                processed = true;

                PointerState touchPointerState;
                touchPointerState.oldHitEntityGuid = hitTestEntity->GetGuid();
                touchPointerState.captureEntityGuid = hitTestEntity->GetGuid();

                touchPointerStateTable.Set(touch.id, touchPointerState);

                ComponentPtrArray scriptComponents = hitTestEntity->GetComponents(&ComScript::metaObject);

                for (int i = 0; i < scriptComponents.Count(); i++) {
                    ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();

                    scriptComponent->OnPointerEnter();
                    scriptComponent->OnPointerDown();
                }
            }
        } else if (touch.phase == InputSystem::Touch::Phase::Ended || touch.phase == InputSystem::Touch::Phase::Canceled) {
            PointerState touchPointerState;

            if (touchPointerStateTable.Get(touch.id, &touchPointerState)) {
                Entity *capturedEntity = (Entity *)Entity::FindInstance(touchPointerState.captureEntityGuid);

                if (capturedEntity) {
                    processed = true;

                    ComponentPtrArray scriptComponents = capturedEntity->GetComponents(&ComScript::metaObject);

                    for (int i = 0; i < scriptComponents.Count(); i++) {
                        ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();

                        scriptComponent->OnPointerUp();

                        if (touch.phase == InputSystem::Touch::Phase::Ended && hitTestEntity == capturedEntity) {
                            scriptComponent->OnPointerClick();
                        }

                        scriptComponent->OnPointerExit();
                    }
                }

                touchPointerStateTable.Remove(touch.id);
            }
        } else if (touch.phase == InputSystem::Touch::Phase::Moved) {
            PointerState touchPointerState;

            if (touchPointerStateTable.Get(touch.id, &touchPointerState)) {
                Entity *capturedEntity = (Entity *)Entity::FindInstance(touchPointerState.captureEntityGuid);
                Entity *oldHitEntity = (Entity *)Entity::FindInstance(touchPointerState.oldHitEntityGuid);

                if (capturedEntity) {
                    processed = true;

                    ComponentPtrArray scriptComponents = capturedEntity->GetComponents(&ComScript::metaObject);

                    for (int i = 0; i < scriptComponents.Count(); i++) {
                        ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();

                        if (hitTestEntity != oldHitEntity) {
                            if (hitTestEntity == capturedEntity) {
                                scriptComponent->OnPointerEnter();
                            } else if (oldHitEntity == capturedEntity) {
                                scriptComponent->OnPointerExit();
                            }
                        }

                        scriptComponent->OnPointerDrag();
                    }
                }

                touchPointerState.oldHitEntityGuid = hitTestEntity ? hitTestEntity->GetGuid() : Guid::zero;
                touchPointerStateTable.Set(touch.id, touchPointerState);
            }
        }
    }

    return processed;
}

BE_NAMESPACE_END
