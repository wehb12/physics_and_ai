#pragma once

#include "../ExternalLibs/ENET/include/enet/types.h"

enum packetType
{
	MAZE_REQUEST = 12,
	MAZE_DATA8
};

struct MazeRequestPacket
{
	enet_uint8 type = MAZE_REQUEST;
	enet_uint8 mazeSize;
	enet_uint8 mazeDensity;
};

// maze data struct with 8 bit wall number
struct MazeDataPacket8
{
	enet_uint8 type = MAZE_DATA8;
	enet_uint8 numWalls;
	// contains lists of nodes numWalls in size
	// each index of which provides information about
	// which two nodes a wall separates
	enet_uint8* nodeA;
	enet_uint8* nodeB;
};