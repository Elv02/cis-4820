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
 * Struct that contains world data
 */
struct world {
    // World size
    int worldWidth;
    int worldHeight;

    // 'Cell' dividers (room zones)
    int hd1, hd2, vd1, vd2;
    char** worldData;
};

/*
 * Entry point function for maze generation. 
 * Creates and returns a character array representing one level
 * Bound by the width/height passed
 * Also hard coded to generate a 3x3 grid of rooms (varying size)
 */
struct world* initMaze(int worldWidth, int worldHeight);

/*
 * Generate the world data (rooms + corridors)
 */
void genMaze(struct world* maze);

/*
 * Generate cell borders
 */

void genCells(struct world* maze);

/*
 * Generate a room in a given (x,y) cell
 */
void genRoom(struct world* maze, int x, int y);

/*
 * Generate corridors between 2 specified rooms
 */
// TODO: ADD THIS

/*
 * Print the world data to the console
 */
void printMaze(struct world* maze);

/*
 * Free the maze structure and all internal data
 */
void freeMaze(struct world* maze);

/*
 * Draws a straight line between 2 positions
 */
void lineDraw(struct world* maze, struct position p1, struct position p2, char toDraw);
