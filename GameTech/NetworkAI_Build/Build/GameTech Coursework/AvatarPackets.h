#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include "Packet.h"
#include "../nclgl/Vector2.h"
#include <string>

class AvatarPositionPacket : public Packet
{
public:
	AvatarPositionPacket(float posX, float posY) :
		Packet(AVATAR_POSITION, 4 + to_string(posX).size() + to_string(posY).size()),
		posX(posX),
		posY(posY)
	{ }

	AvatarPositionPacket(Vector2 pos) :
		Packet(AVATAR_POSITION, 4 + to_string(pos.x).size() + to_string(pos.y).size()),
		posX(pos.x),
		posY(pos.y)
	{ }

	AvatarPositionPacket(enet_uint8* data) :
		Packet(AVATAR_POSITION, *(data + 1))
	{
		string xString = "";
		string yString = "";

		bool x = true;
		for (int i = 2; i < size - 1; ++i)
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

		string xString = to_string(posX);
		string yString = to_string(posY);

		for (int i = 0; i < xString.size(); ++i)
			*(data + 2 + i) = xString[i];

		*(data + 2 + xString.size()) = ' ';

		for (int i = 0; i < yString.size(); ++i)
			*(data + 3 + xString.size() + i) = yString[i];

		data[size - 1] = '\0';

		return data;
	}

public:
	float posX;
	float posY;
};