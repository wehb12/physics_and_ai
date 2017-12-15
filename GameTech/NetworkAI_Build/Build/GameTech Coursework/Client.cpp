/******************************************************************************
Class: Client
Implements:
Author: Pieran Marris <p.marris@newcastle.ac.uk> and YOU!
Description:

:README:
- In order to run this demo, we also need to run "Tuts_Network_Server" at the same time.
- To do this:-
1. right click on the entire solution (top of the solution exporerer) and go to properties
2. Go to Common Properties -> Statup Project
3. Select Multiple Statup Projects
4. Select 'Start with Debugging' for both "Tuts_Network_Client" and "Tuts_Network_Server"

- Now when you run the program it will build and run both projects at the same time. =]
- You can also optionally spawn more instances by right clicking on the specific project
and going to Debug->Start New Instance.




This demo scene will demonstrate a very simple network example, with a single server
and multiple clients. The client will attempt to connect to the server, and say "Hellooo!"
if it successfully connects. The server, will continually broadcast a packet containing a
Vector3 position to all connected clients informing them where to place the server's player.

This designed as an example of how to setup networked communication between clients, it is
by no means the optimal way of handling a networked game (sending position updates at xhz).
If your interested in this sort of thing, I highly recommend finding a good book or an
online tutorial as there are many many different ways of handling networked game updates
all with varying pitfalls and benefits. In general, the problem always comes down to the
fact that sending updates for every game object 60+ frames a second is just not possible,
so sacrifices and approximations MUST be made. These approximations do result in a sub-optimal
solution however, so most work on networking (that I have found atleast) is on building
a network bespoke communication system that sends the minimal amount of data needed to
produce satisfactory results on the networked peers.


::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::: IF YOUR BORED! :::
::::::::::::::::::::::
1. Try setting up both the server and client within the same Scene (disabling collisions
on the objects as they now share the same physics engine). This way we can clearly
see the affect of packet-loss and latency on the network. There is a program called "Clumsy"
which is found within the root directory of this framework that allows you to inject
latency/packet loss etc on network. Try messing around with various latency/packet-loss
values.

2. Packet Loss
This causes the object to jump in large (and VERY noticable) gaps from one position to
another.

A good place to start in compensating for this is to build a buffer and store the
last x update packets, now if we miss a packet it isn't too bad as the likelyhood is
that by the time we need that position update, we will already have the next position
packet which we can use to interpolate that missing data from. The number of packets we
will need to backup will be relative to the amount of expected packet loss. This method
will also insert additional 'buffer' latency to our system, so we better not make it wait
too long.

3. Latency
There is no easy way of solving this, and will have all felt it's punishing effects
on all networked games. The way most games attempt to hide any latency is by actually
running different games on different machines, these will react instantly to your actions
such as shooting which the server will eventually process at some point and tell you if you
have hit anything. This makes it appear (client side) like have no latency at all as you
moving instantly when you hit forward and shoot when you hit shoot, though this is all smoke
and mirrors and the server is still doing all the hard stuff (except now it has to take into account
the fact that you shot at time - latency time).

This smoke and mirrors approach also leads into another major issue, which is what happens when
the instances are desyncrhonised. If player 1 shoots and and player 2 moves at the same time, does
player 1 hit player 2? On player 1's screen he/she does, but on player 2's screen he/she gets
hit. This leads to issues which the server has to decipher on it's own, this will also happen
alot with generic physical elements which will ocasional 'snap' back to it's actual position on
the servers game simulation. This methodology is known as "Dead Reckoning".

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


*//////////////////////////////////////////////////////////////////////////////

#include "PacketHandler.h"
#include "Client.h"

const Vector3 status_color3 = Vector3(1.0f, 0.6f, 0.6f);
const Vector4 status_color = Vector4(status_color3.x, status_color3.y, status_color3.z, 1.0f);

Client::Client()
	: Scene("Client")
	, serverConnection(NULL)
	, packetHandler(NULL)
	, mazeSize(16)
	, mazeDensity(1.0f)
	, lastDensity(1.0f)
	, start(true)
	, maze(NULL)
	, mazeRender(NULL)
	, ifMoved(false)
	, startIndex(0)
	, endIndex(0)
	, currentIndex(0)
	, clientID(0)
	, usePhysics(false)
	, stringPulling(false)
{
	GLuint whitetex;
	glGenTextures(1, &whitetex);
	glBindTexture(GL_TEXTURE_2D, whitetex);
	unsigned int pixel = 0xFFFFFFFF;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, &pixel);
	glBindTexture(GL_TEXTURE_2D, 0);

	wallmesh = new OBJMesh("../../Data/Meshes/cube.obj");
	wallmesh->SetTexture(whitetex);

	// seed the start and end node position generation
	// makes sure each client has a different default start/ end 
	srand(time(NULL));

	avatarPosition.push_back(Vector2(0, 0));
	avatarColour.push_back(0.67f);
}

void Client::OnInitializeScene()
{
	//Initialize Client Network
	if (network.Initialize(0))
	{
		NCLDebug::Log("Network: Initialized!");

		//Attempt to connect to the server on localhost:1234
		serverConnection = network.ConnectPeer(127, 0, 0, 1, 1234);
		NCLDebug::Log("Network: Attempting to connect to server.");
	}

	this->AddGameObject(CommonUtils::BuildCuboidObject(
		"Ground",
		Vector3(0.0f, -1.5f, 0.0f),
		Vector3(20.0f, 1.0f, 20.0f),
		false,
		0.0f,
		false,
		false,
		Vector4(0.2f, 0.5f, 1.0f, 1.0f)));
}

void Client::OnCleanupScene()
{
	Scene::OnCleanupScene();

				//Send one final packet telling the server we are disconnecting
				// - We are not waiting to resend this, so if it fails to arrive
				//   the server will have to wait until we time out naturally
	enet_peer_disconnect_now(serverConnection, 0);

	//Release network and all associated data/peer connections
	network.Release();
	serverConnection = NULL;

	mazeRender = NULL;
	maze = NULL;
}

void Client::OnUpdateScene(float dt)
{
	Scene::OnUpdateScene(dt);

	if (printPath)
		PrintPath();

	HandleKeyboardInput();

	//Update Network
	auto callback = std::bind(
		&Client::ProcessNetworkEvent,	// Function to call
		this,								// Associated class instance
		std::placeholders::_1);				// Where to place the first parameter
	network.ServiceNetwork(dt, callback);



	//Add Debug Information to screen
	uint8_t ip1 = serverConnection->address.host & 0xFF;
	uint8_t ip2 = (serverConnection->address.host >> 8) & 0xFF;
	uint8_t ip3 = (serverConnection->address.host >> 16) & 0xFF;
	uint8_t ip4 = (serverConnection->address.host >> 24) & 0xFF;

	NCLDebug::AddStatusEntry(status_color, "Network Traffic");
	NCLDebug::AddStatusEntry(status_color, "    Incoming: %5.2fKbps", network.m_IncomingKb);
	NCLDebug::AddStatusEntry(status_color, "    Outgoing: %5.2fKbps", network.m_OutgoingKb);
	
	if (maze)
	{
		Vector4 paramsColour = Vector4(1.0f, 0.2f, 0.2f, 1.0f);
		NCLDebug::AddStatusEntry(paramsColour, "");
		NCLDebug::AddStatusEntry(paramsColour, "Maze Parameters:");
		NCLDebug::AddStatusEntry(paramsColour, "    Current Maze Size: %d", maze->size);
		NCLDebug::AddStatusEntry(paramsColour, "    Current Maze Density: %5.2f", lastDensity);
		NCLDebug::AddStatusEntry(paramsColour, "");
		NCLDebug::AddStatusEntry(paramsColour, "    Next Maze Size: %d [1/2] to change", mazeSize);
		NCLDebug::AddStatusEntry(paramsColour, "    Next Maze Density: %5.2f [3/4] to change", mazeDensity);
		NCLDebug::AddStatusEntry(paramsColour, "    Use PhysicsEngine: %s [U] to toggle", usePhysics ? "YES" : "NO");
		NCLDebug::AddStatusEntry(paramsColour, "");
		NCLDebug::AddStatusEntry(paramsColour, "    Path Length: %d", pathLength);
		Vector4 controlsColour = Vector4(0.2f, 1.0f, 0.2f, 1.0f);
		NCLDebug::AddStatusEntry(controlsColour, "");
		NCLDebug::AddStatusEntry(controlsColour, "Scene Controls:");
		NCLDebug::AddStatusEntry(controlsColour, "    Move the %s position with the arrow keys", start ? "start" : "end");
		NCLDebug::AddStatusEntry(controlsColour, "    Swap to moving the %s position by pressing [CTRL]", start ? "end" : "start");
		NCLDebug::AddStatusEntry(controlsColour, "");
		NCLDebug::AddStatusEntry(controlsColour, "    Press [P] to toggle path");
		NCLDebug::AddStatusEntry(controlsColour, "    Press [L] to send start/ end data");
		NCLDebug::AddStatusEntry(controlsColour, "    String pulling %s: Press [K] to toggle", stringPulling ? "ENABLED" : "DISABLED");
	}
}

void Client::ProcessNetworkEvent(const ENetEvent& evnt)
{
	switch (evnt.type)
	{
		//New connection request or an existing peer accepted our connection request
	case ENET_EVENT_TYPE_CONNECT:
	{
		if (evnt.peer == serverConnection)
		{
			NCLDebug::Log(status_color3, "Network: Successfully connected to server!");

			//Send a 'hello' packet
			MessagePacket* message = new MessagePacket("Helloo!");
			packetHandler->SendPacket(serverConnection, message);
			delete message;
		}
	}
	break;

	//Server has sent us a new packet
	case ENET_EVENT_TYPE_RECEIVE:
	{
		packetHandler->HandlePacket(evnt.packet);
	}
	break;

	//Server has disconnected
	case ENET_EVENT_TYPE_DISCONNECT:
	{
		NCLDebug::Log(status_color3, "Network: Server has disconnected!");
	}
	break;
	}
}

void Client::HandleKeyboardInput()
{
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_G))
	{
		MazeParamsPacket* packet = new MazeParamsPacket(mazeSize, mazeDensity);
		lastDensity = mazeDensity;
		packetHandler->SendPacket(serverConnection, packet);
		usePhysics = false;
		ifMoved = false;
		delete packet;
	}

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_1))
		mazeSize = max(mazeSize - 1, MIN_MAZE_SIZE);
		
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_2))
		mazeSize = min(mazeSize + 1, MAX_MAZE_SIZE);

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_3))
		mazeDensity = max(mazeDensity - 0.1f, 0.0f);
	
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_4))
		mazeDensity = min(mazeDensity + 0.1f, 1.0f);

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_P))
		printPath = !printPath;

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_U))
	{
		if (maze)
		{
			usePhysics = !usePhysics;
			ToggleBooleanPacket* physPacket = new ToggleBooleanPacket(TOGGLE_PHYSICS, usePhysics);
			packetHandler->SendPacket(serverConnection, physPacket);
			delete physPacket;
		}
	}

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_K))
	{
		if (maze)
		{
			stringPulling = !stringPulling;
			ToggleBooleanPacket* stringPacket = new ToggleBooleanPacket(TOGGLE_STRING_PULLING, stringPulling);
			packetHandler->SendPacket(serverConnection, stringPacket);
			delete stringPacket;
		}
	}

	// resend destination positions to server
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_L))
	{
		if (maze)
		{
			maze->start = &startNode;
			maze->end = &endNode;
			packetHandler->SendPositionPacket(serverConnection, maze, avatarColour[0]);
			if (!ifMoved)
			{
				UpdatePosition(true);
				if (pathLength > 0)
					startIndex = path[currentIndex];
			}
			ifMoved = false;
		}
	}

	if (maze)
	{
		bool moved = false;
		int index = start ? startIndex : endIndex;

		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_UP))
		{
			MoveNodeUp(start, index);
			moved = true;
		}
		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_DOWN))
		{
			MoveNodeDown(start, index);
			moved = true;
		}
		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_LEFT))
		{
			MoveNodeLeft(start, index);
			moved = true;
		}
		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_RIGHT))
		{
			MoveNodeRight(start, index);
			moved = true;
		}
		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_CONTROL))
			start = !start;

		if (moved)
		{
			startNode = *maze->start;
			endNode = *maze->end;
			// remake MazeRenderer start and end RenderNodes
			UpdatePosition(start);
			ifMoved = start;
		}
	}
}

void Client::MoveNodeDown(bool start, int index)
{
	index = index < (maze->size) * (maze->size - 1) ? index + maze->size : index;
	if (start)
	{
		startIndex = index;
		maze->start = &maze->allNodes[index];
	}
	else
	{
		endIndex = index;
		maze->end = &maze->allNodes[index];
	}

}

void Client::MoveNodeUp(bool start, int index)
{
	index = index >= maze->size ? index - maze->size : index;
	if (start)
	{
		startIndex = index;
		maze->start = &maze->allNodes[index];
	}
	else
	{
		endIndex = index;
		maze->end = &maze->allNodes[index];
	}
}

void Client::MoveNodeLeft(bool start, int index)
{
	index = index % (maze->size) > 0 ? index - 1 : index;
	if (start)
	{
		startIndex = index;
		maze->start = &maze->allNodes[index];
	}
	else
	{
		endIndex = index;
		maze->end = &maze->allNodes[index];
	}
}

void Client::MoveNodeRight(bool start, int index)
{
	index = index % (maze->size) < (maze->size - 1) ? index + 1 : index;
	if (start)
	{
		startIndex = index;
		maze->start = &maze->allNodes[index];
	}
	else
	{
		endIndex = index;
		maze->end = &maze->allNodes[index];
	}
}

void Client::UpdatePosition(bool ifStart)
{
	float scalar = 1.0f / (maze->size * 3 - 1);
	Vector3 cellsize = Vector3(
		scalar * 2,
		1.0f,
		scalar * 2
	);

	if (ifStart)
	{
		Vector3 cellpos = Vector3(
			startNode._pos.x * 3,
			0.0f,
			startNode._pos.y * 3
		) * scalar;

		mazeRender->UpdateStartNodeTransform(Matrix4::Translation(cellpos + cellsize * 0.5f) * Matrix4::Scale(cellsize * 0.5f));
	}
	else
	{
		GraphNode* end = maze->end;

		Vector3 cellpos = Vector3(
			endNode._pos.x * 3,
			0.0f,
			endNode._pos.y * 3
		) * scalar;
		
		mazeRender->UpdateEndNodeTransform(Matrix4::Translation(cellpos + cellsize * 0.5f) * Matrix4::Scale(cellsize * 0.5f));
	}
}

void Client::PrintPath()
{
	if (maze && mazeRender)
	{
		float grid_scalar = 1.0f / (float)maze->GetSize();
		float col_factor = 1 / (float)pathLength;

		Matrix4 transform = mazeRender->Render()->GetWorldTransform();

		for (int i = 0; i < pathLength - 1; ++i)
		{
			Vector3 start = transform * Vector3(
				(maze->allNodes[path[i]]._pos.x + 0.5f) * grid_scalar,
				0.1f,
				(maze->allNodes[path[i]]._pos.y + 0.5f) * grid_scalar);

			Vector3 end = transform * Vector3(
				(maze->allNodes[path[i + 1]]._pos.x + 0.5f) * grid_scalar,
				0.1f,
				(maze->allNodes[path[i + 1]]._pos.y + 0.5f) * grid_scalar);

			NCLDebug::DrawThickLine(start, end, 2.5f / maze->size, CommonUtils::GenColor(i * col_factor));
		}
	}
}

void Client::GenerateEmptyMaze()
{
	SAFE_DELETE(maze);

	maze = new MazeGenerator();
	maze->Generate(mazeSize, 0.0f);
}

void Client::GenerateWalledMaze(bool* walledEdges, int numEdges)
{
	GenerateEmptyMaze();
	
	for (int i = 0; i < numEdges; ++i)
	{
		if (walledEdges[i])
			maze->allEdges[i]._iswall = true;
	}

	auto GrabIndex = [&](GraphNode* node)
	{
		for (int i = 0; i < (mazeSize * mazeSize); ++i)
		{
			if (maze->allNodes[i]._pos == node->_pos)
				return i;
		}
		return -1;
	};

	startIndex = GrabIndex(maze->start);
	endIndex = GrabIndex(maze->end);

	startNode = *maze->start;
	endNode = *maze->end;

	avatarPosition[0].x = maze->start->_pos.x;
	avatarPosition[0].y = maze->start->_pos.y;

	SAFE_DELETE(path);
	pathLength = 0;
}

void Client::RenderNewMaze()
{
	if (mazeRender)
	{
		RemoveGameObject(mazeRender);
		//delete mazeRender;
		mazeRender = NULL;
	}

	mazeRender = new MazeRenderer(maze, wallmesh);

	//The maze is returned in a [0,0,0] - [1,1,1] cube (with edge walls outside) regardless of grid_size,
	// so we need to scale it to whatever size we want
	Matrix4 maze_scalar = Matrix4::Scale(Vector3(5.f, 5.0f / float(mazeSize), 5.f)) * Matrix4::Translation(Vector3(-0.5f, 0.f, -0.5f));
	mazeRender->Render()->SetTransform(maze_scalar);
	AddGameObject(mazeRender);

	// now send back to the server the start and end positions
	avatarPosition[0].x = maze->start->_pos.x;
	avatarPosition[0].y = maze->start->_pos.y;

	for (int i = 0; i < avatarColour.size(); ++i)
		AddAvatar(avatarColour[i]);
}

void Client::PopulatePath(Packet* pathPacket)
{
	if (path)
	{
		delete[] path;
		path = NULL;
	}
	switch (pathPacket->type)
	{
		case (MAZE_PATH8):
		{
			MazePathPacket8* packet8 = static_cast<MazePathPacket8*>(pathPacket);
			pathLength = packet8->length;
			path = new int[pathLength];
			for (int i = 0; i < pathLength; ++i)
				path[i] = packet8->path[i];

			break;
		}
		case (MAZE_PATH16):
		{
			MazePathPacket16* packet16 = static_cast<MazePathPacket16*>(pathPacket);
			pathLength = packet16->length;
			path = new int[pathLength];
			for (int i = 0; i < pathLength; ++i)
				path[i] = packet16->path[i];

			break;
		}
	}

	InstructionCompletePacket* complete = new InstructionCompletePacket(true);
	packetHandler->SendPacket(serverConnection, complete);
	delete complete;
}

void Client::UpdateAvatar(int iD)
{
	float scalar = 1.0f / (maze->size * 3 - 1);
	Vector3 cellsize = Vector3(
		scalar * 2,
		2.0f,
		scalar * 2
	);

	GraphNode* start = maze->start;

	Vector3 cellpos = Vector3(
		avatarPosition[iD].x * 3,
		0.0f,
		avatarPosition[iD].y * 3
	) * scalar;

	mazeRender->UpdateAvatarTransform(Matrix4::Translation(cellpos + cellsize * 0.5f) * Matrix4::Scale(cellsize * 0.5f), iD);

	if (iD == 0 && !ifMoved)
	{
		if (currentIndex < pathLength)
			startNode = maze->allNodes[path[currentIndex]];
	}
}

void Client::AddAvatar(float colour)
{
	float startPos = -1 - avatarPosition.size() * 0.2;
	avatarPosition.push_back(Vector2(startPos, 0));
	RenderNode* avatar;

	float scalar = 1.0f / (maze->size * 3 - 1);
	Vector3 cellsize = Vector3(
		scalar * 2,
		2.0f,
		scalar * 2
	);

	Vector3 cellpos = Vector3(
		startPos * 3,
		0.0f,
		0.0f
	) * scalar;

	avatar = new RenderNode(wallmesh, CommonUtils::GenColor(colour));
	avatar->SetTransform(Matrix4::Translation(cellpos + cellsize * 0.5f) * Matrix4::Scale(cellsize * 0.5f));
	mazeRender->AddAvatar(avatar);
}