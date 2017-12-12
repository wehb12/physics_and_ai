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