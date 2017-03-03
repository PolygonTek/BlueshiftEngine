#pragma once

/*
-------------------------------------------------------------------------------

    Physics System

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

class PhysicsSystem {
    friend class PhysCollidable;

public:
    void                    Init();
    void                    Shutdown();

    PhysicsWorld *          AllocPhysicsWorld();
    void                    FreePhysicsWorld(PhysicsWorld *renderWorld);
    
    PhysCollidable *        CreateCollidable(const PhysCollidableDesc *desc);
    void                    DestroyCollidable(PhysCollidable *collidable);
    
    PhysConstraint *        CreateConstraint(const PhysConstraintDesc *desc);
    void                    DestroyConstraint(PhysConstraint *constraint);

    void                    CheckModifiedCVars();

private:
    Array<PhysicsWorld *>   physicsWorlds;
};

extern PhysicsSystem        physicsSystem;

BE_NAMESPACE_END
