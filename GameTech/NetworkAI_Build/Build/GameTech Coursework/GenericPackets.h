#pragma once

#include "Packet.h"

using namespace std;

// suitable for small (< 253 char) messages
class MessagePacket : public Packet
{
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

public:
	string message;
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

class ConfirmConnectionPacket : public Packet
{
public:
	ConfirmConnectionPacket(enet_uint8 iD) :
		Packet(CONFIRM_CONNECTION, 2),
		clientID(iD)
	{ }

	ConfirmConnectionPacket(int iD) :
		Packet(CONFIRM_CONNECTION, 2),
		clientID(iD)
	{ }

	ConfirmConnectionPacket(enet_uint8* data) :
		Packet(CONFIRM_CONNECTION, 2),
		clientID(*(data + 1))
	{ }

	virtual enet_uint8* CreateByteStream() override
	{
		enet_uint8* data = new enet_uint8[size];
		
		data[0] = type;
		data[1] = clientID;

		return data;
	}

public:
	enet_uint8 clientID;
};