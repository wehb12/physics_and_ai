/******************************************************************************
Class: Packet
Implements:
Author:
Will Hinds      <w.hinds2@newcastle.ac.uk>
Description:

An abstract class to use as a base for all packet wrappers. Contains function
to convert packet data into a byte stream that can be sent using the ENET
functionality. Each packet class must implement its own CreateByteStream method.

CreateByteStream converts the packet into a generic HEADER->DATA structure. The
length of the data is contained within the header or is decoded at the server by
knowing the TYPE of the packet, which is ALWAYS included as the first byte of the
HEADER.

*//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "../ExternalLibs/ENET/include/enet/types.h"

#ifndef MIN
#define MIN(a, b) (a < b ? a : b)
#endif

#define MAX_VAL_8BIT 255
#define MAX_VAL_16BIT 65535

enum packetType
{
	MESSAGE = 12,
	MAZE_PARAMS,
	MAZE_DATA8,
	MAZE_DATA16,
	MAZE_POSITIONS8,
	MAZE_POSITIONS16,
	MAZE_PATH8,
	MAZE_PATH16,
	INSTR_COMPLETE
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
