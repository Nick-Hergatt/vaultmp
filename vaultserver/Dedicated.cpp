#include "Dedicated.h"

using namespace RakNet;
using namespace Data;
using namespace std;

RakPeerInterface* Dedicated::peer;
SocketDescriptor* Dedicated::sockdescr;
int Dedicated::port;
int Dedicated::connections;
AMX* Dedicated::amx;
char* Dedicated::announce;
bool Dedicated::query;
Debug* Dedicated::debug;

SystemAddress Dedicated::master;
TimeMS Dedicated::announcetime;

ServerEntry* Dedicated::self;

bool Dedicated::thread;

void Dedicated::TerminateThread()
{
    thread = false;
}

void Dedicated::SetServerName(string name)
{
    self->SetServerName(name);
}

void Dedicated::SetServerMap(string map)
{
    self->SetServerMap(map);
}

void Dedicated::SetServerRule(string rule, string value)
{
    self->SetServerRule(rule, value);
}

bool Dedicated::IsNewVegas()
{
    return self->IsNewVegas();
}

void Dedicated::Announce(bool announce)
{
    if (peer->GetConnectionState(master) == IS_CONNECTED)
    {
        BitStream query;

        if (announce)
        {
            query.Write((MessageID) ID_MASTER_ANNOUNCE);
            query.Write((bool) true);

            RakString name(self->GetServerName().c_str()); RakString map(self->GetServerMap().c_str());
            int players = self->GetServerPlayers().first; int playersMax = self->GetServerPlayers().second;
            bool NewVegas = self->IsNewVegas();
            std::map<string, string> rules = self->GetServerRules();

            query.Write(name);
            query.Write(map);
            query.Write(players);
            query.Write(playersMax);
            query.Write(NewVegas);
            query.Write(rules.size());

            for (std::map<string, string>::const_iterator i = rules.begin(); i != rules.end(); ++i)
            {
                RakString key(i->first.c_str());
                RakString value(i->second.c_str());
                query.Write(key);
                query.Write(value);
            }
        }
        else
        {
            query.Write((MessageID) ID_MASTER_ANNOUNCE);
            query.Write((bool) false);
        }

        peer->Send(&query, HIGH_PRIORITY, RELIABLE_ORDERED, CHANNEL_SYSTEM, master, false, 0);
    }
    else
    {
        Utils::timestamp();
        printf("Lost connection to MasterServer (%s)\n", master.ToString());
        peer->Connect(master.ToString(false), master.GetPort(), MASTER_VERSION, sizeof(MASTER_VERSION), 0, 0, 3, 100, 0);
    }

    announcetime = GetTimeMS();
}

DWORD WINAPI Dedicated::DedicatedThread(LPVOID data)
{
      sockdescr = new SocketDescriptor(port, 0);
      peer = RakPeerInterface::GetInstance();
      peer->SetIncomingPassword(DEDICATED_VERSION, sizeof(DEDICATED_VERSION));

      if (announce)
      {
            peer->Startup(connections + 1, sockdescr, 1, THREAD_PRIORITY_NORMAL);
            peer->SetMaximumIncomingConnections(connections);
            master.SetBinaryAddress(strtok(announce, ":"));
            char* cport = strtok(NULL, ":");
            master.SetPort(cport != NULL ? atoi(cport) : RAKNET_MASTER_STANDARD_PORT);
            peer->Connect(master.ToString(false), master.GetPort(), MASTER_VERSION, sizeof(MASTER_VERSION), 0, 0, 3, 500, 0);
            announcetime = GetTimeMS();
      }
      else
      {
            peer->Startup(connections, sockdescr, 1, THREAD_PRIORITY_NORMAL);
            peer->SetMaximumIncomingConnections(connections);
      }

      #ifdef VAULTMP_DEBUG
      debug = new Debug((char*) "vaultserver");

      char text[128];
      ZeroMemory(text, sizeof(text));
      sprintf(text, "Vault-Tec Multiplayer Mod Dedicated server debug log (%s)", DEDICATED_VERSION);
      debug->Print(text, false);

      ZeroMemory(text, sizeof(text));
      sprintf(text, "Local host: %s (game: %s)", peer->GetMyBoundAddress().ToString(), self->IsNewVegas() ? (char*) "Fallout New Vegas" : (char*) "Fallout 3");
      debug->Print(text, false);

      debug->Print((char*) "Visit www.vaultmp.com for help and upload this log if you experience problems with the mod.", false);
      debug->Print((char*) "-----------------------------------------------------------------------------------------------------", false);
      //debug->PrintSystem();
      //Player::SetDebugHandler(debug);
      Inventory::SetDebugHandler(debug);
      #endif

      Packet* packet;

      Client::SetMaximumClients(connections);

      Inventory::Cleanup();
      Inventory::Initialize(self->IsNewVegas());

      while (thread)
      {
            for (packet = peer->Receive(); packet; peer->DeallocatePacket(packet), packet = peer->Receive())
            {
                switch (packet->data[0])
                {
                    case ID_NEW_INCOMING_CONNECTION:
                        Utils::timestamp();
                        printf("New incoming connection from %s\n", packet->systemAddress.ToString());
                        break;
                    case ID_DISCONNECTION_NOTIFICATION:
                    case ID_CONNECTION_LOST:
                    {
                        Client* client = Client::GetClientFromGUID(packet->guid);
                        Player* player = Player::GetPlayerFromGUID(packet->guid);

                        int ret = 1;

                        if (client != NULL && player != NULL && amx != NULL)
                        {
                            void* args[1];

                            int id = client->GetClientID();

                            args[0] = reinterpret_cast<void*>(&id);

                            ret = Script::Call(amx, (char*) "OnPlayerDisconnect", (char*) "i", args);
                        }

                        Utils::timestamp();

                        switch (packet->data[0])
                        {
                            case ID_DISCONNECTION_NOTIFICATION:
                                printf("Client disconnected (%s)\n", packet->systemAddress.ToString());
                                break;
                            case ID_CONNECTION_LOST:
                                printf("Lost connection (%s)\n", packet->systemAddress.ToString());
                                break;
                        }

                        if (player != NULL)
                        {
                            delete player;

                            map<RakNetGUID, string> players = Player::GetPlayerList();
                            map<RakNetGUID, string>::iterator it;

                            BitStream query(packet->data, packet->length, false);
                            query.IgnoreBytes(sizeof(MessageID));
                            query.Reset();

                            query.Write((MessageID) ID_PLAYER_LEFT);
                            query.Write(packet->guid);

                            for (it = players.begin(); it != players.end(); ++it)
                            {
                                RakNetGUID guid = it->first;
                                peer->Send(&query, HIGH_PRIORITY, RELIABLE_ORDERED, CHANNEL_SYSTEM, guid, false, 0);
                            }
                        }

                        if (client != NULL)
                        {
                            delete client;
                            self->SetServerPlayers(pair<int, int>(Client::GetClientCount(), connections));
                        }
                        break;
                    }
                    case ID_CONNECTION_REQUEST_ACCEPTED:
                    {
                        master = packet->systemAddress;
                        Announce(true);
                        Utils::timestamp();
                        printf("Connected to MasterServer (%s)\n", packet->systemAddress.ToString());
                        break;
                    }
                    case ID_INVALID_PASSWORD:
                    {
                        Utils::timestamp();
                        printf("MasterServer version mismatch (%s)\n", packet->systemAddress.ToString());
                        break;
                    }
                    case ID_MASTER_UPDATE:
                    {
                        if (query)
                        {
                            BitStream query(packet->data, packet->length, false);
                            query.IgnoreBytes(sizeof(MessageID));

                            SystemAddress addr;
                            query.Read(addr);
                            query.Reset();

                            query.Write((MessageID) ID_MASTER_UPDATE);
                            query.Write(addr);

                            RakString name(self->GetServerName().c_str()); RakString map(self->GetServerMap().c_str());
                            int players = self->GetServerPlayers().first; int playersMax = self->GetServerPlayers().second;
                            std::map<string, string> rules = self->GetServerRules();

                            query.Write(name);
                            query.Write(map);
                            query.Write(players);
                            query.Write(playersMax);
                            query.Write((int) rules.size());

                            for (std::map<string, string>::const_iterator i = rules.begin(); i != rules.end(); ++i)
                            {
                                RakString key(i->first.c_str());
                                RakString value(i->second.c_str());
                                query.Write(key);
                                query.Write(value);
                            }

                            peer->Send(&query, LOW_PRIORITY, RELIABLE_ORDERED, CHANNEL_SYSTEM, packet->systemAddress, false, 0);

                            Utils::timestamp();
                            printf("Query processed (%s)\n", packet->systemAddress.ToString());
                            break;
                        }
                        else
                        {
                            Utils::timestamp();
                            printf("Query is disabled (%s)\n", packet->systemAddress.ToString());
                            peer->CloseConnection(packet->systemAddress, true, 0, LOW_PRIORITY);
                            break;
                        }
                    }
                    case ID_GAME_INIT:
                    {
                        BitStream query(packet->data, packet->length, false);
                        query.IgnoreBytes(sizeof(MessageID));

                        RakString rname, rpwd;
                        query.Read(rname);
                        query.Read(rpwd);
                        query.Reset();

                        char name[rname.GetLength()];
                        char pwd[rpwd.GetLength()];
                        strcpy(name, rname.C_String());
                        strcpy(pwd, rpwd.C_String());

                        Client* client = new Client(packet->guid, string(name), string(pwd));
                        self->SetServerPlayers(pair<int, int>(Client::GetClientCount(), connections));

                        int ret = 1;

                        if (amx != NULL)
                        {
                            void* args[3];

                            int id = client->GetClientID();

                            args[0] = reinterpret_cast<void*>(pwd);
                            args[1] = reinterpret_cast<void*>(name);
                            args[2] = reinterpret_cast<void*>(&id);

                            ret = Script::Call(amx, (char*) "OnClientAuthenticate", (char*) "ssi", args);
                        }

                        if (ret)
                        {
                            query.Write((MessageID) ID_GAME_INIT);
                            peer->Send(&query, HIGH_PRIORITY, RELIABLE_ORDERED, CHANNEL_SYSTEM, packet->systemAddress, false, 0);
                        }
                        else
                            peer->CloseConnection(packet->systemAddress, true, 0, HIGH_PRIORITY);
                        break;
                    }
                    case ID_GAME_RUN:
                    {
                        BitStream query;

                        char savegame[128];
                        strcpy(savegame, "default");

                        int ret = 1;

                        if (amx != NULL)
                        {
                            void* args[3];

                            int id = Client::GetClientFromGUID(packet->guid)->GetClientID();
                            int len = sizeof(savegame);

                            args[0] = reinterpret_cast<void*>(&len);
                            args[1] = reinterpret_cast<void*>(savegame);
                            args[2] = reinterpret_cast<void*>(&id);

                            ret = Script::Call(amx, (char*) "OnClientRequestGame", (char*) "isi", args, len);
                        }

                        if (ret)
                        {
                            query.Write((MessageID) ID_GAME_RUN);

                            RakString save(savegame);
                            query.Write(save);

                            peer->Send(&query, HIGH_PRIORITY, RELIABLE_ORDERED, CHANNEL_SYSTEM, packet->systemAddress, false, 0);
                        }
                        else
                            peer->CloseConnection(packet->systemAddress, true, 0, HIGH_PRIORITY);
                        break;
                    }
                    case ID_GAME_START:
                    {
                        map<RakNetGUID, string> players = Player::GetPlayerList();
                        map<RakNetGUID, string>::iterator it;

                        Client* client = Client::GetClientFromGUID(packet->guid);
                        string name = client->GetAuthName();
                        RakString pname(name.c_str());

                        BitStream query(packet->data, packet->length, false);
                        query.IgnoreBytes(sizeof(MessageID));
                        query.Reset();

                        query.Write((MessageID) ID_NEW_PLAYER);
                        query.Write(packet->guid);
                        query.Write(pname);

                        for (it = players.begin(); it != players.end(); ++it)
                        {
                            RakNetGUID guid = it->first;
                            peer->Send(&query, HIGH_PRIORITY, RELIABLE_ORDERED, CHANNEL_SYSTEM, guid, false, 0);
                        }

                        query.Reset();

                        for (it = players.begin(); it != players.end(); ++it)
                        {
                            RakNetGUID guid = it->first;
                            Player* player = Player::GetPlayerFromGUID(guid);
                            string name = player->GetPlayerName();
                            RakString pname(name.c_str());

                            query.Write((MessageID) ID_NEW_PLAYER);
                            query.Write(guid);
                            query.Write(pname);
                            peer->Send(&query, HIGH_PRIORITY, RELIABLE_ORDERED, CHANNEL_SYSTEM, packet->guid, false, 0);
                            query.Reset();

                            Inventory* handle = player->GetPlayerInventory();

                            if (handle != NULL && !handle->IsEmpty())
                            {
                                list<Item*> items = handle->GetItemList();
                                list<Item*>::iterator it2;

                                for (it2 = items.begin(); it2 != items.end(); ++it2)
                                {
                                    pPlayerItemUpdate item = player->GetPlayerItemUpdateStruct(*it2);
                                    peer->Send((char*) &item, sizeof(pPlayerItemUpdate), HIGH_PRIORITY, RELIABLE_ORDERED, CHANNEL_PLAYER_ITEM_UPDATE, packet->guid, false, 0);
                                }
                            }
                        }

                        Player* player = new Player(packet->guid);
                        player->SetPlayerName(name);

                        int ret = 1;

                        if (amx != NULL)
                        {
                            void* args[1];

                            int id = Client::GetClientFromGUID(packet->guid)->GetClientID();

                            args[0] = reinterpret_cast<void*>(&id);

                            ret = Script::Call(amx, (char*) "OnPlayerJoin", (char*) "i", args);
                        }
                        break;
                    }
                    case ID_PLAYER_UPDATE:
                    {
                        pPlayerUpdate* update = (pPlayerUpdate*) packet->data;

                        if (packet->length != sizeof(pPlayerUpdate))
                            break;

                        Player* player = Player::GetPlayerFromGUID(packet->guid);

                        player->SetPlayerPos(X_AXIS, update->X);
                        player->SetPlayerPos(Y_AXIS, update->Y);
                        player->SetPlayerPos(Z_AXIS, update->Z);
                        player->SetPlayerAngle(update->A);
                        player->SetPlayerAlerted(update->alerted);
                        player->SetPlayerMoving(update->moving);

                        player->UpdatePlayerUpdateStruct(update);

                        map<RakNetGUID, string> players = Player::GetPlayerList();
                        map<RakNetGUID, string>::iterator it;

                        for (it = players.begin(); it != players.end(); ++it)
                        {
                            RakNetGUID guid = it->first;
                            if (guid != packet->guid) peer->Send((char*) update, sizeof(pPlayerUpdate), MEDIUM_PRIORITY, RELIABLE_SEQUENCED, CHANNEL_PLAYER_UPDATE, guid, false, 0);
                        }
                        break;
                    }
                    case ID_PLAYER_STATE_UPDATE:
                    {
                        pPlayerStateUpdate* update = (pPlayerStateUpdate*) packet->data;

                        if (packet->length != sizeof(pPlayerStateUpdate))
                            break;

                        Player* player = Player::GetPlayerFromGUID(packet->guid);

                        if (update->dead == true && player->IsPlayerDead() != true)
                        {
                            int ret = 1;

                            if (amx != NULL)
                            {
                                void* args[1];

                                int id = Client::GetClientFromGUID(packet->guid)->GetClientID();

                                args[0] = reinterpret_cast<void*>(&id);

                                ret = Script::Call(amx, (char*) "OnPlayerDeath", (char*) "i", args);
                            }
                        }

                        player->SetPlayerHealth(update->health);
                        player->SetPlayerBaseHealth(update->baseHealth);
                        player->SetPlayerCondition(COND_PERCEPTION, update->conds[COND_PERCEPTION]);
                        player->SetPlayerCondition(COND_ENDURANCE, update->conds[COND_ENDURANCE]);
                        player->SetPlayerCondition(COND_LEFTATTACK, update->conds[COND_LEFTATTACK]);
                        player->SetPlayerCondition(COND_RIGHTATTACK, update->conds[COND_RIGHTATTACK]);
                        player->SetPlayerCondition(COND_LEFTMOBILITY, update->conds[COND_LEFTMOBILITY]);
                        player->SetPlayerCondition(COND_RIGHTMOBILITY, update->conds[COND_RIGHTMOBILITY]);
                        player->SetPlayerDead(update->dead);

                        player->UpdatePlayerStateUpdateStruct(update);

                        map<RakNetGUID, string> players = Player::GetPlayerList();
                        map<RakNetGUID, string>::iterator it;

                        for (it = players.begin(); it != players.end(); ++it)
                        {
                            RakNetGUID guid = it->first;
                            if (guid != packet->guid) peer->Send((char*) update, sizeof(pPlayerStateUpdate), HIGH_PRIORITY, RELIABLE_SEQUENCED, CHANNEL_PLAYER_STATE_UPDATE, guid, false, 0);
                        }
                        break;
                    }
                    case ID_PLAYER_CELL_UPDATE:
                    {
                        pPlayerCellUpdate* update = (pPlayerCellUpdate*) packet->data;

                        if (packet->length != sizeof(pPlayerCellUpdate))
                            break;

                        Player* player = Player::GetPlayerFromGUID(packet->guid);

                        int ret = 1;

                        if (amx != NULL)
                        {
                            void* args[1];

                            int id = Client::GetClientFromGUID(packet->guid)->GetClientID();
                            int cell = update->cell;

                            args[0] = reinterpret_cast<void*>(&cell);
                            args[1] = reinterpret_cast<void*>(&id);

                            ret = Script::Call(amx, (char*) "OnPlayerCellChange", (char*) "ii", args);
                        }

                        player->SetPlayerGameCell(update->cell);
                        player->SetPlayerNetworkCell(update->cell);

                        player->UpdatePlayerCellUpdateStruct(update);

                        map<RakNetGUID, string> players = Player::GetPlayerList();
                        map<RakNetGUID, string>::iterator it;

                        for (it = players.begin(); it != players.end(); ++it)
                        {
                            RakNetGUID guid = it->first;
                            if (guid != packet->guid) peer->Send((char*) update, sizeof(pPlayerCellUpdate), HIGH_PRIORITY, RELIABLE_SEQUENCED, CHANNEL_PLAYER_CELL_UPDATE, guid, false, 0);
                        }
                        break;
                    }
                    case ID_PLAYER_ITEM_UPDATE:
                    {
                        pPlayerItemUpdate* update = (pPlayerItemUpdate*) packet->data;

                        if (packet->length != sizeof(pPlayerItemUpdate))
                            break;

                        Player* player = Player::GetPlayerFromGUID(packet->guid);
                        Inventory* handle = player->GetPlayerInventory();

                        if (handle == NULL)
                        {
                            handle = new Inventory();
                            player->SetPlayerInventory(handle);
                        }

                        if (update->item.count == 0)
                        {
                            if (!handle->UpdateItem(string(update->baseID), update->item.condition, update->item.worn))
                            {

                            }
                        }
                        else if (update->item.count > 0)
                        {
                            if (!handle->AddItem(string(update->baseID), update->item.count, update->item.type, update->item.condition, update->item.worn))
                            {

                            }
                        }
                        else if (update->item.count < 0)
                        {
                            if (!handle->RemoveItem(string(update->baseID), abs(update->item.count)))
                            {

                            }
                        }

                        map<RakNetGUID, string> players = Player::GetPlayerList();
                        map<RakNetGUID, string>::iterator it;

                        for (it = players.begin(); it != players.end(); ++it)
                        {
                            RakNetGUID guid = it->first;
                            if (guid != packet->guid) peer->Send((char*) update, sizeof(pPlayerItemUpdate), HIGH_PRIORITY, RELIABLE_ORDERED, CHANNEL_PLAYER_ITEM_UPDATE, guid, false, 0);
                        }
                        break;
                    }
                    case ID_GAME_END:
                    {
                        /* Do things - game end for client */

                        peer->CloseConnection(packet->systemAddress, true, 0, HIGH_PRIORITY);
                        break;
                    }
                }
            }

            RakSleep(2);

            if (announce)
            {
                if ((GetTimeMS() - announcetime) > RAKNET_MASTER_RATE)
                    Announce(true);
            }
      }

      peer->Shutdown(300);
      RakPeerInterface::DestroyInstance(peer);

      #ifdef VAULTMP_DEBUG
      debug->Print((char*) "Network thread is going to terminate...", true);
      delete debug;
      #endif

      return ((DWORD) data);
}

HANDLE Dedicated::InitializeServer(int port, int connections, AMX* amx, char* announce, bool query)
{
    HANDLE hDedicatedThread;
    DWORD DedicatedID;

    thread = true;
    Dedicated::port = port ? port : RAKNET_STANDARD_PORT;
    Dedicated::connections = connections ? connections : RAKNET_STANDARD_CONNECTIONS;
    Dedicated::amx = amx;
    Dedicated::announce = announce;
    Dedicated::query = query;

    Dedicated::self->SetServerPlayers(pair<int, int>(0, Dedicated::connections));
    hDedicatedThread = CreateThread(NULL, 0, DedicatedThread, (LPVOID) 0, 0, &DedicatedID);

    return hDedicatedThread;
}

void Dedicated::SetServerEntry(ServerEntry* self)
{
    Dedicated::self = self;
}

/* void Dedicated::SetServerConnections(int connections)
{

} */
