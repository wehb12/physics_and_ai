/******************************************************************************
Class: MazeDataPackets
Implements:
Author:
Will Hinds      <w.hinds2@newcastle.ac.uk>
Description:

Multiple packets for sending different data structures over a bytestream link.
Allows for easy use and handling of data at each side of the connection, as
encoding and decoding is handled inside the class constructors/ CreateByteStream
method.

*//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Packet.h"

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
		mazeSize = MIN(size, MAX_VAL_8BIT);
		mazeDensity = (float)MAX_VAL_8BIT * density;
	}

	MazeRequestPacket(enet_uint8* data) :
		Packet(MAZE_DATA8, 3)
	{
		mazeSize = *(data + 1);
		mazeDensity = *(data + 2);
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

// maze data packet with 8 bit wall number
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

		for (int i = 0; i <= (numEdges / 8); ++i)
		{
			int max = 8;
			if (i == numEdges / 8)
				max = numEdges % 8;

			enet_uint8 thisByte = *(data + 2 + i);
			for (int j = 0; j < max; ++j)
				edgesThatAreWalls[i * 8 + j] = (thisByte >> j) & 1;
		}
	}


	virtual enet_uint8* CreateByteStream() override
	{
		enet_uint8* data = new enet_uint8[size + 1];

		data[0] = type;
		data[1] = numEdges;
		for (int i = 0; i <= (numEdges / 8); ++i)
		{
			int max = 8;
			if (i == numEdges / 8)
				max = numEdges % 8;

			enet_uint8 byte = 0;
			for (int j = 0; j < max; ++j)
				byte ^= edgesThatAreWalls[(i * 8) + j] << j;

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

// maze data packet with 16 bit wall number
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
		enet_uint8* data = new enet_uint8[size + 1];

		data[0] = type;
		data[1] = 0;
		data[1] ^= numEdges;
		data[2] = 0;
		data[2] ^= numEdges >> 8;
		for (int i = 0; i <= (numEdges / 8); ++i)
		{
			int max = 8;
			if (i == (numEdges / 8))
				max = numEdges % 8;

			enet_uint8 byte = 0;
			for (int j = 0; j < max; ++j)
				byte ^= edgesThatAreWalls[(i * 8) + j] << j;

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

// maze start and end position data packet for 8 bit node number
class MazePositionsPacket8 : public Packet
{
public:
	MazePositionsPacket8() : Packet(MAZE_POSITIONS8, 3) { }
	~MazePositionsPacket8() { }

	MazePositionsPacket8(enet_uint8 start, enet_uint8 end) :
		Packet(MAZE_POSITIONS8, 3),
		start(start),
		end(end) { }

	MazePositionsPacket8(int strt, int End) :
		Packet(MAZE_POSITIONS8, 3)
	{
		this->start = MIN(strt, MAX_VAL_8BIT);
		this->end = MIN(End, MAX_VAL_8BIT);
	}

	MazePositionsPacket8(enet_uint8* data) :
		Packet(MAZE_POSITIONS8, 3)
	{
		start = *(data + 1);
		end = *(data + 2);
	}

	virtual enet_uint8* CreateByteStream() override
	{
		enet_uint8* data = new enet_uint8[size];

		data[0] = type;
		data[1] = start;
		data[2] = end;

		return data;
	}

public:
	enet_uint8 start;
	enet_uint8 end;
};

// maze start and end position data packet for 16 bit node number
class MazePositionsPacket16 : public Packet
{
public:
	MazePositionsPacket16() : Packet(MAZE_POSITIONS16, 5) { }
	~MazePositionsPacket16() { }

	MazePositionsPacket16(enet_uint16 start, enet_uint16 end) :
		Packet(MAZE_POSITIONS16, 5),
		start(start),
		end(end) { }

	MazePositionsPacket16(int strt, int End) :
		Packet(MAZE_POSITIONS16, 5)
	{
		start = MIN(strt, MAX_VAL_16BIT);
		end = MIN(End, MAX_VAL_16BIT);
	}

	MazePositionsPacket16(enet_uint8* data) :
		Packet(MAZE_POSITIONS16, 3)
	{
		start = *(data + 1) | (*(data + 2) << 8);
		end = *(data + 3) | (*(data + 4) << 8);
	}

	virtual enet_uint8* CreateByteStream() override
	{
		enet_uint8* data = new enet_uint8[size];

		data[0] = type;
		data[1] = 0;
		data[2] = 0;
		data[1] ^= start;
		data[2] ^= start >> 8;
		data[3] = 0;
		data[4] = 0;
		data[3] ^= end;
		data[4] ^= end >> 8;

		return data;
	}

public:
	enet_uint16 start;
	enet_uint16 end;
};