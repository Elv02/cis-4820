// Toggle debug printing (0 to enable, non zero to disable)
#define DEBUG 0

/*
 * Basic struct for tracking 2d positions
 */
struct position {
    int x;
    int y;
};

/*
 * Struct for storing individual room related data
 */
struct room {
    // Floor position for the top left corner of the room
    struct position origin;
    // Floor position of the corner opposite the origin for the room
    struct position corner;
    // Chunk/cell position of this room (ranges for (0,0) to (2,2))
    struct position cellpos;
    // Floor position for the doors in this room
    struct position northDoor;
    struct position southDoor;
    struct position eastDoor;
    struct position westDoor;
    
    // Room dimensions
    int roomWidth;
    int roomHeight;
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
    // 2D Char array containing all data for this floor
    char** floorData;
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
void genDoors(struct floor* maze, struct room r);

/*
 * Generate corridors between 2 specified rooms
 */
// TODO: ADD THIS

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
 * Draw a character onto the maze at a given position
 */
void charDraw(struct floor* maze, struct position p, char toDraw);

/*
 * Draws a straight line between 2 positions
 */
void lineDraw(struct floor* maze, struct position p1, struct position p2, char toDraw);

/*
 * Draws a rectangle bounded by the 2 positions (top left and bottom right)
 */
void rectDraw(struct floor* maze, struct position p1, struct position p2, char toDraw);

/*
 * Fills in a rectangle bounded by the 2 positions (top left and bottom right) with a checker or line pattern made of 2 chars
 */
void rectPatternFill(struct floor* maze, struct position p1, struct position p2, char c1, char c2);
