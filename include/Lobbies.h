#ifndef _LOBBIES_H_
#define _LOBBIES_H_

#include <stdint.h>
#include "client.h"
#include <time.h>
#include <map>
#include "opcode.h"

//I dunno what the best way around circular inclusion is...
class Client;

//TODO: Can the game even handle this many people in a room?
#define ROOM_MAX_CLIENTS 0xff
//I think the game truncates the name after 30 something characters anyways...
#define ROOM_NAME_MAX_LEN 32

#define ROOM_TYPE_NONE          0x0000
#define ROOM_TYPE_LOBBY         0x7403
#define ROOM_TYPE_PUBLIC_CHAT   0x7409 //Chatroom with no password
#define ROOM_TYPE_PRIATE_CHAT   0x740c //Chatroom with password
#define ROOM_TYPE_GUILD         0x7418

/* LobbyRoom Packets
 *
 *--------------Player Status Related--------------
 *  0x700a - NotifyUserLeftRoom //Notifies a user that another user has left room.
 *      uint16_t userID; //Room Specific userID of user that is leaving.
 *
 *  0x7009 - UserStatus //User status data
 *  //When forwarded to other clients, it's prepended with the source client's room specific userID(uint16_t)
 *  All data in this packet is LITTLE_ENDIAN, as it was more than likely not meant to be parsed by the server.
 *  I still need to investigate the specifics, but it's basically the character's info like GP, Class, modelID, guildID/membership type, etc.
 *
 * --------------Message Broadcasts--------------
 *
 * 0x7862 - RoomPublicBroadcast // gets forwarded to all clients in room.
 *      uint16_t roomID; //When sent from client,
 *          //OR room specific userID of the player that sent the broadcast.
 *      uint16_t paramLength; //Length of paramData
 *      uint8_t broadcastType; //first byte of paramData appears to be broadcast Type.
 *          //Types:
 *          //0 - Chat message
 *          //1 - unknown (black text)
 *          //2 - unknown (gold text)
 *          //3 - unknown
 *          //4 - User Name (broadcasted upon entry to all clients who were previously in room)
 *          //5 - Greeting (broadcasted upon entry to all clients who were previously in room)
 *      uint8_t unkParamData[paramLength - 1]; //unknown/unused param data
 *      uint16_t textLength; //length of broadcast's text.
 *      char text[textLength]; //broadcast's text.
 *
 *
 *  0x788c - RoomPrivateBroadcast //gets forwarded to only one user.
 *      uint16_t srcUserId; //Room specific userID of the user who sent the broadcast
 *      uint16_t destUserId; //Room specific userID of user to forward the message to
 *      uint16_t paramLength; //Length of paramData
 *      uint8_t broadcastType; //first byte of paramData appears to be broadcast Type.
 *          //Types are the same as RoomPublicBroadcast
 *      uint8_t unkParamData[paramLength - 1]; //unknown/unused param data
 *      uint16_t textLength; //length of broadcast's text
 *      char text[textLength]; //broadcast's text.
 *
 * --------------Pre-Defined Messages--------------
 *
 *  0x781f - SetRoomOwnership //Tells the client that they've become owner of the room.
 *      //Only send this to the client who becomes room owner, apparently he will notify other
 *      //clients that he's the room owner now.
 *
 *  0x78aa - GuildDisbandedMessage //Tells a user that the guild has been disbanded.
 *
 *  0x7405 - RoomFullMessage //Tells a user that the room is full, kicks them back to list.
 *
 *  0x7827 - Unknown //Probably kicked.
 *      uint8_t unknown //probably one of the 3 durations (3 Min, 30 Min, 3 Hours.
 *
 *--------------Unknown Packets, Probably Guild Invite Related--------------
 *  0x760a
 *  0x7620
 *  0x761f
 *  0x7606
 *
 */




struct RoomUser_s
{
    //client object for the Room user.
    Client * lClient;

    //used for room ownership.
    time_t entryTime;

    //needs to be sent to a new user when they enter a room, so we have to store the last one the client sent us

    uint8_t lastStatus[256];
    uint16_t lastStatusSize;

};

//TODO: Room Specific Client Bans(temp and perma)


class LobbyChatRoom
{
	private:
        //Room owner's room specific userID.
        uint16_t roomOwnerId;
        //map containing roomUsers
        std::map <uint16_t, RoomUser_s*> userList;

        //Room Type
        uint16_t roomType;

        //Room ID
        uint16_t roomID;


        //Should probably disable ownership of lobbies, lest the trolls attack.
        bool canBeOwned;

        //Room name
        char roomName[ROOM_NAME_MAX_LEN];




        /**
         * @brief set the next room owner.
         *
         * Find the next room owner, set roomOwnerId, and notify the user they're the new room owner.
         */
        void setOwner();

	public:
        LobbyChatRoom(const char *rName, uint16_t rID, uint16_t rType);
        ~LobbyChatRoom();
		
        /**
         * @brief Return number of users currently within the room.
         * @return
         */
        uint32_t GetNumUsers();


        /**
         * @brief Returns the Room Name
         * @return Room Name,
         */
        char * GetRoomName();

        //TODO: Is this a good way to do this?
        /**
         * @brief Find the next available (lowest) userID.
         * @return returns the next availble userID (or 0 if operation failed)
         *
         * Will search through the userList and find the next lowest available room specific userID
         * Returns the next lowest availble userID, or if none is availble (room is full),
         * it will return 0;
         */
        uint16_t FindNextUserID();


        /**
         * @brief Dispatch Public Broadcast Packet (0x7862)
         * @param srcClientID room specific client number of the source client.
         * @param bData broadcast packet raw data.
         * @param bDataSize broadcast packet raw data size
         *
         * Forwards the public broadcast packet to all clients within the room.
         */
        void DispatchPublicBroadcast(uint16_t srcClientID, uint8_t *bData, uint16_t bDataSize);

        /**
         * @brief Dispatch Private Broadcast Packet (0x788c)
         * @param srcClientID room specific client number of source client.
         * @param dstClientID room specific client number of the destinaton client.
         * @param bData broadcast packet raw data (Minus source and destination client IDs)
         * @param bDataSize broadcast packet raw data size
         *
         * Forwards the private broadcast packket to the specified client within the room.
         */
        void DispatchPrivateBroadcast(uint16_t srcClientID, uint16_t dstClientID, uint8_t *bData, uint16_t bDataSize);

        /**
         * @brief Dispatch Client Status Packet (0x7009)
         * @param srcClientID lobbby specific client number of the source client.
         * @param bData broadcast raw packet data.
         * @param bDataSize broadcast packet raw data size)
         */
        void DispatchStatusBroadcast(uint16_t srcClientID, uint8_t * bData, uint16_t bDataSize);
		
	
	
        /**
         * @brief Notify a guild room of the guild's disbandment.
         *
         * Only used in Guild Rooms.
         * Notifies all users of the guild's disbandment.
         */
        void NotifyGuildDisbandment();
	
	
        /**
         * @brief Adds a user to the room.
         * @param client
         * @return room specific userID of the client.
         *
         * Adds the user to the room.
         */
        uint16_t AddUser(Client * incomingClient);
	

        /**
         * @brief Removes a user from the room,
         * @param departingClient
         *
         * Removes the client from the room, notifying all current users
         * in the room of the client's departure.
         */
        void RemoveUser(Client * departingClient);

        /**
         * @brief Gets the room's ID
         * @return room ID
         */
        uint16_t GetRoomID();

        /**
         * @brief Forwards all current user status packets to the destination client
         * @param destClient
         *
         * Does not send a client it's own status packet.
         */
        void forwardAllStatusPackets(Client * destClient);

        //TODO:
        //User Kickicking.
        //Guild Invites.


};


#endif
