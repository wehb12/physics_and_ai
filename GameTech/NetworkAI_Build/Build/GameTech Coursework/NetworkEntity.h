#pragma once

#include <ncltech\NetworkBase.h>
#include <string>
#include "PacketTypes.h"
#include "MazeGenerator.h"
#include "MazeRenderer.h"

enum NetworkEntityType
{
	CLIENT,
	SERVER
};

class NetworkEntity
{
public:
	NetworkEntity(enet_uint8 type) : type(type) { }
	~NetworkEntity() { }

	string HandlePacket(const ENetPacket* packet);


//////// GETTERS ////////
	inline enet_uint8 GetType() { return type; }

private:
	enet_uint8 type;
};