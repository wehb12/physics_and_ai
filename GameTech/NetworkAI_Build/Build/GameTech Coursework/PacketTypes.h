#pragma once

#include "../ExternalLibs/ENET/include/enet/types.h"

enum packetType
{
	MAZE_REQUEST = 12,		// first byte is mazeSize, next byte is mazeDensity
	MAZE_DATA
};

struct Packet
{
	unsigned char* data;
};