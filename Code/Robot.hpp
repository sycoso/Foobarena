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

#ifndef _ROBOT_HPP_
#define _ROBOT_HPP_

#include "../../BaseEntity.hpp"
#include "Math3D/Vector3.hpp"

#include <vector>
using namespace std;

class EntityCreateParamsT;
class EntRobotPartT;
class EntSmokeT;
class SoundI;

class EntRobotT : public BaseEntityT
{
    public:

    EntRobotT(const EntityCreateParamsT& Params);
    ~EntRobotT();

    void Think(float FrameTime, unsigned long ServerFrameNr);

    void TakeDamage(BaseEntityT* Entity, char Amount, const VectorT& ImpactDir, bool isTorso, EntRobotPartT *part);

    void Serialize(cf::Network::OutStreamT &Stream) const {}
    void Deserialize(cf::Network::InStreamT &Stream, bool IsIniting) {}

    const cf::TypeSys::TypeInfoT* GetType() const;
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


private:
    bool mCreated;
    vector<EntRobotPartT *> mPart;
    vector<Vector3dT> mSlotPos;
    vector<Vector3dT> mSlotRot;
    int mHeadCount, mWeaponCount, mMovementCount;
    int mSpeed, mRange, mDamage;
    double mFirerate, mTimeSinceLastShot;
    int mMovementRadius;
    int mFovy;

    bool mIsShooting;
    bool mDirection;

    BaseEntityT *mTarget;

    EntSmokeT *mSmoke;
    SoundI* mSound;
};

#endif
