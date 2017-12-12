#pragma once

#include "../ExternalLibs/ENET/include/enet/types.h"

#ifndef MAX
#define MIN(a, b) (a < b ? a : b)
#endif

enum packetType
{
	MAZE_REQUEST = 12,
	MAZE_DATA8,
	MAZE_DATA16
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
		if (edgesThatAreWalls)
			delete[] edgesThatAreWalls;
	}
	MazeDataPacket8(enet_uint8 numEdges, bool* walls) :
		Packet(MAZE_DATA8, (2 + (numEdges / 8) + (numEdges % 8))),
		numEdges(numEdges),
		edgesThatAreWalls(walls)
	{ }

	MazeDataPacket8(int numEdges, bool* walls) :
		Packet(MAZE_DATA8, (2 + (numEdges / 8) + (numEdges % 8))),
		numEdges(numEdges),
		edgesThatAreWalls(walls)
	{ }

	MazeDataPacket8(int numEdges) :
		Packet(MAZE_DATA8, (2 + (numEdges / 8) + (numEdges % 8))),
		numEdges(numEdges)
	{
		if (numEdges)
			edgesThatAreWalls = new bool[numEdges];
	}

	MazeDataPacket8(enet_uint8* data) :
		Packet(MAZE_DATA8, (2 + (*(data + 1) / 8) + (*(data + 1) % 8)))
	{
		numEdges = *(data + 1);
		if (numEdges)
			edgesThatAreWalls = new bool[numEdges];
		
		for (int i = 0; i < numEdges; ++i)
		{
			edgesThatAreWalls[i] = *(data + 8 + i);
		}
	}


	virtual enet_uint8* CreateByteStream() override
	{
		enet_uint8* data = new enet_uint8[size];

		data[0] = type;
		data[1] = numEdges;
		for (int i = 0; i <= (numEdges / 8); ++i)
		{
			int max = 8;
			if (i == numEdges / 8)
				max = numEdges % 8;

			enet_uint8 byte = 0;
			for (int j = 0; j < max; ++j)
				byte ^= edgesThatAreWalls[(i * 8) + j] << (max - j);
			data[2 + i] = byte;
		}

		return data;
	}

public:
	enet_uint8 numEdges;
	// contains lists of nodes numEdges in size
	// each index of which provides information about
	// which two nodes a wall separates
	bool* edgesThatAreWalls;
};

// maze data struct with 8 bit wall number
class MazeDataPacket16 : public Packet
{
public:
	~MazeDataPacket16()
	{
		if (edgesThatAreWalls)
			delete[] edgesThatAreWalls;
	}
	MazeDataPacket16(enet_uint16 numEdges, bool* walls) :
		Packet(MAZE_DATA16, (3 + (numEdges / 8) + (numEdges % 8))),
		numEdges(numEdges),
		edgesThatAreWalls(walls)
	{ }

	MazeDataPacket16(int numEdges, bool* walls) :
		Packet(MAZE_DATA16, (3 + (numEdges / 8) + (numEdges % 8))),
		numEdges(numEdges),
		edgesThatAreWalls(walls)
	{ }

	MazeDataPacket16(int numEdges) :
		Packet(MAZE_DATA16, (3 + (numEdges / 8) + (numEdges % 8))),
		numEdges(numEdges)
	{
		if (numEdges)
			edgesThatAreWalls = new bool[numEdges];
	}

	MazeDataPacket16(enet_uint8* data) :
		Packet(MAZE_DATA16, (3 + (*(data + 1) / 8) + (*(data + 1) % 8)))
	{
		numEdges = *(data + 1) | (*(data + 2) << 8);
		if (numEdges)
			edgesThatAreWalls = new bool[numEdges];

		for (int i = 0; i <= (numEdges / 8); ++i)
		{
			int max = 8;
			if (i == numEdges / 8)
				max = numEdges % 8;

			enet_uint8 thisByte = *(data + 3 + i);
			for (int j = 0; j < max; ++j)
				edgesThatAreWalls[i * 8 + j] = (thisByte >> j) & 1;
		}
	}


	virtual enet_uint8* CreateByteStream() override
	{
		enet_uint8* data = new enet_uint8[size];

		data[0] = type;
		data[1] = 0;
		data[1] ^= numEdges;
		data[2] = 0;
		data[2] ^= numEdges >> 8;
		for (int i = 0; i <= (numEdges / 8); ++i)
		{
			int max = 8;
			if (i == numEdges / 8)
				max = numEdges % 8;

			enet_uint8 byte = 0;
			for (int j = 0; j < max; ++j)
				byte ^= edgesThatAreWalls[(i * 8) + j] << (max - 1 - j);
			data[3 + i] = byte;
		}

		return data;
	}

public:
	enet_uint16 numEdges;
	// contains lists of nodes numEdges in size
	// each index of which provides information about
	// which two nodes a wall separates
	bool* edgesThatAreWalls;
};