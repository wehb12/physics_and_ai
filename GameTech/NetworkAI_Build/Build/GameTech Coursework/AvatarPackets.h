#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include "Packet.h"
#include "../nclgl/Vector2.h"
#include <string>

class AvatarPositionPacket : public Packet
{
public:
	AvatarPositionPacket(float posX, float posY, int index, int iD) :
		Packet(AVATAR_POSITION, 6 + to_string(posX).size() + to_string(posY).size()),
		posX(posX),
		posY(posY),
		currentIndex(index),
		iD(iD)
	{ }

	AvatarPositionPacket(Vector2 pos, int index, int iD) :
		Packet(AVATAR_POSITION, 6 + to_string(pos.x).size() + to_string(pos.y).size()),
		posX(pos.x),
		posY(pos.y),
		currentIndex(index),
		iD(iD)
	{ }

	AvatarPositionPacket(enet_uint8* data) :
		Packet(AVATAR_POSITION, *(data + 1)),
		currentIndex(*(data + 2) | (*(data + 3) << 8)),
		iD(*(data + 4))
	{
		string xString = "";
		string yString = "";

		bool x = true;
		for (int i = 5; i < size - 1; ++i)
		{
			if (data[i] == ' ')
			{
				x = false;
				continue;
			}
			else
			{
				if (x)
					xString += data[i];
				else
					yString += data[i];
			}
		}
		posX = stof(xString);
		posY = stof(yString);
	}

	virtual enet_uint8* CreateByteStream() override
	{
		enet_uint8* data = new enet_uint8[size];

		data[0] = type;
		data[1] = size;
		data[2] = 0;
		data[2] ^= currentIndex;
		data[3] = 0;
		data[3] ^= currentIndex >> 8;
		data[4] = iD;

		string xString = to_string(posX);
		string yString = to_string(posY);

		for (int i = 0; i < xString.size(); ++i)
			*(data + 5 + i) = xString[i];

		*(data + 5 + xString.size()) = ' ';

		for (int i = 0; i < yString.size(); ++i)
			*(data + 6 + xString.size() + i) = yString[i];

		data[size - 1] = '\0';

		return data;
	}

public:
	float posX;
	float posY;
	enet_uint16 currentIndex;
	enet_uint8 iD;	//iD of client who this avatar belongs to
};

class NewAvatarPacket : public Packet
{
public:
	NewAvatarPacket(float colour, int iD) :
		Packet(AVATAR_NEW, 3 + to_string(colour).size()),
		avatarColour(colour),
		iD(iD)
	{ }

	NewAvatarPacket(enet_uint8* data) :
		Packet(AVATAR_NEW, *(data + 1)),
		iD(*(data + 2))
	{
		string colString = "";

		int variable = 0;
		for (int i = 3; i < size - 1; ++i)
		{
			colString += data[i];
		}
		avatarColour = stof(colString);
	}

	virtual enet_uint8* CreateByteStream() override
	{
		enet_uint8* data = new enet_uint8[size];

		data[0] = type;
		data[1] = size;
		data[2] = iD;

		string colString = to_string(avatarColour);

		for (int i = 0; i < colString.size(); ++i)
			*(data + 3 + i) = colString[i];

		//data[size - 1] = '\0';

		return data;
	}

public:
	float avatarColour;
	enet_uint8 iD;
};