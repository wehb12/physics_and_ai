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

class MazeParamsPacket : public Packet
{
public:
	MazeParamsPacket() : Packet(MAZE_PARAMS, 3) { }
	~MazeParamsPacket() { }
	MazeParamsPacket(enet_uint8 size, enet_uint8 density) :
		Packet(MAZE_PARAMS, 3),
		mazeSize(size),
		mazeDensity(density) { }

	MazeParamsPacket(int size, float density) :
		Packet(MAZE_PARAMS, 3)
	{
		mazeSize = MIN(size, MAX_VAL_8BIT);
		mazeDensity = (float)MAX_VAL_8BIT * density;
	}

	MazeParamsPacket(enet_uint8* data) :
		Packet(MAZE_PARAMS, 3)
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

class MazePathPacket8 : public Packet
{
public:
	MazePathPacket8(enet_uint8* finalPath, enet_uint8 lngth) :
		Packet(MAZE_PATH8, 2 + lngth)
	{
		length = lngth;
		path = new enet_uint8[length];

		for (int i = 0; i < length; ++i)
			path[i] = finalPath[i];
	}

	MazePathPacket8(int* finalPath, int lngth) :
		Packet(MAZE_PATH8, 2 + lngth)
	{
		length = lngth;
		path = new enet_uint8[length];

		for (int i = 0; i < length; ++i)
			path[i] = finalPath[i];
	}

	MazePathPacket8(enet_uint8* data) :
		Packet(MAZE_PATH8, 2 + *(data + 1))
	{
		length = *(data + 1);

		path = new enet_uint8[length];

		for (int i = 0; i < length; ++i)
			path[i] = data[2 + i];
	}

	~MazePathPacket8()
	{
		if (path)
			delete[] path;
		path = NULL;
	}

	virtual enet_uint8* CreateByteStream() override
	{
		enet_uint8* data = new enet_uint8[size];

		data[0] = type;
		data[1] = length;
		
		for (int i = 0; i < length; ++i)
			data[2 + i] = path[i];

		return data;
	}

public:
	enet_uint8 length;
	enet_uint8* path;
};

class MazePathPacket16 : public Packet
{
public:
	MazePathPacket16(enet_uint16* finalPath, enet_uint16 lngth) :
		Packet(MAZE_PATH16, 3 + 2 * lngth)
	{
		length = lngth;
		path = new enet_uint16[length];

		for (int i = 0; i < length; ++i)
			path[i] = finalPath[i];
	}

	MazePathPacket16(int* finalPath, int lngth) :
		Packet(MAZE_PATH16, 3 + 2 * lngth)
	{
		length = lngth;
		path = new enet_uint16[length];

		for (int i = 0; i < length; ++i)
			path[i] = finalPath[i];
	}

	MazePathPacket16(enet_uint8* data) :
		length(*(data + 1) | (*(data + 2) << 8)),
		Packet(MAZE_PATH16, 3 + 2 * length)
	{
		path = new enet_uint16[length];

		for (int i = 0; i < length; ++i)
		{
			path[i] = data[3 + 2 * i];
			path[i] ^= data[4 + 2 * i] << 8;
		}
	}

	~MazePathPacket16()
	{
		if (path)
			delete[] path;
		path = NULL;
	}

	virtual enet_uint8* CreateByteStream() override
	{
		enet_uint8* data = new enet_uint8[size];

		data[0] = type;
		data[1] = 0;
		data[2] = 0;
		data[1] ^= length;
		data[2] ^= length >> 8;

		for (int i = 0; i < length; ++i)
		{
			data[3 + 2 * i] = 0;
			data[4 + 2 * i] = 0;
			data[3 + 2 * i] = path[i];
			data[4 + 2 * i] = path[i] >> 8;
		}

		return data;
	}

public:
	enet_uint16 length;
	enet_uint16* path;
};

enum PreviousInstructionStatus
{
	SUCCESS = 100,
	FAILURE
};

class InstructionCompletePacket : public Packet
{
public:
	InstructionCompletePacket(enet_uint8 status) :
		Packet(INSTR_COMPLETE, 2),
		status(status)
	{ }

	InstructionCompletePacket(bool status) :
		Packet(INSTR_COMPLETE, 2),
		status(status ? SUCCESS : FAILURE)
	{ }

	InstructionCompletePacket(enet_uint8* data) :
		Packet(INSTR_COMPLETE, 2),
		status(*(data + 1))
	{ }

	virtual enet_uint8* CreateByteStream()
	{
		enet_uint8* data = new enet_uint8[size];
		data[0] = type;
		data[1] = status;
		return data;
	}

public:
	enet_uint8 status;
};