#include "Lobbies.h"
#include <cstring>
#include <netinet/in.h>

LobbyChatRoom::LobbyChatRoom(const char *rName, uint16_t rID, uint16_t rType)
{
    //Don't judge me. I get so little sleep...
    strncpy(this->roomName,rName,(ROOM_NAME_MAX_LEN - 1));
    roomName[ROOM_NAME_MAX_LEN] = 0x0;

    this->roomID = rID;
    this->roomType = rType;


}

LobbyChatRoom::~LobbyChatRoom()
{
    //Do some cleanup...
    //Clear userlist...
    uint32_t numUsers = this->GetNumUsers();
    for(std::map<uint16_t,RoomUser_s*>::iterator it = userList.begin(); it != userList.end(); it++)
    {
        RoomUser_s * tmpUser = it->second;
        userList.erase(it->first);
        delete tmpUser;
    }
    //notify admin
    printf("CLEANED %d users from Room: '%s'\n",numUsers,this->GetRoomName());
}

uint16_t LobbyChatRoom::FindNextUserID()
{
    uint16_t foundID = 0;
    for(uint16_t i = 1; i < ROOM_MAX_CLIENTS; i++)
    {
        if(userList.count(i) == 0)
        {
            foundID = i;
            break;
        }

    }
    return foundID;
}

void LobbyChatRoom::setOwner()
{
    //First, check that the room can even be owned...
    if(this->canBeOwned)
    {

        //first, check to make sure we have any more users...
        if(this->GetNumUsers() > 0)
        {

            //Fragment will suffer from year 2038 problem :-/
            uint32_t oldestTime = (uint32_t)time(0);
            for(std::map<uint16_t,RoomUser_s*>::iterator  it = userList.begin(); it != userList.end(); it++)
            {
                RoomUser_s * tmpUser = it->second;
                if(tmpUser->entryTime < oldestTime)
                {
                    this->roomOwnerId = it->first;

                    //Now send packet to client granting room ownership...
                    uint8_t uRes[] = {0x00,0x00};
                    Client * tmpClient = tmpUser->lClient;
                    tmpClient->SendChatPacket(uRes, sizeof(uRes),OPCODE_DATA_LOBBYROOM_GRANT_OWNERSHIP);


                }
            }
        }
        else
        {
            roomOwnerId = 0;
        }
    }
}

uint32_t LobbyChatRoom::GetNumUsers()
{
    return (uint32_t)this->userList.size();
}

char *LobbyChatRoom::GetRoomName()
{
    return this->roomName;
}

void LobbyChatRoom::DispatchPublicBroadcast(uint16_t srcClientID, uint8_t *bData, uint16_t bDataSize)
{
    //Public broadcasts are easy, just set first two bytes
    uint16_t * tmpSrcClient = (uint16_t*)bData;
    uint16_t * tmpUDataLen = &tmpSrcClient[1];
    uint8_t * tmpMsgType = (uint8_t*)&tmpUDataLen[1];

    for(std::map<uint16_t,RoomUser_s*>::iterator it = userList.begin(); it != userList.end(); it++)
    {
        RoomUser_s * tmpUser = it->second;
        Client * tmpClient = tmpUser->lClient;
        //Clients consider themselves user 0xffff(apparently), so if this is the src client,
        //we have to set the srcClientID accordingly...
        if(it->first == srcClientID)
        {
            if(*tmpMsgType < 4)
            {
                *tmpSrcClient = 0xffff;
                tmpClient->SendChatPacket(bData,bDataSize,OPCODE_DATA_LOBBYROOM_PUBLIC_BROADCAST);
            }
        }
        else
        {
            *tmpSrcClient = htons(srcClientID);
            tmpClient->SendChatPacket(bData,bDataSize, OPCODE_DATA_LOBBYROOM_PUBLIC_BROADCAST);
        }


    }

}

void LobbyChatRoom::DispatchPrivateBroadcast(uint16_t srcClientID, uint16_t dstClientID, uint8_t *bData, uint16_t bDataSize)
{
    //Private broadcasts are relatively easy.
    uint16_t * tmpSrcClient = (uint16_t *)bData;
    uint16_t * tmpDestClientID = &tmpSrcClient[1];

    if(userList.count(dstClientID) > 0)
    {
        *tmpDestClientID = htons(srcClientID);
        RoomUser_s * tmpDestUser = userList[dstClientID];
        Client * tmpDestClient = tmpDestUser->lClient;
        tmpDestClient->SendChatPacket(bData,bDataSize,OPCODE_DATA_LOBBYROOM_PRIVATE_BROADCAST);

        tmpDestUser = userList[srcClientID];
        tmpDestClient = tmpDestUser->lClient;
        *tmpDestClientID = htons(0xffff);
        tmpDestClient->SendChatPacket(bData,bDataSize,OPCODE_DATA_LOBBYROOM_PRIVATE_BROADCAST);

    }



}

void LobbyChatRoom::DispatchStatusBroadcast(uint16_t srcClientID, uint8_t *bData, uint16_t bDataSize)
{
    //status packets are a pain in the ass.
    uint16_t uResLen = bDataSize + sizeof(uint16_t) + sizeof(uint16_t);
    uint8_t * uRes = new uint8_t[uResLen];

    //create fields
    uint16_t * tmpSrcClient = (uint16_t *)uRes;
    uint16_t * tmpPDataSize = &tmpSrcClient[1];
    uint8_t * tmpPData = (uint8_t *)&tmpPDataSize[1];

    //fill out packet fields
    *tmpSrcClient = htons(srcClientID);
    *tmpPDataSize = htons(bDataSize);
  //  printf("\tmemcpy...\n");
    memcpy(tmpPData,bData,bDataSize);

    //loop through users to forward the packet
    for(std::map<uint16_t,RoomUser_s*>::iterator it = userList.begin(); it != userList.end(); it++)
    {
        if(it->first != srcClientID)
        {
            RoomUser_s * tmpUser = it->second;
            Client *tmpClient = tmpUser->lClient;
            tmpClient->SendChatPacket(uRes,uResLen,OPCODE_DATA_LOBBYROOM_UPDATE_STATUS);
        }
    }



    //This shouldn't fail, but if it does, we might oughta notify the admin somehow...
    //TODO:Notify admin if the following check fails...
    //Set the client's last status info. We're gonna go ahead and use the ready to send one
    if(userList.count(srcClientID) == 1)
    {
        //Truncate data if it's longer than we allow...
        if(uResLen > 256) uResLen = 256;
        RoomUser_s * tmpUser = userList[srcClientID];
        memcpy(&tmpUser->lastStatus,uRes,uResLen);
        tmpUser->lastStatusSize = uResLen;
    }
    //cleanup
    delete[] uRes;
}

void LobbyChatRoom::NotifyGuildDisbandment()
{
    //Right now, we don't really have any need to send this message, except in a guild room...
    if(this->roomType == ROOM_TYPE_GUILD)
    {
        for(std::map<uint16_t, RoomUser_s*>::iterator it = userList.begin(); it != userList.end(); it++)
        {
            RoomUser_s * tmpUser = it->second;
            Client *tmpClient = tmpUser->lClient;
            uint8_t uRes[] = {0x00,0x00};
            tmpClient->SendChatPacket(uRes,sizeof(uRes),OPCODE_DATA_LOBBYROOM_NOTIFY_GUILD_DISBANDED);

        }


    }
}

uint16_t LobbyChatRoom::AddUser(Client *incomingClient)
{
    RoomUser_s * incomingUser = new RoomUser_s;
    incomingUser->lClient = incomingClient;

    //the client should have checked for a valid roomUserID before even
    //attempting to add itself to the user list...
    uint16_t incomingClientUserID = incomingClient->GetCurrentRoomUserID();
    //store user entry time
    incomingUser->entryTime = time(0);
    memset(incomingUser->lastStatus,0,sizeof(incomingUser->lastStatus));
    incomingUser->lastStatusSize = 0;

    //Make sure there's not already not a client with that ID...
    if(userList.count(incomingClientUserID) != 0)
    {
        //notify administrator that Dyson fails at life...
        printf("ERROR ADDING USER_%d TO ROOM: '%s', ID alread in use?\n",incomingClientUserID,this->roomName);
        delete incomingUser;
        return 0;
    }
    else
    {
        userList[incomingClientUserID] = incomingUser;
        //notify admin
        printf("Added User('%s')(USER: %d) to room '%s'(%d_%04X)\n",incomingClient->GetCharacterName(),incomingClientUserID,this->roomName,this->roomID,this->roomType);

        //forward all status packets for users already in the room...
        //forwardAllStatusPackets(incomingClient);

        return incomingClientUserID;
    }
    return 0;
}

void LobbyChatRoom::RemoveUser(Client *departingClient)
{
    uint16_t departingClientID = departingClient->GetCurrentRoomUserID();
    //double check to make sure that the user is even in the room...
    if(userList.count(departingClientID) != 1)
    {
        //Notify admin that something bad happened...
        printf("ERROR REMOVING USER('%s') from ROOM '%s'(%d,%04X), USERID %d does not exist in Room`s user list!\n",departingClient->GetCharacterName(),this->GetRoomName(),this->roomID,this->roomType,departingClientID);
    }
    else
    {
        RoomUser_s * departingUser = userList[departingClientID];
        userList.erase(departingClientID);

        //iterate users and notify them of user's departure...
        for(std::map<uint16_t, RoomUser_s*>::iterator it = userList.begin(); it != userList.end(); it++)
        {
            //ignore the departing client
            if(it->first != departingClient->GetCurrentRoomUserID())
            {
                RoomUser_s * tmpUser = it->second;
                Client * tmpClient = tmpUser->lClient;
                uint16_t uRes = htons(departingClientID);
                tmpClient->SendChatPacket((uint8_t*)&uRes,sizeof(uint16_t),OPCODE_DATA_LOBBYROOM_NOTIFY_USER_DEPARTURE);
            }
        }

        delete departingUser;
        printf("Removed User('%s',%d) from ROOM ('%s'(%d,%04X))\n",departingClient->GetCharacterName(),departingClientID,this->GetRoomName(),this->roomID,this->roomType);
    }
    //now find the next room owner...
    this->setOwner();
}

uint16_t LobbyChatRoom::GetRoomID()
{
    return this->roomID;
}

void LobbyChatRoom::forwardAllStatusPackets(Client *destClient)
{
    for(std::map<uint16_t, RoomUser_s*>::iterator it = userList.begin(); it != userList.end(); it++)
    {
        if(it->first != destClient->GetCurrentRoomUserID())
        {
            RoomUser_s * tmpUser = it->second;
            destClient->SendChatPacket(tmpUser->lastStatus,tmpUser->lastStatusSize, OPCODE_DATA_LOBBYROOM_UPDATE_STATUS);
        }
    }
}
