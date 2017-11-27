#include <math.h>
#include <iostream>
#include <stdlib.h>

using namespace std;

#define QUEUESIZE 128	// Defines maximum size of our priority queues - can be tuned based on map
#define MAPSIZE 1024	// Size of the map we're considering
#define XSIZE 32		// Size of X Axis
#define YSIZE 32		// Size of Y Axis

/* mapData charts our considered area in terms of grid squares, each square represented by a value
of 0 or 1. 0 indicates an impassable grid square. 1 indicates a passable grid square */

unsigned char mapData[MAPSIZE] = {	1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
									1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
									0,0,0,1,0,0,0,1,0,1,1,1,0,1,0,1,1,1,1,1,0,0,1,0,0,0,1,0,1,0,1,0,
									0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
									0,1,0,1,0,1,0,0,0,1,1,1,1,1,0,1,1,1,0,0,1,1,1,0,1,0,1,0,1,0,1,0,
									0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,0,
									0,1,0,1,0,1,0,1,0,1,0,0,0,0,1,1,1,1,1,1,1,1,1,0,1,0,1,0,1,0,1,0,
									0,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
									0,0,0,1,0,1,0,1,0,1,0,1,0,0,0,1,0,0,0,0,1,1,1,0,1,0,1,0,1,0,1,0,
									0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,0,
									0,1,1,1,0,1,0,1,0,1,1,1,1,0,0,1,0,0,0,1,0,0,1,1,1,1,1,1,1,1,1,0,
									0,1,1,1,0,1,0,1,0,1,1,1,1,1,0,1,0,1,1,1,0,0,1,1,1,1,1,1,1,1,1,0,
									0,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,0,1,0,1,1,1,1,0,1,1,1,0,1,1,0,0,
									0,1,1,1,0,0,1,1,1,1,0,0,1,1,1,1,0,1,1,1,1,1,1,0,1,1,1,0,1,1,0,0,
									0,1,0,1,0,1,1,1,1,1,1,1,1,1,0,1,0,1,0,0,1,1,1,1,1,1,0,0,1,1,0,0,
									0,1,0,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,0,0,1,0,0,0,
									0,1,1,1,0,1,0,1,0,1,0,0,1,1,1,1,1,0,1,1,0,0,1,1,1,1,1,1,1,0,1,0,
									0,1,1,1,0,1,0,1,0,1,0,0,1,1,0,0,1,0,1,1,0,0,1,1,1,1,1,1,1,1,1,0,
									0,0,0,1,1,1,0,1,0,1,1,1,1,1,0,0,1,1,1,1,1,1,1,0,1,1,1,0,1,1,1,0,
									0,0,0,1,1,1,0,1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,1,0,1,1,1,0,1,1,1,0,
									0,1,1,1,0,1,1,1,0,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,0,
									0,1,1,1,0,1,1,1,0,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,0,
									0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
									0,1,0,1,0,1,0,1,0,1,0,0,1,1,0,0,1,1,0,0,1,1,1,0,1,0,1,0,1,0,1,0,
									0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,0,
									0,1,0,1,0,1,0,1,0,1,1,1,0,0,0,0,1,1,0,0,1,1,1,0,1,0,1,0,1,0,1,0,
									0,1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
									0,1,0,1,0,1,0,1,0,1,0,0,0,0,0,0,1,1,0,0,1,1,1,0,1,0,1,0,1,0,1,0,
									0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
									0,1,0,1,0,1,0,1,0,1,1,1,0,1,1,1,1,1,0,0,1,1,1,0,1,0,1,0,1,0,1,0,
									0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
									0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1	};

int numberOfDirections = 4;		// This defines the maximum number of directions an agent can choose
								// from - a diagonal solution would have 8 instead of 4
int xAxis[4] = {1, 0, -1, 0};	// xAxis and yAxis define the relative movement for each of the four
int yAxis[4] = {0, 1, 0, -1};	// directions our agent can choose from. For example, direction '0'
								// increases x by 1, without changing y.

typedef struct NavigationNode	// Our NavigationNode structure includes the coordinates of a given
{								// node, along with its g value (cost to reach it) and f value
	unsigned char x;			// (g + the 'h' heuristic estimation to reach the goal). Note that
	unsigned char y;			// we don't need to store 'h'.
	
	unsigned char g;
	unsigned char f;
} NavigationNode;

typedef struct AStarQueue		// Our priority queue structure - contains an array of NavigationNodes
{								// and variables that allow us to manage our priority queue
	NavigationNode node[QUEUESIZE];
	unsigned char queueSize;	// Size of the queue. Shocking, I know
	unsigned char tempF;		// Holding variable for a member's f value - used in 'pushing' into the queue
	unsigned char pushF;		// Holding variable for the pushed f value - used in 'pushing' into the queue
	unsigned char tempID;		// Point of insertion into the queue - reset every 'push'
} AStarQueue;

typedef struct LeanPathingData	// LeanPathingData contains two AStarQueues, a map of openNodes, a map of
{								// closedNodes, and a map of traversed directions
	AStarQueue naviQueue[2];
	unsigned char openNodes[MAPSIZE];
	unsigned char closedNodes[MAPSIZE];
	unsigned char directionHistory[MAPSIZE];
} LeanPathingData;

/* Our calculateAStarPath function accepts start and goal coordinates, and a pointer to the results array. The
function calculates the path and returns the first step to the results array. Note that our solution is based
entirely on the stack and avoids the STL - common examples online utilise the heap and either the <vector> or
<queue> headers */

void calculateAStarPath(unsigned char xStart, unsigned char yStart, unsigned char xGoal, unsigned char yGoal, unsigned char* results)
{
	LeanPathingData naviComp;		// Instance of a LeanPathingData struct we call naviComp

	unsigned char pathFound;		// Checker to see if a path has been found
	unsigned char queueIndex;		// queueIndex tells us which of the AStarQueues in naviComp we are operating upon
	
	unsigned char stopX;			// Catcher variable
	unsigned char stopY;			// Catcher variable
	
	unsigned char nextX, nextY;		// The x and y coordinates of the next step on our agent's path

	int i, j, k, l;					// Pre-declared iterators
	unsigned char x, y, xChange, yChange;	// Pre-declared variables; x and y are the coordinates of our 'current'
											// node; xChange and yChange refer to the nodes connected to (x,y)
	queueIndex  = 0;	// Here
	pathFound = 0;		// we
	nextX = 255;		// set
	nextY = 255;		// up
	stopX = 255;		// our
	stopY = 255;		// variables
	
	for(i = 0; i < MAPSIZE; i++)		// Declare the open and closed lists
	{
		naviComp.openNodes[i] = 0;
		naviComp.closedNodes[i] = 0;
	}
	
	naviComp.naviQueue[queueIndex].queueSize = 0;		// Set queue
	naviComp.naviQueue[1 - queueIndex].queueSize = 0;	// sizes

	NavigationNode goalNode;
	goalNode.x = xGoal;
	goalNode.y = yGoal;
	
	/* Set up startNode with the xStart and yStart coordinates; set g to 0 (it
	costs nothing to get to where we are, after all!), and f = 0 + the
	Manhattan Distance heuristic to our goal node. See the Tutorial for
	exploration of heuristics */

	NavigationNode startNode;
	startNode.x = xStart;
	startNode.y = yStart;
	startNode.g = 0;
	startNode.f = abs(startNode.x - goalNode.x) + abs(startNode.y - goalNode.y);
	
	naviComp.naviQueue[queueIndex].node[0] = startNode;	// 'Push' start node onto
	naviComp.naviQueue[queueIndex].queueSize++;			// queue, and increase queueSize
	
	naviComp.openNodes[(startNode.x * YSIZE) + startNode.y] = startNode.f;	// Add to Open List
	
	/* If queueSize ever equals 0, there is no path. pathFound is our general escape from the
	function loop */

	while(naviComp.naviQueue[queueIndex].queueSize != 0 && pathFound == 0)
	{
		NavigationNode currentNode;		// Our current Node!
		
		currentNode = naviComp.naviQueue[queueIndex].node[0];	// Draw currentNode from the queue!
		
		x = currentNode.x;	// Update x and y
		y = currentNode.y;
		
		/* The next few lines are our 'Push' equivalent on our queue. What do you think we could
		do to our queue structure to make this function cheaper? */

		for( k = 0; k < naviComp.naviQueue[queueIndex].queueSize - 1; k++)
		{
			naviComp.naviQueue[queueIndex].node[k] = naviComp.naviQueue[queueIndex].node[k + 1];
		}
		naviComp.naviQueue[queueIndex].queueSize--;
		naviComp.naviQueue[queueIndex].node[naviComp.naviQueue[queueIndex].queueSize].f = 255;
		
		naviComp.openNodes[(x * YSIZE) + y] = 0;		// No longer open
		naviComp.closedNodes[(x * YSIZE) + y] = 1;		// Is closed
		
		if(x == goalNode.x && y == goalNode.y)			// Check to see if we have gotten home!
		{
			pathFound = 1;		// If we've gotten home, update pathFound
			stopX = x;			// Confirm our stopping point to
			stopY = y;			// enable our backtracking!
		}
		else
		{
			for(i = 0; i < numberOfDirections; i++)		// Check all four directions!
			{
				xChange=x+xAxis[i];		// Set up xChange and yChange
				yChange=y+yAxis[i];
				
				/* This check ensures our considered square is within the boundaries of the map,
				to allow for edge cases. It also checks that our square is not on the closed
				list, and that it's not impassable*/

				if(xChange >= 0 && xChange < XSIZE && yChange >= 0
					&& yChange < YSIZE  && naviComp.closedNodes[(xChange * YSIZE) + yChange] != 1
					&& (mapData[(xChange * YSIZE) + yChange] == 1))
				{
					NavigationNode nextNode;				// Generate and populate nextNode. Again, note the
					nextNode.x = xChange;					// calculation of 'f' using the Manhattan Distance
					nextNode.y = yChange;					// heuristic
					nextNode.g = currentNode.g;
					nextNode.f = nextNode.g + (abs(nextNode.x - goalNode.x) + abs(nextNode.y - goalNode.y));
					
					if(naviComp.openNodes[(xChange * YSIZE) + yChange] == 0)	// If node is not in the Open List...
					{
						naviComp.openNodes[(xChange * YSIZE) + yChange] = nextNode.f;	// Add it to the Open List...
						
						/* ... and push it to the queue. Think of the next if-else function as
						comparable to the push function in the vector container, except it sorts
						as it pushes. If there's nothing in the queue, this is simple. If there're
						other nodes in the queue, it requires us to check the pre-existing f values
						of nodes within the list, and 'slip' our considered node in where it fits in
						accordance with its own 'f' value.*/

						if(naviComp.naviQueue[queueIndex].queueSize == 0)
						{
							naviComp.naviQueue[queueIndex].node[0].x = nextNode.x;
							naviComp.naviQueue[queueIndex].node[0].y = nextNode.y;
							naviComp.naviQueue[queueIndex].node[0].g = nextNode.g;
							naviComp.naviQueue[queueIndex].node[0].f = nextNode.f;
							naviComp.naviQueue[queueIndex].queueSize++;
						}
						else
						{
							naviComp.naviQueue[queueIndex].pushF = nextNode.f;
							naviComp.naviQueue[queueIndex].tempID = 0;
							naviComp.naviQueue[queueIndex].tempF = naviComp.naviQueue[queueIndex].node[naviComp.naviQueue[queueIndex].tempID].f;
							
							while(naviComp.naviQueue[queueIndex].tempF < naviComp.naviQueue[queueIndex].pushF)
							{
								naviComp.naviQueue[queueIndex].tempID++;
								naviComp.naviQueue[queueIndex].tempF = naviComp.naviQueue[queueIndex].node[naviComp.naviQueue[queueIndex].tempID].f;
							}
							
							for(l = naviComp.naviQueue[queueIndex].queueSize; l > naviComp.naviQueue[queueIndex].tempID; l--)
							{
								naviComp.naviQueue[queueIndex].node[l].x = naviComp.naviQueue[queueIndex].node[l-1].x;
								naviComp.naviQueue[queueIndex].node[l].y = naviComp.naviQueue[queueIndex].node[l-1].y;
								naviComp.naviQueue[queueIndex].node[l].g = naviComp.naviQueue[queueIndex].node[l-1].g;
								naviComp.naviQueue[queueIndex].node[l].f = naviComp.naviQueue[queueIndex].node[l-1].f;
							}
							naviComp.naviQueue[queueIndex].node[naviComp.naviQueue[queueIndex].tempID].x = nextNode.x;
							naviComp.naviQueue[queueIndex].node[naviComp.naviQueue[queueIndex].tempID].y = nextNode.y;
							naviComp.naviQueue[queueIndex].node[naviComp.naviQueue[queueIndex].tempID].g = nextNode.g;
							naviComp.naviQueue[queueIndex].node[naviComp.naviQueue[queueIndex].tempID].f = nextNode.f;
							
							naviComp.naviQueue[queueIndex].queueSize++;
						}

						/* Here, we insert the opposite direction to the direction we're considering - for example, if we have
						journeyed (0,1) from our currentNode to our nextNode, this direction would be (0, -1). This is necessary
						for when we backtrack from the goal to the start. What can we do to the structure of our function to make
						this unnecessary? */

						naviComp.directionHistory[(xChange * YSIZE) + yChange] = (i+numberOfDirections/2)%numberOfDirections;
					}
					else if(naviComp.openNodes[(xChange * YSIZE) + yChange] > nextNode.f)	// Checks to see if this node is in the
																							// Open List at a higher 'f' cost. If
																							// approaching nextNode from currentNode is
																							// cheaper, go with the cheaper option!
					{
						/* Update the 'f' value in the Open List. Update the directionHistory, as in the above case. Now, we copy the contents
						of our first queue to the second queue, erasing them as we go, until the first queue's first element is nextNode. In
						practical terms, this means we 'push' onto the second queue, and pop from the first. */

						naviComp.openNodes[(xChange * YSIZE) + yChange] = nextNode.f;
						naviComp.directionHistory[(xChange * YSIZE) + yChange] = (i+numberOfDirections/2)%numberOfDirections;
						
						while(!(xChange == naviComp.naviQueue[queueIndex].node[0].x && yChange == naviComp.naviQueue[queueIndex].node[0].y))
						{
							if(naviComp.naviQueue[1 - queueIndex].queueSize < 1)
							{
								naviComp.naviQueue[1 - queueIndex].node[0].x = naviComp.naviQueue[queueIndex].node[0].x;
								naviComp.naviQueue[1 - queueIndex].node[0].y = naviComp.naviQueue[queueIndex].node[0].y;
								naviComp.naviQueue[1 - queueIndex].node[0].g = naviComp.naviQueue[queueIndex].node[0].g;
								naviComp.naviQueue[1 - queueIndex].node[0].f = naviComp.naviQueue[queueIndex].node[0].f;
								naviComp.naviQueue[1 - queueIndex].queueSize++;
							}
							else
							{
								naviComp.naviQueue[1 - queueIndex].pushF = naviComp.naviQueue[queueIndex].node[0].f;
								naviComp.naviQueue[1 - queueIndex].tempID = 0;
								naviComp.naviQueue[1 - queueIndex].tempF = naviComp.naviQueue[1 - queueIndex].node[naviComp.naviQueue[1 - queueIndex].tempID].f;
								
								while(naviComp.naviQueue[1 - queueIndex].tempF < naviComp.naviQueue[1 - queueIndex].pushF)
								{
									naviComp.naviQueue[1 - queueIndex].tempID++;
									naviComp.naviQueue[1 - queueIndex].tempF = naviComp.naviQueue[1 - queueIndex].node[naviComp.naviQueue[1 - queueIndex].tempID].f;
								}
								
								for(l = naviComp.naviQueue[1 - queueIndex].queueSize; l > naviComp.naviQueue[1 - queueIndex].tempID; l--)
								{
									naviComp.naviQueue[1 - queueIndex].node[l].x = naviComp.naviQueue[1 - queueIndex].node[l-1].x;
									naviComp.naviQueue[1 - queueIndex].node[l].y = naviComp.naviQueue[1 - queueIndex].node[l-1].y;
									naviComp.naviQueue[1 - queueIndex].node[l].g = naviComp.naviQueue[1 - queueIndex].node[l-1].g;
									naviComp.naviQueue[1 - queueIndex].node[l].f = naviComp.naviQueue[1 - queueIndex].node[l-1].f;								
								}
								
								naviComp.naviQueue[1 - queueIndex].node[naviComp.naviQueue[1 - queueIndex].tempID].x = naviComp.naviQueue[queueIndex].node[0].x;
								naviComp.naviQueue[1 - queueIndex].node[naviComp.naviQueue[1 - queueIndex].tempID].y = naviComp.naviQueue[queueIndex].node[0].y;
								naviComp.naviQueue[1 - queueIndex].node[naviComp.naviQueue[1 - queueIndex].tempID].g = naviComp.naviQueue[queueIndex].node[0].g;
								naviComp.naviQueue[1 - queueIndex].node[naviComp.naviQueue[1 - queueIndex].tempID].f = naviComp.naviQueue[queueIndex].node[0].f;
								
								naviComp.naviQueue[1 - queueIndex].queueSize++;
							}
							
							for( k = 0; k < naviComp.naviQueue[queueIndex].queueSize - 1; k++)
							{
								naviComp.naviQueue[queueIndex].node[k] = naviComp.naviQueue[queueIndex].node[k + 1];
							}
							naviComp.naviQueue[queueIndex].queueSize--;
							naviComp.naviQueue[queueIndex].node[naviComp.naviQueue[queueIndex].queueSize].f = 255;
						}
						
						for( k = 0; k < naviComp.naviQueue[queueIndex].queueSize - 1; k++)
						{
							naviComp.naviQueue[queueIndex].node[k] = naviComp.naviQueue[queueIndex].node[k + 1];
						}
						naviComp.naviQueue[queueIndex].queueSize--;
						naviComp.naviQueue[queueIndex].node[naviComp.naviQueue[queueIndex].queueSize].f = 255;
						
						/* Now that's done, if the first queue is bigger than the second queue, we push from
						the second queue onto the first, and pop from the second. Lastly, we push nextNode onto
						the first queue. */

						if(naviComp.naviQueue[queueIndex].queueSize > naviComp.naviQueue[1 - queueIndex].queueSize)
						{
							queueIndex = 1 - queueIndex;
												
							while(naviComp.naviQueue[queueIndex].queueSize != 0)
							{
								if(naviComp.naviQueue[1 - queueIndex].queueSize < 1)
								{
									naviComp.naviQueue[1 - queueIndex].node[0].x = naviComp.naviQueue[queueIndex].node[0].x;
									naviComp.naviQueue[1 - queueIndex].node[0].y = naviComp.naviQueue[queueIndex].node[0].y;
									naviComp.naviQueue[1 - queueIndex].node[0].g = naviComp.naviQueue[queueIndex].node[0].g;
									naviComp.naviQueue[1 - queueIndex].node[0].f = naviComp.naviQueue[queueIndex].node[0].f;
									naviComp.naviQueue[1 - queueIndex].queueSize++;
								}
								else
								{
									naviComp.naviQueue[1 - queueIndex].pushF = naviComp.naviQueue[queueIndex].node[0].f;
									naviComp.naviQueue[1 - queueIndex].tempID = 0;
									naviComp.naviQueue[1 - queueIndex].tempF = naviComp.naviQueue[1 - queueIndex].node[naviComp.naviQueue[1 - queueIndex].tempID].f;
								
									while(naviComp.naviQueue[1 - queueIndex].tempF < naviComp.naviQueue[1 - queueIndex].pushF)
									{
										naviComp.naviQueue[1 - queueIndex].tempID++;
										naviComp.naviQueue[1 - queueIndex].tempF = naviComp.naviQueue[1 - queueIndex].node[naviComp.naviQueue[1 - queueIndex].tempID].f;
									}
								
									for(l = naviComp.naviQueue[1 - queueIndex].queueSize; l > naviComp.naviQueue[1 - queueIndex].tempID; l--)
									{
										naviComp.naviQueue[1 - queueIndex].node[l].x = naviComp.naviQueue[1 - queueIndex].node[l-1].x;
										naviComp.naviQueue[1 - queueIndex].node[l].y = naviComp.naviQueue[1 - queueIndex].node[l-1].y;
										naviComp.naviQueue[1 - queueIndex].node[l].g = naviComp.naviQueue[1 - queueIndex].node[l-1].g;
										naviComp.naviQueue[1 - queueIndex].node[l].f = naviComp.naviQueue[1 - queueIndex].node[l-1].f;								
									}
								
									naviComp.naviQueue[1 - queueIndex].node[naviComp.naviQueue[1 - queueIndex].tempID].x = naviComp.naviQueue[queueIndex].node[0].x;
									naviComp.naviQueue[1 - queueIndex].node[naviComp.naviQueue[1 - queueIndex].tempID].y = naviComp.naviQueue[queueIndex].node[0].y;
									naviComp.naviQueue[1 - queueIndex].node[naviComp.naviQueue[1 - queueIndex].tempID].g = naviComp.naviQueue[queueIndex].node[0].g;
									naviComp.naviQueue[1 - queueIndex].node[naviComp.naviQueue[1 - queueIndex].tempID].f = naviComp.naviQueue[queueIndex].node[0].f;
								
									naviComp.naviQueue[1 - queueIndex].queueSize++;
								}
							
								for( k = 0; k < naviComp.naviQueue[queueIndex].queueSize - 1; k++)
								{
									naviComp.naviQueue[queueIndex].node[k] = naviComp.naviQueue[queueIndex].node[k + 1];
								}
								naviComp.naviQueue[queueIndex].queueSize--;
								naviComp.naviQueue[queueIndex].node[naviComp.naviQueue[queueIndex].queueSize].f = 255;
							}
						
						
							queueIndex = 1 - queueIndex;
						
							if(naviComp.naviQueue[queueIndex].queueSize == 0)
							{
								naviComp.naviQueue[queueIndex].node[0].x = nextNode.x;
								naviComp.naviQueue[queueIndex].node[0].y = nextNode.y;
								naviComp.naviQueue[queueIndex].node[0].g = nextNode.g;
								naviComp.naviQueue[queueIndex].node[0].f = nextNode.f;
								naviComp.naviQueue[queueIndex].queueSize++;
							}
							else
							{
								naviComp.naviQueue[queueIndex].pushF = nextNode.f;
								naviComp.naviQueue[queueIndex].tempID = 0;
								naviComp.naviQueue[queueIndex].tempF = naviComp.naviQueue[queueIndex].node[naviComp.naviQueue[queueIndex].tempID].f;
							
								while(naviComp.naviQueue[queueIndex].tempF < naviComp.naviQueue[queueIndex].pushF)
								{
									naviComp.naviQueue[queueIndex].tempID++;
									naviComp.naviQueue[queueIndex].tempF = naviComp.naviQueue[queueIndex].node[naviComp.naviQueue[queueIndex].tempID].f;
								}
							
								for(l = naviComp.naviQueue[queueIndex].queueSize; l > naviComp.naviQueue[queueIndex].tempID; l--)
								{
									naviComp.naviQueue[queueIndex].node[l].x = naviComp.naviQueue[queueIndex].node[l-1].x;
									naviComp.naviQueue[queueIndex].node[l].y = naviComp.naviQueue[queueIndex].node[l-1].y;
									naviComp.naviQueue[queueIndex].node[l].g = naviComp.naviQueue[queueIndex].node[l-1].g;
									naviComp.naviQueue[queueIndex].node[l].f = naviComp.naviQueue[queueIndex].node[l-1].f;
								}
								naviComp.naviQueue[queueIndex].node[naviComp.naviQueue[queueIndex].tempID].x = nextNode.x;
								naviComp.naviQueue[queueIndex].node[naviComp.naviQueue[queueIndex].tempID].y = nextNode.y;
								naviComp.naviQueue[queueIndex].node[naviComp.naviQueue[queueIndex].tempID].g = nextNode.g;
								naviComp.naviQueue[queueIndex].node[naviComp.naviQueue[queueIndex].tempID].f = nextNode.f;
							
								naviComp.naviQueue[queueIndex].queueSize++;
							}
						}											
					}
				}
			}
		}
	}
	
	/* By this time, we either have a path, or there is no path. If there is a path,
	we backtrack from the goal node to the node before the start node, then update
	results. What can we do to avoid the need for this final while loop? */
	
	if(pathFound == 1)
	{
		x = stopX;
		y = stopY;

		while(!(x == startNode.x && y == startNode.y))
		{
			j = naviComp.directionHistory[(x * YSIZE) + y];
			nextX = x;
			nextY = y;
			x = x + xAxis[j];
			y = y + yAxis[j];
		}

		results[0] = nextX;
		results[1] = nextY;
	}
}

/* Lastly, this main function simply runs through every possible path that
neither starts nor ends on an impassable grid square. Note that it also ignores
any path where the start node is the same as the goal node. */

int main()
{
	int startX;
	int startY;
	int goalX;
	int goalY;

	unsigned char results[2];

	int counter = 0;
	int secondCounter = 0;

	for(int i = 0; i < XSIZE; i++)
	{
		for(int j = 0; j < YSIZE; j++)
		{
			for(int k = 0; k < XSIZE; k++)
			{
				for(int l = 0; l < YSIZE; l++)
				{
					startX = i;
					startY = j;
					goalX = k;
					goalY = l;
					results[0] = 255;
					results[1] = 255;

					if(startX != goalX || startY != goalY)
					{
						if(mapData[(startX*YSIZE)+startY] != 0 && mapData[(goalX*YSIZE) + goalY] != 0)
						{
							secondCounter++;

							calculateAStarPath(startX, startY, goalX, goalY, &results[0]);

							if(results[0] != 255 && results[1] != 255)
							{
								counter++;

//								cout << "(" << (int)results[0] << "," << (int)results[1] << ")" << "\n";	// A little line of debug code
							}
						}
					}
				}
			}
		}
	}

	cout << counter << " Agents out of "<< secondCounter << " found a path home!" << "\n";

	int wayout;
	
	cin >> wayout;


	return 0;
}

/* Note that this implementation is not optimised, nor is it universally applicable. Its
purpose is solely demonstrative, to prompt you to look further into node navigation
yourselves! */