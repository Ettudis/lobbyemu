#ifndef _SERVER_H_
#define _SERVER_H_

#include <list>
#include "client.h"
#include "Lobbies.h"

class Server
{
	private:

	// Server Instance
	static Server * instance;

	// Internal Client List
	std::list<Client *> * clients;

	// Internal Area Server List
	std::list<AreaServer *> * areaServers;

    // Internal Lobby Room List
    std::list<LobbyChatRoom *> * lobbyRooms;


	/**
	 * Server Constructor
	 */
	Server();

	/**
	 * Server Destructor
	 */
	~Server();

	public:

	/**
	 * Singleton Instance Getter
	 * @return Singleton Instance
	 */
	static Server * getInstance();

	/**
	 * Free Singleton Memory
	 */
	static void release();

	/**
	 * Get Client List from Server
	 * @return Client List
	 */
	std::list<Client *> * GetClientList();

	/**
	 * Get Area Server List from Server
	 * @return Area Server List
	 */
	std::list<AreaServer *> * GetAreaServerList();

    /**
     * @brief Get Lobby Room List from Server
     * @return  Lobby Room List
     */
    std::list<LobbyChatRoom *> * GetLobbyRoomList();

};

#endif

