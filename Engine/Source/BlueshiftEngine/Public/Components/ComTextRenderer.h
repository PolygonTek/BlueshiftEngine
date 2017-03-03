#pragma once

#include "ComRenderable.h"

BE_NAMESPACE_BEGIN

class ComTextRenderer : public ComRenderable {
public:
    OBJECT_PROTOTYPE(ComTextRenderer);

    ComTextRenderer();
    virtual ~ComTextRenderer();

    virtual void            Purge(bool chainPurge = true) override;

    virtual void            Init() override;

    void                    SetText(const char *text);

protected:
    void                    ChangeFont(const Guid fontGuid, int fontSize);

    void                    UpdateAABB();

    void                    PropertyChanged(const char *classname, const char *propName);
};

BE_NAMESPACE_END
