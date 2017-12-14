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

class NewAvatarPacket : public Packet
{
public:
	NewAvatarPacket(float posX, float posY, float colour, int iD) :
		Packet(AVATAR_NEW, 6 + to_string(posX).size() + to_string(posY).size() + to_string(colour).size()),
		posX(posX),
		posY(posY),
		avatarColour(colour),
		iD(iD)
	{ }

	NewAvatarPacket(Vector2 pos, float colour, int iD) :
		Packet(AVATAR_NEW, 6 + to_string(pos.x).size() + to_string(pos.y).size() + to_string(colour).size()),
		posX(pos.x),
		posY(pos.y),
		avatarColour(colour),
		iD(iD)
	{ }

	NewAvatarPacket(enet_uint8* data) :
		Packet(AVATAR_NEW, *(data + 1)),
		iD(*(data + 2))
	{
		string xString = "";
		string yString = "";
		string colString = "";

		int variable = 0;
		for (int i = 2; i < size - 1; ++i)
		{
			if (data[i] == ' ')
			{
				++variable;
				continue;
			}
			else
			{
				switch (variable)
				{
					case 0:
						xString += data[i];
						break;
					case 1:
						yString += data[i];
						break;
					case 2:
						colString += data[i];
						break;
				}
			}
		}
		posX = stof(xString);
		posY = stof(yString);
		avatarColour = stof(colString);
	}

	virtual enet_uint8* CreateByteStream() override
	{
		enet_uint8* data = new enet_uint8[size];

		data[0] = type;
		data[1] = size;
		data[2] = iD;

		string xString = to_string(posX);
		string yString = to_string(posY);
		string colString = to_string(avatarColour);

		for (int i = 0; i < xString.size(); ++i)
			*(data + 2 + i) = xString[i];

		*(data + 2 + xString.size()) = ' ';

		for (int i = 0; i < yString.size(); ++i)
			*(data + 3 + xString.size() + i) = yString[i];

		*(data + 3 + xString.size() + yString.size()) = ' ';

		for (int i = 0; i < colString.size(); ++i)
			*(data + 4 + xString.size() + yString.size() + i) = colString[i];

		data[size - 1] = '\0';

		return data;
	}

public:
	float posX;
	float posY;
	float avatarColour;
	enet_uint8 iD;
};

class TogglePhysicsPacket : public Packet
{
public:
	TogglePhysicsPacket(bool usePhysics) : 
		Packet(TOGGLE_PHYSICS, 2),
		usePhysics(usePhysics)
	{ }

	TogglePhysicsPacket(enet_uint8* data) :
		Packet(TOGGLE_PHYSICS, 2),
		usePhysics(*(data +1) == SUCCESS)
	{ }

	virtual enet_uint8* CreateByteStream()
	{
		enet_uint8* data = new enet_uint8[size];

		data[0] = type;
		data[1] = usePhysics ? SUCCESS : FAILURE;
		
		return data;
	}

public:
	bool usePhysics;
};