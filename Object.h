#ifndef OBJECT_H
#define OBJECT_H

#define TYPECLASS
#include "GameFactory.h"

#include "vaultmp.h"
#include "Data.h"
#include "API.h"
#include "Reference.h"
#include "Value.h"

#ifdef VAULTMP_DEBUG
#include "Debug.h"
#endif

using namespace Data;
using namespace Values;
using namespace std;

class Object : public Reference
{
friend class GameFactory;

private:
#ifdef VAULTMP_DEBUG
    static Debug* debug;
#endif

    Value<string> object_Name;
    map<unsigned char, Value<double> > object_Pos;
    map<unsigned char, Value<double> > object_Angle;
    Value<unsigned int> cell_Game;
    Value<unsigned int> cell_Network;
    Value<bool> state_Enabled;

    Object(const Object&);
    Object& operator=(const Object&);

protected:
    Object(unsigned int refID, unsigned int baseID);
    virtual ~Object();

public:
#ifdef VAULTMP_DEBUG
    static void SetDebugHandler(Debug* debug);
#endif

    static const Parameter Param_Axis;

    string GetName() const;
    double GetPos(unsigned char axis) const;
    double GetAngle(unsigned char axis) const;
    bool GetEnabled() const;
    unsigned int GetGameCell() const;
    unsigned int GetNetworkCell() const;

    Lockable* SetName(string name);
    Lockable* SetPos(unsigned char axis, double pos);
    Lockable* SetAngle(unsigned char axis, double angle);
    Lockable* SetEnabled(bool state);
    Lockable* SetGameCell(unsigned int cell);
    Lockable* SetNetworkCell(unsigned int cell);

    bool IsNearPoint(double X, double Y, double Z, double R) const;
    bool IsCoordinateInRange(unsigned char axis, double value, double R) const;

};

#endif