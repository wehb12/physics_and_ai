#pragma once

#include "../ExternalLibs/ENET/include/enet/types.h"

#ifndef MAX
#define MIN(a, b) (a < b ? a : b)
#endif

enum packetType
{
	MAZE_REQUEST = 12,
	MAZE_DATA8
};

class Packet
{
public:
	Packet(enet_uint8 type, int size) : 
		type(type), size(size)	{ }
	~Packet()	{ }

	enet_uint8 type;
	const int size;

	virtual enet_uint8* CreateByteStream() = 0;
};

class MazeRequestPacket : public Packet
{
public:
	MazeRequestPacket() : Packet(MAZE_REQUEST, 3) { }
	~MazeRequestPacket() { }
	MazeRequestPacket(enet_uint8 size, enet_uint8 density) :
		Packet(MAZE_REQUEST, 3),
		mazeSize(size),
		mazeDensity(density) { }

	MazeRequestPacket(int size, float density) :
		Packet(MAZE_REQUEST, 3)
	{
		mazeSize = MIN(size, 255);
		mazeDensity = (float)255 * density;
	}

	virtual enet_uint8* CreateByteStream() override
	{
		enet_uint8* data = new enet_uint8[size];

		data[0] = type;
		data[1] = mazeSize;
		data[2] = mazeDensity;

		return data;
	}

public:
	enet_uint8 mazeSize;
	enet_uint8 mazeDensity;
};

// maze data struct with 8 bit wall number
class MazeDataPacket8 : public Packet
{
public:
	~MazeDataPacket8()
	{
		if (numWalls)
		{
			delete[] nodeA;
			delete[] nodeB;
		}
	}
	MazeDataPacket8(enet_uint8 numWalls, enet_uint8* nodeA, enet_uint8* nodeB) :
		Packet(MAZE_DATA8, (2 + 2 * numWalls)),
		numWalls(numWalls),
		nodeA(nodeA),
		nodeB(nodeB)
	{ }

	MazeDataPacket8(int numWalls, enet_uint8* nodeA, enet_uint8* nodeB) :
		Packet(MAZE_DATA8, (2 + 2 * numWalls)),
		numWalls(numWalls),
		nodeA(nodeA),
		nodeB(nodeB)
	{ }

	MazeDataPacket8(int numWalls) :
		Packet(MAZE_DATA8, (2 + 2 * numWalls)),
		numWalls(numWalls)
	{
		if (numWalls)
		{
			nodeA = new enet_uint8[numWalls];
			nodeB = new enet_uint8[numWalls];
		}
	}


	virtual enet_uint8* CreateByteStream() override
	{
		enet_uint8* data = new enet_uint8[size];

		data[0] = type;
		data[1] = numWalls;
		for (int i = 0; i < numWalls; ++i)
			data[2 + i] = nodeA[i];
		for (int i = 0; i < numWalls; ++i)
			data[2 + numWalls + 1] = nodeB[i];

		return data;
	}

public:
	enet_uint8 numWalls;
	// contains lists of nodes numWalls in size
	// each index of which provides information about
	// which two nodes a wall separates
	enet_uint8* nodeA;
	enet_uint8* nodeB;
};