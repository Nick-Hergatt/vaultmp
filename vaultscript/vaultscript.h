/*
 *  vaultscript.h
 *  Don't change anything here
 */

#include <string>

#ifndef __WIN32__
  #define VAULTSCRIPT __attribute__ ((__visibility__("default")))
#else
    #define VAULTSCRIPT __declspec(dllexport)
#endif

namespace vaultmp {

typedef unsigned char Index; // 1 byte
typedef unsigned char Reason; // 1 byte
typedef bool State; // 1 byte
typedef unsigned int Player; // 4 byte
typedef unsigned int Cell; // 4 byte
typedef unsigned int Base; // 4 byte
typedef unsigned int Interval; // 4 byte
typedef double Value; // 8 byte
typedef double Timer; // 8 byte

typedef void (*TimerFunc)();

static const Index FALLOUT3             =   0x01;
static const Index NEWVEGAS             =   FALLOUT3 << 1;
static const Index OBLIVION             =   NEWVEGAS << 1;
static const Index FALLOUT_GAMES        =   FALLOUT3 | NEWVEGAS;
static const Index ALL_GAMES            =   FALLOUT_GAMES | OBLIVION;

};

extern "C" {
    VAULTSCRIPT void exec();
    VAULTSCRIPT bool OnClientAuthenticate(std::string, std::string);
    VAULTSCRIPT void OnPlayerDisconnect(vaultmp::Player, vaultmp::Reason);
    VAULTSCRIPT vaultmp::Base OnPlayerRequestGame(vaultmp::Player);
    VAULTSCRIPT void OnPlayerSpawn(vaultmp::Player);
    VAULTSCRIPT void OnPlayerDeath(vaultmp::Player);
    VAULTSCRIPT void OnPlayerCellChange(vaultmp::Player, vaultmp::Cell);
    VAULTSCRIPT void OnPlayerValueChange(vaultmp::Player, vaultmp::Index, vaultmp::Value);
    VAULTSCRIPT void OnPlayerBaseValueChange(vaultmp::Player, vaultmp::Index, vaultmp::Value);
    VAULTSCRIPT void OnPlayerStateChange(vaultmp::Player, vaultmp::Index, vaultmp::State);

    VAULTSCRIPT void (*timestamp)();
    VAULTSCRIPT vaultmp::Timer (*CreateTimer)(vaultmp::TimerFunc, vaultmp::Interval);
    VAULTSCRIPT void (*KillTimer)(vaultmp::Timer);

    VAULTSCRIPT void (*SetServerName)(std::string);
    VAULTSCRIPT void (*SetServerMap)(std::string);
    VAULTSCRIPT void (*SetServerRule)(std::string, std::string);
    VAULTSCRIPT vaultmp::Index (*GetGameCode)();

    VAULTSCRIPT std::string (*ValueToString)(vaultmp::Index);
    VAULTSCRIPT std::string (*AxisToString)(vaultmp::Index);
    VAULTSCRIPT std::string (*AnimToString)(vaultmp::Index);

    VAULTSCRIPT vaultmp::Value (*GetPlayerPos)(vaultmp::Player, vaultmp::Index);
    VAULTSCRIPT void (*GetPlayerPosXYZ)(vaultmp::Player, vaultmp::Value&, vaultmp::Value&, vaultmp::Value&);
    VAULTSCRIPT vaultmp::Value (*GetPlayerAngle)(vaultmp::Player, vaultmp::Index);
    VAULTSCRIPT void (*GetPlayerAngleXYZ)(vaultmp::Player, vaultmp::Value&, vaultmp::Value&, vaultmp::Value&);
    VAULTSCRIPT vaultmp::Value (*GetPlayerValue)(vaultmp::Player, vaultmp::Index);
    VAULTSCRIPT vaultmp::Value (*GetPlayerBaseValue)(vaultmp::Player, vaultmp::Index);
    VAULTSCRIPT vaultmp::Cell (*GetPlayerCell)(vaultmp::Player);
}