#pragma once

/*
-------------------------------------------------------------------------------

    Skin

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

#define MAX_SKINS           4096

class Skin {
    friend class SkinManager;

public:
    struct SkinMapping {
        const Material *    from;   // nullptr == any unmatched material
        const Material *    to;
    };

    Skin();
    ~Skin();

    const char *            GetHashName() const { return hashName; }

    bool                    Create(const char *text);

    void                    Purge();

    bool                    Load(const char *name);
    bool                    Reload();
    void                    Write(const char *filename);

    const Skin *            AddRefCount() const { refCount++; return this; }

    const Material *        RemapMaterialBySkin(const Material *material) const;

                            // mesh associations are just for the preview dialog in the editor
    const int               GetNumMeshAssociations() const;
    const char *            GetAssociatedMesh(int index) const;

private:
    Str                     hashName;
    int                     hashKey;
    mutable int             refCount;
    int                     index;
    bool                    permanence;

    Array<SkinMapping>      mappingList;
    StrArray                associatedMeshes;
};

BE_INLINE Skin::Skin() {
    refCount                = 0;
    permanence              = false;
}

BE_INLINE Skin::~Skin() {
    Purge();
}

class SkinManager {
    friend class Skin;

public:
    void                    Init();
    void                    Shutdown();

    Skin *                  AllocSkin(const char *name);
    Skin *                  FindSkin(const char *name) const;
    Skin *                  GetSkin(const char *name);
    
    void                    ReleaseSkin(Skin *skin, bool immediateDestroy = false);
    void                    DestroySkin(Skin *skin);
    void                    DestroyUnusedSkins();

    static Skin *           defaultSkin;

private:
    static void             Cmd_ListSkins(const CmdArgs &args);

    StrIHashMap<Skin *>     skinHashMap;
};

extern SkinManager          skinManager;

BE_NAMESPACE_END
