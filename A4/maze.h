#include <stdbool.h>

// Toggle debug printing (0 to enable, non zero to disable)
#define DEBUG 1

// Shortcuts for accessing heap children and parent
#define LCHILD(x) 2 * x + 1
#define RCHILD(x) 2 * x + 2
#define PARENT(x) (x - 1) / 2

// Min heaps for our open and closed lists
struct HEAP{
    int size;
    struct TILE* nodes;
} HEAP;

/*
 * Basic path struct for A*
 */
struct path {
    struct position* points;
    int numPoints;
    int currPoint;
} path;

/*
 * Basic struct for tracking 2d positions
 */
struct position {
    int x;
    int y;
};

// Individual tile nodes in the graph
struct TILE{
    // Check whether this tile can be traversed
    bool traversable;
    // Flag if tile has been fully processed
    bool isClosed;

    // Tile costs
    int g; // Cost to get to this tile so far from start
    int h; // Heuristic cost (distance to goal)
    int f; // Total cost (g + h)

    // Position of the current tile
    struct position pos;
    // Position of the previous tile
    struct position prev;
} TILE;

/*
 * Direction a hallway will be connected in (E.g. N_S is north to south)
 */
enum hallway_direction{N_S, E_W, S_N, W_E};

/*
 * Travel state for mobs
 */
enum direction{NORTH, SOUTH, EAST, WEST};

/*
 * Active state of the mob
 */
enum active_status{IDLE, ROAMING, PURSUING, ATTACKING, STUCK};

/*
 * Struct for storing basic entity data
 */
struct mob {
    // Position of the mob on the world map
    struct position location;
    // Position the mob is moving towards
    struct position next_location;
    // Path the mob will follow (A*)
    struct path* my_path;
    // Direction the mob is currently looking (Start north)
    int facing;
    // Track if mob is active(alive) or inactive(dead)
    bool is_active;
    // Track if mob is currently moving
    bool is_moving;
    // Track if the mob can currently be seen by the player
    bool is_visible;
    // Track if the mob has been spotted (aggro)
    bool is_aggro;
    // Track if mob should take its turn
    bool my_turn;
    // Track current mob state
    int state;
    // Mob's symbol to draw onto the entity array
    char symbol; 
    // World coordinates for the mob
    float worldX;
    float worldY;
    float worldZ;
    // World coordinates for the next travel point
    float destX;
    float destY;
    float destZ;
    // World rotation for the mob
    float rotX;
    float rotY;
    float rotZ;
};

/*
 * Struct for storing individual room related data
 */
struct room {
    // Floor position for the top left corner of the room
    struct position origin;
    // Floor position of the corner opposite the origin for the room
    struct position corner;
    // Chunk/cell position of this room (ranges from (0,0) to (2,2))
    struct position cellpos;
    // Floor position for the doors in this room
    struct position northDoor;
    struct position southDoor;
    struct position eastDoor;
    struct position westDoor;
    // Track if a hallway connection needs to be made
    bool connectNorth;
    bool connectSouth;
    bool connectEast;
    bool connectWest;

    // Room dimensions
    int roomWidth;
    int roomHeight;
    int ceilHeight; // For the engine, how 'tall' this room is

    // Room found
    bool roomVisible;
};

/*
 * Struct that contains floor data
 */
struct floor {
    // Track whether this is the outdoor level (Different render params)
    bool isOutdoors;
    // Track how many mobs are on this floor (used for mob list allocation)
    int mobCount;
    // floor size
    int floorWidth;
    int floorHeight;
    // 3D Stair position for outdoor level
    int sx, sy, sz;
    // 3D position for player controller
    int px, py, pz;

    // 2D Float array holding heightmap data for the outside level
    float** heightMap;
    // 'Cell' dividers (room zones)
    int hd1, hd2, vd1, vd2;
    // 2D Char array containing all data for this floorplan (geometry)
    char** floorData;
    // 2D Char array containing all entity data for this floor (player position, enemy positions, loot locations, etc)
    char** floorEntities;
    // 2D Int array which indicates if a given tile has been 'discovered' and can draw in a Fog of War map
    int** isVisible;
    // 1D Mob list containing references to all mobs on a given floor.
    struct mob* mobs;
    // 2D Struct array containing room data for this floor
    struct room** rooms;
};

/*
 * Struct containing an array of floors (World)
 */

struct floor_stack {
    // Current floor (index)
    int currentFloor;
    // Maximum amount of floors allocated so far (Array size)
    int maxFloors;
    // Array of floors
    struct floor** floors;
};

/*
 * Entry point function for maze generation. 
 * Creates and returns a character array representing one level
 * Bound by the width/height passed
 * Also hard coded to generate a 3x3 grid of rooms (varying size)
 */
struct floor* initMaze(int floorWidth, int floorHeight, bool isOutdoors);

/*
 * Generate the floor data (rooms + corridors)
 */
void genMaze(struct floor* maze);

/*
 * Generate an outdoor area (Top floor only)
 */
void genOutdoors(struct floor* maze);

/*
 * Generate cell borders
 */

void genCells(struct floor* maze);

/*
 * Generate a room in a given (x,y) cell
 */
void genRoom(struct floor* maze, int x, int y);

/*
 * Generate all needed doors for a specific room
 */
struct room genDoors(struct floor* maze, struct room r);

/*
 * Generate corridors for all rooms (Driver function)
 * Actual connections are made with call to connectDoors
 */
void genCorridors(struct floor* maze);

/*
 * Connect the doors with a hall from d1 to d2 using direction specified
 */
void connectDoors(struct floor* maze, struct position d1, struct position d2, int dir);

/*
 * Check over the entire floor and make sure all hallways are fully enclosed
 * (Done seperately after halls generated in event of overlapping halls blocking each other)
 */
void wallOffHalls(struct floor* maze);

/*
 * Populate the rooms (set player position, spawn mobs, drop loot, add decor, etc)
 */
void populateFloor(struct floor* maze);

/*
 * Print the floor data to the console
 */
void printMaze(struct floor* maze);

/*
 * Free the maze structure and all internal data
 */
void freeMaze(struct floor* maze);

/*
 * Utility function: returns a random integer within the bounds between low and high
 */
int randRange(int low, int high);

/*
 * Utility function for getting ceiling height at an x y coordinate.
 * Will return default height 2 when in a corridor, 0 when pointing to empty space
 * and the room height otherwise
 */
int getCeilHeight(struct floor* maze, int x, int y);

/*
 * Utility function for detail placement.  Checks if a specified tile in the world
 * is adjacent to a door tile ('/') to ensure doors are  not 'cut off'
 */
bool isBlockingDoor(struct floor* maze, int x, int y);

/*
 * Draw a character onto the maze at a given position
 */
void charDraw(struct floor* maze, struct position p, char toDraw);

/*
 * Draws a straight line between 2 positions
 */
void lineDraw(struct floor* maze, struct position p1, struct position p2, char toDraw);

/*
 * Draw a hallway between 2 points
 */
//void hallDraw(struct floor* maze, struct position p1, struct position p2);

/*
 * Draws a rectangle bounded by the 2 positions (top left and bottom right)
 */
void rectDraw(struct floor* maze, struct position p1, struct position p2, char toDraw);

/*
 * Fills in a rectangle bounded by the 2 positions (top left and bottom right) with a checker or line pattern made of 2 chars
 */
void rectPatternFill(struct floor* maze, struct position p1, struct position p2, char c1, char c2);

/***********************************\
 * A STAR IMPLEMENTATION FUNCTIONS *
\***********************************/

/*
 * MinHeap Implementation is referenced from:
 * https://robin-thomas.github.io/min-heap/
 */
// Initializes the AStar map and returns it
struct TILE** initTileMap(struct floor* f);

// Generate successors for a given tile
void genSuccessors(struct HEAP* heap, struct TILE** tileMap, struct TILE* origin);

// Get a path from start to finish
struct path* aStar(struct floor* f, struct position start, struct position end);

// Initialize minHeap
struct HEAP initHeap();

// Sorted insertion into minHeap
void insertTile(struct HEAP* heap, struct TILE* toAdd);

// Swap to nodes in the heap
void swap(struct TILE* a, struct TILE* b);

// Recursively sort out the heap
void heapify(struct HEAP* heap, int i);

// Pop the head off the heap
struct TILE* pop(struct HEAP* heap);

// Clear memory
void delHeap(struct HEAP* h);

// Check list for a node with a given position and compare against f score.
// If it exists and f score is lower than provided, returns true otherwise false.
bool skipPos(struct HEAP* heap, struct position p, int f);

// Check if a position is within the floor bounds
bool posValid(struct floor* f, struct position p);

// Check if 2 positions are equal
bool posMatch(struct position a, struct position b);

// Hueristic calculation to get distance to goal
int hueristic(struct position p, struct position goal);

// Build a path list from start to finish
struct path* buildPath(struct TILE** tileMap, struct TILE* start, struct TILE* end);

/*
 * Returns true if a given position at the specified floor has no entities or obstructions
 */
bool positionClear(struct floor* maze, struct position p);

/*
 * Given a floor position, return the room coordinate [0,0] through [2,2] or [-1,-1] if its not in a room
 */
struct position getRoomAtPosition(struct floor* maze, struct position p);
/*
 * Given a position, find a random position in the same room
 * If not in a room, return a random position from the floor
 */
struct position randPosInSameRoom(struct floor* maze, struct position p);
/*
 * Get a random valid position in the floor
 */
struct position randPosInFloor(struct floor* maze);