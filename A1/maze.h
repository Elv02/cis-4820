#include <stdbool.h>

// Toggle debug printing (0 to enable, non zero to disable)
#define DEBUG 1

/*
 * Basic struct for tracking 2d positions
 */
struct position {
    int x;
    int y;
};

/*
 * Direction a hallway will be connected in (E.g. N_S is north to south)
 */
enum hallway_direction{N_S, E_W, S_N, W_E};

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
};

/*
 * Struct that contains floor data
 */
struct floor {
    // floor size
    int floorWidth;
    int floorHeight;

    // 'Cell' dividers (room zones)
    int hd1, hd2, vd1, vd2;
    // 2D Char array containing all data for this floorplan (geometry)
    char** floorData;
    // 2D Char array containing all entity data for this floor (player position, enemy positions, loot locations, etc)
    char** floorEntities;
    // 2D Struct array containing room data for this floor
    struct room** rooms;
};

/*
 * Entry point function for maze generation. 
 * Creates and returns a character array representing one level
 * Bound by the width/height passed
 * Also hard coded to generate a 3x3 grid of rooms (varying size)
 */
struct floor* initMaze(int floorWidth, int floorHeight);

/*
 * Generate the floor data (rooms + corridors)
 */
void genMaze(struct floor* maze);

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
