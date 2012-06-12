/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

Cafu is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

Cafu is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Cafu. If not, see <http://www.gnu.org/licenses/>.

For support and more information about Cafu, visit us at <http://www.cafu.de>.
=================================================================================
*/

#ifndef CAFU_GAMEDLL_HPP_INCLUDED
#define CAFU_GAMEDLL_HPP_INCLUDED

#include "Math3D/BoundingBox.hpp"
#include "ClipSys/ClipModel.hpp"

#include <map>


class EntityCreateParamsT;
class PhysicsWorldT;
struct lua_State;
namespace cf { namespace ClipSys { class CollisionModelT; } }
namespace cf { namespace GameSys { class GameWorldI; } }
namespace cf { namespace Network { class InStreamT; } }
namespace cf { namespace Network { class OutStreamT; } }
namespace cf { namespace TypeSys { class TypeInfoT; } }
namespace cf { namespace TypeSys { class TypeInfoManT; } }
namespace cf { namespace TypeSys { class CreateParamsT; } }


/// The TypeInfoTs of all BaseEntityT derived classes must register with this TypeInfoManT instance.
cf::TypeSys::TypeInfoManT& GetBaseEntTIM();


// This structure describes each entitys state and is transmitted from the server to the clients over the network.
struct EntityStateT
{
    /***************************************************************************************************************/
    /*** EntityStateT field changes FORCE protocol changes in ALL places that are marked *like this* (use grep)! ***/
    /***************************************************************************************************************/

    VectorT               Origin;           // World coordinate of (the eye of) this entity.
    VectorT               Velocity;         // Velocity of this entity.
    BoundingBox3T<double> Dimensions;       // The bounding box of this entity (relative to the eye).

    unsigned short Heading;                 // Heading (North is down the ??-axis).
    unsigned short Pitch;                   // Pitch (for looking up/down).
    unsigned short Bank;                    // Bank (e.g. used when dead and lying on the side).

    char           StateOfExistance;        // For entity defined state machines, e.g. "specator, dead, alive, ...".
    char           Flags;                   // Entity defined flags.
    char           PlayerName[64];          // If it is a human player, this is its name. Usually unused otherwise.
 // ArrayT<char>   PlayerName;
    char           ModelIndex;              // An arbitrary (game defined) number that describes the BODY model of this entity (that OTHERS see).
    char           ModelSequNr;             // The sequence number of the body model.
    float          ModelFrameNr;            // The frame number of the current sequence of the body model.

    short          Health;                  // Health.
    char           Armor;                   // Armor.
    unsigned long  HaveItems;               // Bit field, entity can carry 32 different items.
    unsigned long  HaveWeapons;             // Bit field, entity can carry 32 different weapons.
    char           ActiveWeaponSlot;        // Index into HaveWeapons, HaveAmmoInWeapons, and for determining the weapon model index.
    char           ActiveWeaponSequNr;      // The weapon sequ. that WE see (the LOCAL clients VIEW weapon model). Could (and should in the future) also be used for other clients PLAYER weapon model, but currently is not.
    float          ActiveWeaponFrameNr;     // Respectively, this is the frame number of the current weapon sequence.
    unsigned short HaveAmmo[16];            // Entity can carry 16 different types of ammo (weapon independent). This is the amount of each.
    unsigned char  HaveAmmoInWeapons[32];   // Entity can carry ammo in each of the 32 weapons. This is the amount of each.

    // Special variable used to notify clients about events.
    // On server side, in 'BaseEntityT::Think()', use bit-wise XOR to toggle event flags (as in 'State.Events^=MY_EVENT_BIT').
    // On client side, the function 'BaseEntityT::ProcessEvent(i)' is automatically called for each toggled bit i.
    // TODO: This should be hidden in 'EngineEntityT', as 'Events' receives special treatment in the engine.
    //       At that opportunity, also think about a proper fixed-point presentation of 'Origin', 'Velocity', ...
    //       (Or, more generally, about the convenient types here and the requ. precision across the net.)
    unsigned long  Events;

    EntityStateT(const VectorT& Origin_, const VectorT& Velocity_, const BoundingBox3T<double>& Dimensions_,
                 unsigned short Heading_, unsigned short Pitch_, unsigned short Bank_,
                 char StateOfExistance_, char Flags_, char ModelIndex_, char ModelSequNr_, float ModelFrameNr_,
                 short Health_, char Armor_, unsigned long HaveItems_, unsigned long HaveWeapons_,
                 char ActiveWeaponSlot_, char ActiveWeaponSequNr_, float ActiveWeaponFrameNr_);
};


// This class describes "base entities", the most central component in game<-->engine communication.
class BaseEntityT
{
    public:

    const unsigned long ID;             // The unique ID of this entity.
    const std::string   Name;           ///< The unique instance name of this entity, normally equal to and obtained from Properties["name"]. It's explicitly kept (possibly redundantly to Properties["name"]), because with it: a) there is no confusion when "name" is not found in the Properties list, b) its const-ness is more obvious and easier to guarantee (e.g. the ScriptStateT class relies on the name never changing throughout entity lifetime, because script objects are addressed and found by name, not by instance pointer (light userdata)!), and c) access like   MyEnt->Name   is much easier to write than using Properties.find().
    const std::map<std::string, std::string> Properties;    ///< The properties of this entities from the map file. THIS IS UNSAFE due to the EXE/DLL boundaries (if a const operation modified memory...), but for now it seems to work on all STL implementations - TODO: revision required later!!!
    const unsigned long WorldFileIndex; // The index of this entity into the array in the world file, -1 is no such information exists. See [1].
    unsigned long       ParentID;       // The 'ID' of the entity that created us.
 // ID[]                ChildrenIDs;    // The entities that we have created (e.g. the rockets that a human player fired).
    EntityStateT        State;          // The current state of this entity.

    cf::GameSys::GameWorldI*            GameWorld;      ///< Pointer to the game world implementation.
    PhysicsWorldT*                      PhysicsWorld;   ///< Pointer to the physics world implementation.
    const cf::ClipSys::CollisionModelT* CollisionModel; ///< The collision model of this entity, NULL for none.
    cf::ClipSys::ClipModelT             ClipModel;      ///< The clip model of this entity. Note that clip models can take NULL collision model pointers, so that the ClipModel instance is always non-NULL and available.

    // [1]  I have been trying hard to save this variable, because 'ID' seems to handle the same purpose.
    // This would mean to overload 'ID' with multiple purposes though, and that doesn't seem right to me:
    // a) Care must be taken that 'ID's are always unique (never get re-used), but this should be the case anyway.
    // b) Worse is the implied order assumption (both on server and on *CLIENT* side):
    //    In order to conclude from the 'ID' to the (no-longer-present) 'MapFileID', assumptions must hold that are hard and dangerous to enforce.
    //    For example, 'ID==MapFileID' must hold for all entities that have map file information associated with them.
    // c) Even if all that worked, it would e.g. be impossible to duplicate such an entity.
    // d) Moreover, it isn't worth the effort: 'MapFileID' causes network load only once, during the baseline message for this entity.
    //    That doesn't happen all that often, so I rather keep it than introducing a lot of complicated dependencies and problems.


    /// The destructor.
    virtual ~BaseEntityT();

    /// Writes the current state of this entity into the given stream.
    /// This method is called to send the state of the entity over the network or to save it to disk.
    /// Note that this method is the twin of Deserialize(), whose implementation it must match.
    virtual void Serialize(cf::Network::OutStreamT& Stream) const;

    /// Reads the state of this entity from the given stream, and updates the entity accordingly.
    /// This method is called after the state of the entity has been received over the network,
    /// has been loaded from disk or must be "reset" for the purpose of (re-)prediction.
    /// Note that this method is the twin of Serialize(), whose implementation it must match.
    ///
    /// @param IsIniting   Only used by the ctor implementation: Set to \c true in order to indicate
    ///     that the entity is newly constructed. User code should always leave this at \c false.
    virtual void Deserialize(cf::Network::InStreamT& Stream, bool IsIniting=false);

    /// Returns the origin point of this entity. Used for
    ///   - obtaining the camera position of the local human player entity (1st person view),
    ///   - interpolating the origin (NPC entities) and
    ///   - computing light source positions.
    virtual const Vector3dT& GetOrigin() const { return State.Origin; }

    /// This method is a temporary hack, used so that the caller can temporarily set the origin to the "interpolated" origin,
    /// draw the entity or get the entity light source position, then set the origin back to the previous origin.
    /// Interpolation of client entities needs to be thoroughly redone anyway (e.g. combined with interpolating animation sequences),
    /// this is also when this method will be removed again.
    void SetInterpolationOrigin(const Vector3dT& O) { State.Origin=O; }

    /// Returns the dimensions of this entity.
    virtual const BoundingBox3dT& GetDimensions() const { return State.Dimensions; }

    /// Returns the camera orientation angles of this entity.
    /// Used for setting up the camera of the local human player entity (1st person view).
    virtual void GetCameraOrientation(unsigned short& h, unsigned short& p, unsigned short& b) const { h=State.Heading; p=State.Pitch; b=State.Bank; }

    /// Returns the orientation angles of the entity itself.
    /// Used for computing the light source and eye positions in entity (model) space.
    /// TODO: Both the signature as well as the implementation of this method are temporary, and fully expected to change later.
    virtual void GetBodyOrientation(unsigned short& h, unsigned short& p, unsigned short& b) const { h=State.Heading; p=State.Pitch; b=State.Bank; }


    // Some convenience functions for reading the Properties.
    // Actually, the Properties should be replaced by a PropDictT class that has these functions as its methods...
    float       GetProp(const std::string& Key, float       Default=0.0f) const;
    double      GetProp(const std::string& Key, double      Default=0.0) const;
    int         GetProp(const std::string& Key, int         Default=0) const;
    std::string GetProp(const std::string& Key, std::string Default="") const;
    Vector3fT   GetProp(const std::string& Key, Vector3fT   Default=Vector3fT()) const;
    Vector3dT   GetProp(const std::string& Key, Vector3dT   Default=Vector3dT()) const;


    /// This SERVER-SIDE function is used to notify this entity that it was touched by another entity.
    /// 'Entity' is the entity that touches this one, and is usually the entity from which the call is made.
    /// Calls are only made from within other entities 'Think()' functions.
    /// (Unfortunately, this function cannot be declared as "protected": see "C++ FAQs, 2nd edition" by Cline, Lomow, Girou, pages 249f.)
    virtual void NotifyTouchedBy(BaseEntityT* Entity);

    /// This SERVER-SIDE method is called whenever another entity walked into one of our trigger volumes (trigger brushes).
    /// It is called once per frame as long as the other entity stays within our trigger volume.
    ///
    /// @param Activator   The entity that walked into our trigger volume and thus caused the call of this method.
    ///
    /// Note that most entity classes just do nothing in their implementation of this method, usually because they never have trigger brushes anyway.
    /// Entity classes that are very likely to provide an implementation though are EntTriggerT (of course!) and the items like weapons,
    /// so they can e.g. detect being picked up.
    /// Calls to this method normally come from within the Activator->Think() method, because it are the activators themselves that detect
    /// that they entered a trigger volume (it are not the entities with the triggers that detect that something entered their volume).
    /// (Unfortunately, this function cannot be declared as "protected": see "C++ FAQs, 2nd edition" by Cline, Lomow, Girou, pages 249f.)
    virtual void OnTrigger(BaseEntityT* Activator);

    /// This SERVER-SIDE method is called whenever another entity pushes us.
    /// Whenever an entity moves, it may want to push other entities that are touching it.
    /// Examples are lifts that give the objects on it a ride, or players that push barrels or crates.
    /// As the pusher itself may be pushed (e.g. when a player pushes a barrel that pushes a barrel or when a lift pushes a crate
    /// that pushes the player on top of it), there can be an entire chain of pushers.
    ///
    /// @param Pushers
    ///   The chain of pushers that are pushing this entity. Usually, there is only one element in this list,
    ///   but in theory, e.g. when a train pushes a waggon that pushes a waggon that pushes a waggon..., the list can be
    ///   arbitrarily long.
    ///   The topmost pusher, i.e. Pushers[i] with i==Pushers.Size()-1, is our immediate pusher, Pushers[i-1] is the pusher of our pusher,
    ///   and so forth. Pushers[0] is the source pusher of the entire chain.
    ///   Note that the Pushers list must always be kept acyclic, i.e. each entity must occur in the list at most once,
    ///   or otherwise there is momentum to infinite recursion. For example, the last waggon of as train must not also be a pusher of the train;
    ///   when a T-shaped object pushes a U-shaped object sideways (with its vertical bar in the center of the U), the U must not also push the T, etc.
    ///
    /// @param PushVector
    ///   This vector describes the desired and required direction and length of the push.
    virtual void OnPush(ArrayT<BaseEntityT*>& Pushers, const Vector3dT& PushVector);

    /// This SERVER-SIDE function is used to have this entity take damage.
    /// 'Entity' is the entitiy that causes the damage (i.e., who fired a shot). It is usually the entity from which the call is made.
    /// 'Amount' is the amount of damage that was caused, and is usually subtracted from this entitys health.
    /// 'ImpactDir' is the direction from which we were hit.
    /// Calls are only made from within other entities 'Think()' functions.
    /// (Unfortunately, this function cannot be declared as "protected": see "C++ FAQs, 2nd edition" by Cline, Lomow, Girou, pages 249f.)
    virtual void TakeDamage(BaseEntityT* Entity, char Amount, const VectorT& ImpactDir);

    /// THIS FUNCTION IS DEPRECATED! TRY TO AVOID TO USE IT!
    /// This DEPRECATED, SERVER-SIDE function is called in order to "communicate" with this entity.
    /// It is probably a (bad) replacement for additional, missing member functions, so don't use it!
    /// Mostly used for setting/querying the properties of specific, concrete entities,
    /// e.g. the player names, player model names, player commands, or if something is solid,
    /// alive or was picked up (and is thus "invisible" for a while), and so on.
    /// This should most certainly be resolved as soon as possible, requires careful design considerations, however.
    /// At a first glance, the related methods are *only* called from the server (like passing in player commands),
    /// or from within the Think() functions, but never on the client side.
    virtual void ProcessConfigString(const void* ConfigData, const char* ConfigString);

    /// This SERVER-SIDE function is called by the server in order to advance the world one clock-tick.
    /// That is, basing on the present (old) 'State', it is called for computing the next (new) state.
    /// 'FrameTime' is the time of the clock-tick, in seconds.
    /// 'ServerFrameNr' is the number of the current server frame.
    /// >>> IMPORTANT NOTE: In truth, also the CLIENT-SIDE calls this function for the purpose of predicting the local human player entity! <<<
    /// >>> As a consequence, special rules apply when this function is called for predicted entities (that is, human player entities).     <<<
    /// >>> For further details and examples, please refer to the EntHumanPlayerT::Think() function in HumanPlayer.cpp.                     <<<
    virtual void Think(float FrameTime, unsigned long ServerFrameNr);


    /// The client calls this method when it received an update of this entitys state from the server.
    // /// @param NetMsg   The network message that contains the update.
    virtual void Cl_UnserializeFrom(/*TODO: Add NetMsg param here.*/);

    /// This CLIENT-SIDE function is called exactly once per received event (on client-side).
    /// Usually, the server code (in 'Think()') will trigger events like "fire current weapon"
    /// (using event ID 'EventID'), and then this is the place to react accordingly.
    /// Note that both the name and the prototype of this function are subject to change in the future,
    /// in order to take also more flexible, custom network messages into account.
    ///
    /// Notes for future prototypes:
    ///   - What's the difference between events and custom network messages?
    ///     Events are fully predictable, i.e. they work well even in the presence of client prediction,
    ///     and are always related to a certain entity.
    ///     Custom network messages are (currently) not predictable, and not necessarily associated with an entity.
    ///     Examples for custom messages include radar displays, and so on.
    virtual void ProcessEvent(char EventID);

    /// This CLIENT-SIDE function is called in order to retrieve light source information about this entity.
    /// Returns 'true' if this entity is a light source and 'DiffuseColor', 'SpecularColor', 'Position' (in world space!), 'Radius' and 'CastsShadows' have been set.
    /// Returns 'false' if this entity is no light source.
    /// (In theory, this function might also be called on server-side, from within Think().)
    virtual bool GetLightSourceInfo(unsigned long& DiffuseColor, unsigned long& SpecularColor, VectorT& Position, float& Radius, bool& CastsShadows) const;

    /// This CLIENT-SIDE method determines whether the engine should interpolate the origin (State.Origin) of this entity for drawing.
    /// The normal return value is true, but e.g. rigid bodies (EntRigidBodyT) use some variables in the State member in a fashion
    /// that doesn't work with the engines interpolation, so they can turn if off by overriding this method.
    /// (This in turn makes this method a kind of hack for the EntRigidBodyT class...)
    virtual bool DrawInterpolated() const;

    /// This CLIENT-SIDE function is called by the client in order to get this entity drawn.
    ///
    /// Note that it is usually called several times per frame, in order to gather the individual terms of the lighting equation:
    /// there is a call for the ambient contribution (everything that is independent of a lightsource) and for each light source there is
    /// a call for the shadows, followed by another call for adding the lightsource-dependent contribution (diffuse and specular terms etc.).
    ///
    /// Also note that the calling code has properly set up the Cafu Material Systems global lighting parameters before calling.
    /// That is, the ambient light color, light source position (in model space), radius, diff+spec color and the eye position (in model space) are set.
    /// However, normally only those parameters that are relevant for the current Material Systems rendering action are set:
    /// In the AMBIENT rendering action, only the ambient colors are set, in the STENCILSHADOW action only the light source position may be set,
    /// and in the LIGHTING action all parameters except for the ambient light color are set.
    ///
    /// @param FirstPersonView   is true when the engine has rendered the world from this entities viewpoint, e.g. because this is the local players entity.
    /// @param LodDist           is the world-space distance of the entity to the viewer, supposed to be used for level-of-detail reductions.
    virtual void Draw(bool FirstPersonView, float LodDist) const;

    /// This CLIENT-SIDE function is called once per frame, for each entity, after the regular rendering (calls to 'Draw()') is completed,
    /// in order to provide entities an opportunity to render the HUD, employ simple "mini-prediction", triggers sounds,
    /// register particles, do other server-independent eye-candy, and so on.
    /// 'FrameTime' is the time in seconds that it took to render the last (previous) frame.
    /// 'FirstPersonView' is true when the engine has rendered the world from this entities viewpoint, e.g. because this is the local players entity.
    /// As it is convenient for current code (dealing with the particle system...), it is guaranteed that there is exactly one call to this function
    /// with 'FirstPersonView==true', which occurs after the appropriate calls for all other entities. This behaviour may change in the future.
    virtual void PostDraw(float FrameTime, bool FirstPersonView);


    /// Returns the proper type info for this entity.
    virtual const cf::TypeSys::TypeInfoT* GetType() const;
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;     ///< The type info object for (objects/instances of) this class.

    virtual unsigned long GetTypeNr() const;    ///< This method is needed only *TEMPORARILY*, because the code that needs it is still in the engine core.


    // Methods provided to be called from the map/entity Lua scripts.
    static int GetName(lua_State* L);
    static int GetOrigin(lua_State* L);
    static int SetOrigin(lua_State* L);


    protected:

    // Protected constructor such that only concrete entities can call this for creating a 'BaseEntityT', but nobody else.
    // Concrete entities are created in the GameI::CreateBaseEntityFromMapFile() method for the server-side,
    // and in the GameI::CreateBaseEntityFromTypeNr() method for the client-side.
    BaseEntityT(const EntityCreateParamsT& Params, const EntityStateT& State_);


    private:

    BaseEntityT(const BaseEntityT&);        ///< Use of the Copy    Constructor is not allowed.
    void operator = (const BaseEntityT&);   ///< Use of the Assignment Operator is not allowed.

    unsigned long m_OldEvents;
};

#endif