#pragma once

#include "ComMeshRenderer.h"

BE_NAMESPACE_BEGIN

class ComStaticMeshRenderer : public ComMeshRenderer {
public:
    OBJECT_PROTOTYPE(ComStaticMeshRenderer);

    ComStaticMeshRenderer();
    virtual ~ComStaticMeshRenderer();

    virtual bool            IsConflictComponent(const MetaObject &componentClass) const override;

    virtual void            Purge(bool chainPurge = true) override;

    virtual void            Init() override;

    virtual void            Update() override;

protected:
    virtual void            MeshUpdated() override;

    void                    PropertyChanged(const char *classname, const char *propName);
};

BE_NAMESPACE_END
