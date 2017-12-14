#pragma once

#include "Packet.h"

using namespace std;

// suitable for small (< 253 char) messages
class MessagePacket : public Packet
{
public:
	string message;

public:
	MessagePacket(string msg) :
		Packet(MESSAGE, 2 + msg.size()),
		message(msg)
	{ }

	MessagePacket(enet_uint8* data) :
		Packet(MESSAGE, *(data + 1))
	{
		message = "";
		for (int i = 0; i < size - 2; ++i)
			message += *(data + 2 + i);
	}

	virtual enet_uint8* CreateByteStream() override
	{
		enet_uint8* data = new enet_uint8[size];
		
		data[0] = type;
		data[1] = (enet_uint8)size;
		
		for (int i = 0; i < message.size(); ++i)
			data[i + 2] = message[i];

		return data;
	}
};

enum PreviousInstructionStatus
{
	SUCCESS = 100,
	FAILURE
};

class InstructionCompletePacket : public Packet
{
public:
	enet_uint8 status;

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
};

// used to tell client they are connected and the server knows abou them
class ConfirmConnectionPacket : public Packet
{
public:
	enet_uint8 clientID;
	// this is the client's avatar colour
	float colour;

public:
	ConfirmConnectionPacket(enet_uint8 iD, float colour) :
		Packet(CONFIRM_CONNECTION, 3 + to_string(colour).size()),
		clientID(iD),
		colour(colour)
	{ }

	ConfirmConnectionPacket(int iD, float colour) :
		Packet(CONFIRM_CONNECTION, 3 + to_string(colour).size()),
		clientID(iD),
		colour(colour)
	{ }

	ConfirmConnectionPacket(enet_uint8* data) :
		Packet(CONFIRM_CONNECTION, *(data + 1)),
		clientID(*(data + 1))
	{
		clientID = *(data + 2);
		string colString = "";
		for (int i = 3; i < size; ++i)
			colString += *(data + i);

		colour = stof(colString);
	}

	virtual enet_uint8* CreateByteStream() override
	{
		enet_uint8* data = new enet_uint8[size];
		
		data[0] = type;
		data[1] = size;
		data[2] = clientID;

		string colString = to_string(colour);

		for (int i = 0; i < colString.size(); ++i)
			*(data + 3 + i) = colString[i];

		return data;
	}
};

class ToggleBooleanPacket : public Packet
{
public:
	bool value;

public:
	ToggleBooleanPacket(enet_uint8 type, bool value) :
		Packet(type, 2),
		value(value)
	{ }

	ToggleBooleanPacket(enet_uint8* data) :
		Packet(*data, 2),
		value(*(data + 1) == SUCCESS)
	{ }

	virtual enet_uint8* CreateByteStream()
	{
		enet_uint8* data = new enet_uint8[size];

		data[0] = type;
		data[1] = value ? SUCCESS : FAILURE;

		return data;
	}
};