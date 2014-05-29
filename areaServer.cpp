#include "areaServer.h"
#include <string.h>
#include <netinet/in.h>
#include <stdio.h>

/*

		char serverName[21];
		char shortName[9];
		uint8_t id[8];
		uint16_t port;
		uint32_t localIP;
		uint32_t extIP;
		
		uint16_t numUsers;
		uint8_t status;
*/



AreaServer::AreaServer()
{
	// init everything to 0?
	this->serverIPExt = 0;
	this->serverIPLocal = 0;
	this->serverPort = 0;
	this->serverUsers = 0;
	this->serverType = 0;	
	this->serverLevel = 0;
	this->serverStatus = 0;
}


																																						
AreaServer::AreaServer(int socket, uint32_t eIp, uint32_t lIp, uint32_t port, char* name, uint8_t * id, uint16_t level,uint8_t status,uint16_t sType)
{
	this->socket = socket;
	this->serverIPExt = eIp;
	this->serverIPLocal = lIp;
	this->serverPort = port;
	strncpy((char*)this->serverName,name,20);
	this->serverName[20] = 0x00;
	memcpy(this->serverId,id,8);
	this->serverStatus = 0;
	this->serverUsers = 0;
	this->serverLevel = level;	
	this->serverType = sType;
	printf("REGISTERING AREA SERVER: %s, ID:%08X, STATUS:%02X, LEVEL:%02X, USERS:%02X, TYPE: %02X\n",this->serverName,this->serverId,this->serverStatus,this->serverLevel,this->serverUsers, this->serverType);
	
}	

AreaServer::~AreaServer()
{
	//nothing to really do yet...
};

void AreaServer::setStatus(uint8_t status)
{
	this->serverStatus = status;
}

void AreaServer::setType(uint16_t type)
{
	this->serverType = type;
}

void AreaServer::setUsers(uint16_t users)
{
	this->serverUsers = users;
}

void AreaServer::setLevel(uint16_t level)
{
	this->serverLevel = level;
	
}

bool AreaServer::GetServerLine(uint8_t * output,uint16_t outputLen, uint32_t clientIP)
//Convenience. Automatically determines if Client and Area Server are on the same ExtIP...
{

//	printf("AWTF1\n");		
	//Cast Fields
	uint8_t * unk1 = output;
	uint32_t * sAddr = (uint32_t *)&unk1[1];
	uint16_t * sPort = (uint16_t *)&sAddr[1];
	char * sName = (char *)&sPort[1];
	
//	printf("AWTF2\n");
	uint32_t sNLen = strlen(this->serverName) + 1;
	uint16_t * sLevel = (uint16_t *)&sName[sNLen];
	uint16_t * sType = &sLevel[1];
	uint16_t * sUsers = &sType[1];
	uint8_t * sStatus = (uint8_t *)&sUsers[1];
	uint8_t * sID = &sStatus[1];
	

	*unk1 = 0x00;
	
	if(clientIP == this->serverIPExt)
	{
		*sAddr = this->serverIPLocal;
	}
	else
	{
		*sAddr = this->serverIPExt;
	}
	
	*sPort = this->serverPort;
	strcpy(sName,this->serverName);
	*sLevel = htons(this->serverLevel);
	*sType = htons(this->serverType);
	*sUsers = htons(this->serverUsers);
	*sStatus = this->serverStatus;
	memcpy(sID,this->serverId,8);
	
	return true;	



/*	
struct lobbyEntry
				{
					uint8_t unk1;
					uint32_t IP;
					uint16_t Port;
					uint8_t name; //nullTerminated
					uint16_t Level;
					uint16_t st1;
					uint16_t pcs;
					uint8_t st2;
					uint8_t ID;
				}	
	*/
}


//		AreaServer();
//		AreaServer(uint32_t extIp, uint32_t localIp,uint32_t port, char * name, uint8_t id);
	
			
		//Destructor
//		~AreaServer();

		//Setters.. Will code Getters later...	
//		void SetExtIP(uint32_t addr);
	//	void SetLocalIP(uint32_t addr);
//		void SetPort(uint16_t port);
		
//		void SetName(char * name);
		
//		void setStatus(uint8_t status);
//		void setNumUsers(uint16_t users);
		
//		bool GetServerLine(uint8_t * output,uint16_t outputLen);