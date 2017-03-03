#pragma once

#include "Game/GameSettings/GameSettings.h"

BE_NAMESPACE_BEGIN

class TagLayerSettings : public GameSettings {
public:
    enum {
        DefaultLayer,
        UILayer,
        EditorLayer,
    };

    OBJECT_PROTOTYPE(TagLayerSettings);

    TagLayerSettings() {}

    virtual void			Init() override;

    int32_t					FindTag(const char *tagName) const;
    int32_t					FindLayer(const char *layerName) const;

protected:
    void					PropertyChanged(const char *classname, const char *propName);
};

BE_NAMESPACE_END