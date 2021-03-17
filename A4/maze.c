#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>

#include "maze.h"
#include "perlin.h"

struct floor* initMaze(int floorWidth, int floorHeight, bool isOutdoors){
    struct floor* toRet;
    int i;

    if(DEBUG==0)
        printf("Initializing maze...\n");

    // Allocate struct
    toRet = malloc(sizeof(struct floor));
    if(toRet == NULL){
        fprintf(stderr, "ERROR: floor struct allocation failure! Aborting!\n");
        return NULL;
    }

    if(DEBUG==0)
        printf("floor struct allocated...\n");

    // Populate parameters
    toRet->floorWidth = floorWidth;
    toRet->floorHeight = floorHeight;
    toRet->mobCount = 0; // Start with 0 mobs

    // Allocate floor data
    toRet->floorData = (char**)malloc(toRet->floorWidth * sizeof(char*));
    toRet->floorEntities = (char**)malloc(toRet->floorWidth * sizeof(char*));
    toRet->heightMap = (float**)malloc(toRet->floorWidth * sizeof(float*));
    toRet->isVisible = (int**)malloc(toRet->floorWidth * sizeof(int*));

    // Sanity check
    if(toRet->floorData==NULL){
        fprintf(stderr, "ERROR: Could not allocate base for floorData array! Aborting...\n");
        free(toRet);
        return NULL;
    }

    // Init heightmap if outdoor level
    if(isOutdoors){
        for(i = 0; i < toRet->floorWidth; i++){
            toRet->heightMap[i] = (float*)malloc(toRet->floorHeight * sizeof(float));
            if(toRet->heightMap[i] == NULL){
                fprintf(stderr, "ERROR: Height map for column %d could not be allocated! Aborting!", i);
                // Attempt cleanup!
                while(i>=0){
                    free(toRet->heightMap[--i]);
                }
                free(toRet->heightMap);
                free(toRet);
                return NULL;
            }
        }
        genOutdoors(toRet);
        return toRet;
    }

    // Init floorplan
    for(i = 0; i < toRet->floorWidth; i++){
        toRet->floorData[i] = (char*)malloc(toRet->floorHeight * sizeof(char));
        if(toRet->floorData[i] == NULL){
            fprintf(stderr, "ERROR: floor data for column %d could not be allocated! Aborting!", i);
            // Attempt cleanup!
            while(i>=0){
                free(toRet->floorData[--i]);
            }
            free(toRet->floorData);
            free(toRet);
            return NULL;
        }
    }

    // Init entity array
    for(i = 0; i < toRet->floorWidth; i++){
        toRet->floorEntities[i] = (char*)malloc(toRet->floorHeight * sizeof(char));
        if(toRet->floorEntities[i] == NULL){
            fprintf(stderr, "ERROR: floor data for column %d could not be allocated! Aborting!", i);
            // Attempt cleanup!
            while(i>=0){
                free(toRet->floorEntities[--i]);
            }
            free(toRet->floorEntities);
            free(toRet);
            return NULL;
        }
    }

    // Init visible array
    for(i = 0; i < toRet->floorWidth; i++){
        toRet->isVisible[i] = (int*)malloc(toRet->floorHeight * sizeof(int));
        if(toRet->isVisible[i] == NULL){
            fprintf(stderr, "ERROR: floor data for column %d could not be allocated! Aborting!", i);
            // Attempt cleanup!
            while(i>=0){
                free(toRet->isVisible[--i]);
            }
            free(toRet->isVisible);
            free(toRet);
            return NULL;
        }
    }

    if(DEBUG==0)
        printf("floor data allocated...\n");

    // Allocate room data
    toRet->rooms = (struct room**)malloc(3 * sizeof(struct room*));
    for(i = 0; i < 3; i++){
        toRet->rooms[i] = (struct room*)malloc(3 * sizeof(struct room));
        if(toRet->rooms[i] == NULL){
            fprintf(stderr, "ERROR: Could not allocate room data for room column %d! Aborting!\n", i);
            // Attempt cleanup!
            while(i>=0){
                free(toRet->rooms[--i]);
            }
            free(toRet->rooms);
            for(i = 0; i < toRet->floorWidth; i++){
                free(toRet->floorData[i]);
            }
            free(toRet->floorData);
            free(toRet);
            return NULL;
        }
    }

    
    // Populate floor data
    genMaze(toRet);
    

    // Job's done
    return toRet;
}

void genMaze(struct floor* maze){
    int x, y;
    
    if(DEBUG==0)
        printf("Generating maze...\nSeeding number generator...\n");

    // Indicate we are generating an underground floor
    maze->isOutdoors = false;
    
    // Seed the random number generator
    srand(time(NULL));
    // First set entire maze to 'empty' space
    for(x = 0; x < maze->floorWidth; x++){
        for(y = 0; y < maze->floorHeight; y++){
            maze->floorData[x][y] = ' ';
            maze->floorEntities[x][y] = ' ';
            maze->isVisible[x][y] = 0; // All tiles start off as not visible
        }
    }

    // Generate cell borders
    if(DEBUG==0)
        printf("Setting up cell borders...\n");
    genCells(maze);

    // Generate rooms
    for(y = 0; y < 3; y++){
        for(x = 0; x < 3; x++){
            if(DEBUG==0)
                printf("Generating room (%d,%d)...\n", x, y);
            genRoom(maze, x, y);
        }
    }

    // Connect all rooms
    genCorridors(maze);
    
    // Enclose all corridors with walls
    wallOffHalls(maze);

    // Populate the rooms (set player position, spawn mobs, drop loot, add decor, etc)
    populateFloor(maze);

    return;
}

void genOutdoors(struct floor* maze){
    if(DEBUG==0)
        printf("Generating outdoor world...\n");

    int x, y;

    // Seed the random number generator
    srand(time(NULL));
    // Set that this is an outdoor map
    maze->isOutdoors = true;
    // Flag stairs as not placed
    maze->sx = -1;
    // Seed the perlin noise generator
    SEED = randRange(0, 4096);
    
    // Begin iterating over the heightmap
    for(y = 0; y < maze->floorHeight; y++){
        for(x = 0; x < maze->floorWidth; x++){
            // Get a float value for this coordinate
            maze->heightMap[x][y] = perlin2d((float)x, (float)y, 0.1, 1);
        }
    }

    return;
}

void genCells(struct floor* maze){
    int idealHorizontalCellSize; // Ideal horizontal size of a cell
    int idealVerticalCellSize; // Ideal horizontal size of a cell
    float cellPercentShift = 0.10; // Percent amount a cell border is allowed to be 'shifted' relative to floor size
    int horizontalCellShift; // Amount of 'units' the cellShiftPercent translates to horizontally
    int verticalCellShift; // Amount of 'units' the cellShiftPercent translates to vertically
    int offset; // Calculated offset for a given cell division

    // Points for drawing with
    struct position p1;
    struct position p2;

    // Begin setting up cells (Parameters)
    if(DEBUG==0)
        printf("Setting cell size parameters...\n");
    idealHorizontalCellSize = (int)(maze->floorWidth/3);
    idealVerticalCellSize = (int)(maze->floorHeight/3);
    horizontalCellShift = (int)(maze->floorWidth*cellPercentShift);
    verticalCellShift = (int)(maze->floorHeight*cellPercentShift);

    // Setup initial (even) cell borders
    if(DEBUG==0)
        printf("Initializing base cell borders...\n");
    maze->vd1 = (int)(maze->floorWidth/3);
    maze->vd2 = (int)((maze->floorWidth/3)*2);
    maze->hd1 = (int)(maze->floorHeight/3);
    maze->hd2 = (int)((maze->floorHeight/3)*2);

    // Offset even borders to make things more interesting
    if(DEBUG==0)
        printf("Adjusting cell borders...\n");
    offset = rand()%(verticalCellShift*2);
    offset -= verticalCellShift;
    maze->vd1 += offset;
    offset = rand()%(verticalCellShift*2);
    offset -= verticalCellShift;
    maze->vd2 += offset;
    offset = rand()%(horizontalCellShift*2);
    offset -= horizontalCellShift;
    maze->hd1 += offset;
    offset = rand()%(horizontalCellShift*2);
    offset -= horizontalCellShift;
    maze->hd2 += offset;
    
    // Draw horizontal dividers onto the maze
    if(DEBUG==0)
        printf("Writing cell borders to maze...\n");
    p1.x = maze->hd1;
    p1.y = 0;
    p2.x = maze->hd1;
    p2.y = (maze->floorHeight) - 1;
    // lineDraw(maze, p1, p2, '%');

    p1.x = maze->hd2;
    p1.y = 0;
    p2.x = maze->hd2;
    p2.y = (maze->floorHeight) - 1;
    // lineDraw(maze, p1, p2, '%');

    p1.x = 0;
    p1.y = maze->vd1;
    p2.x = (maze->floorWidth) - 1;
    p2.y = maze->vd1;
    // lineDraw(maze, p1, p2, '%');

    p1.x = 0;
    p1.y = maze->vd2;
    p2.x = (maze->floorWidth) - 1;
    p2.y = maze->vd2;
    // lineDraw(maze, p1, p2, '%');
    return;
}

void genRoom(struct floor* maze, int x, int y){
    // Cell borders
    int leftBorder, rightBorder, topBorder, bottomBorder;
    int cellWidth, cellHeight; // Height and width of the current cell

    // Room data to be added to the maze
    struct room toAdd;

    // Set room chunk position
    toAdd.cellpos.x = x;
    toAdd.cellpos.y = y;

    if(DEBUG==0)
        printf("Setting cell borders for room...\n");

    // Get left/right borders based on x location
    switch(x){
        case 0:
            leftBorder = 0;
            rightBorder = maze->hd1;
            break;
        case 1:
            leftBorder = maze->hd1;
            rightBorder = maze->hd2;
            break;
        case 2:
            leftBorder = maze->hd2;
            rightBorder = maze->floorWidth - 1;
            break;
        default:
            fprintf(stderr, "ERROR: Unknown room location (%d,%d)!\n", x, y);
            break;
    }

    // Get top/bottom borders based on y location 
    switch(y){
        case 0:
            topBorder = 0;
            bottomBorder = maze->vd1;
            break;
        case 1:
            topBorder = maze->vd1;
            bottomBorder = maze->vd2;
            break;
        case 2:
            topBorder = maze->vd2;
            bottomBorder = maze->floorHeight;
            break;
        default:
            fprintf(stderr, "ERROR: Unknown room location (%d,%d)!\n", x, y);
            break;
    }

    // Calculate cell size/dimensions
    cellWidth = rightBorder - leftBorder;
    cellHeight = bottomBorder - topBorder;

    // Pick a location for the top left corner of the room within the cell
    toAdd.origin.x = randRange(leftBorder + 2, rightBorder - (cellWidth/2));
    toAdd.origin.y = randRange(topBorder + 2, bottomBorder - (cellHeight/2));

    // Pick a location for the opposite corner
    toAdd.corner.x = randRange(toAdd.origin.x + 5, rightBorder - 2);
    toAdd.corner.y = randRange(toAdd.origin.y + 5, bottomBorder - 2);

    // Fill in the floor
    rectPatternFill(maze, toAdd.origin, toAdd.corner, '.', ',');

    // Draw the walls/border of the room
    rectDraw(maze, toAdd.origin, toAdd.corner, '#');
    
    // Populate extra room parameters
    toAdd.roomHeight = toAdd.corner.y - toAdd.origin.y;
    toAdd.roomWidth = toAdd.corner.x - toAdd.origin.x;
    toAdd.ceilHeight = randRange(3, 10);

    // Lastly, generate the doors for the room
    toAdd = genDoors(maze, toAdd);

    // Add the room to the maze
    maze->rooms[x][y] = toAdd;

    return;
}

struct room genDoors(struct floor* maze, struct room r){

    if(r.cellpos.y != 0){
        r.northDoor.x = randRange(r.origin.x + 1, r.corner.x - 1);
        r.northDoor.y = r.origin.y;
        charDraw(maze, r.northDoor, '/');
        r.connectNorth = true;
    } else {
        r.northDoor.x = -1;
        r.northDoor.y = -1;
        r.connectNorth = false;
    }

    if(r.cellpos.y != 2){
        r.southDoor.x = randRange(r.origin.x + 1, r.corner.x - 1);
        r.southDoor.y = r.corner.y;
        charDraw(maze, r.southDoor, '/');
        r.connectSouth = true;
    } else {
        r.southDoor.x = -1;
        r.southDoor.y = -1;
        r.connectSouth = false;
    }

    if(r.cellpos.x != 0){
        r.westDoor.x = r.origin.x;
        r.westDoor.y = randRange(r.origin.y + 1, r.corner.y - 1);
        charDraw(maze, r.westDoor, '/');
        r.connectWest = true;
    } else {
        r.westDoor.x = -1;
        r.westDoor.y = -1;
        r.connectWest = false;
    }

    if(r.cellpos.x != 2){
        r.eastDoor.x = r.corner.x;
        r.eastDoor.y = randRange(r.origin.y + 1, r.corner.y - 1);
        charDraw(maze, r.eastDoor, '/');
        r.connectEast = true;
    } else {
        r.eastDoor.x = -1;
        r.eastDoor.y = -1;
        r.connectEast = false;
    }


    return r;
}

void genCorridors(struct floor* maze){
    int x, y;

    if(DEBUG==0)
        printf("Generating corridor pathways...\n");

    for(y = 0; y < 3; y++){
        for(x = 0; x < 3; x++){
            if(maze->rooms[x][y].connectNorth){
                connectDoors(maze, maze->rooms[x][y].northDoor, maze->rooms[x][y-1].southDoor, N_S);
                maze->rooms[x][y].connectNorth = false;
                maze->rooms[x][y-1].connectSouth = false;
            }
            if(maze->rooms[x][y].connectSouth){
                connectDoors(maze, maze->rooms[x][y].southDoor, maze->rooms[x][y+1].northDoor, S_N);
                maze->rooms[x][y].connectSouth = false;
                maze->rooms[x][y+1].connectNorth = false;
            }
            if(maze->rooms[x][y].connectEast){
                connectDoors(maze, maze->rooms[x][y].eastDoor, maze->rooms[x+1][y].westDoor, E_W);
                maze->rooms[x][y].connectEast = false;
                maze->rooms[x+1][y].connectWest = false;
            }
            if(maze->rooms[x][y].connectWest){
                connectDoors(maze, maze->rooms[x][y].westDoor, maze->rooms[x-1][y].eastDoor, W_E);
                maze->rooms[x][y].connectWest = false;
                maze->rooms[x-1][y].connectEast = false;
            }
        }
    }

    return;
}

void connectDoors(struct floor* maze, struct position d1, struct position d2, int dir){
    struct position start; // Start of hallway
    struct position stop;  // End of hallway
    struct position bend1; // First bend in hallway after start
    struct position bend2; // Second bend in hallway just before stop
    int splitLoc;          // Location between the 2 doors where the hallway 'splits'

    // Determine all points in the hallway
    switch(dir){
        case N_S:
            splitLoc = randRange(d2.y + 2, d1.y - 2);
            start.x = d1.x;
            start.y = d1.y + 1;
            stop.x = d2.x;
            stop.y = d2.y - 1;
            bend1.x = d1.x;
            bend1.y = splitLoc;
            bend2.x = d2.x;
            bend2.y = splitLoc;
            break;
        case S_N:
            splitLoc = randRange(d1.y + 2, d2.y - 2);
            start.x = d1.x;
            start.y = d1.y + 1;
            stop.x = d2.x;
            stop.y = d2.y - 1;
            bend1.x = d1.x;
            bend1.y = splitLoc;
            bend2.x = d2.x;
            bend2.y = splitLoc;
            break;
        case E_W:
            splitLoc = randRange(d1.x + 2, d2.x - 2);
            start.x = d1.x + 1;
            start.y = d1.y;
            stop.x = d2.x - 1;
            stop.y = d2.y;
            bend1.x = splitLoc;
            bend1.y = d1.y;
            bend2.x = splitLoc;
            bend2.y = d2.y;
            break;
        case W_E:
            splitLoc = randRange(d2.x + 2, d1.x - 2);
            start.x = d1.x - 1;
            start.y = d1.y;
            stop.x = d2.x + 1;
            stop.y = d2.y;
            bend1.x = splitLoc;
            bend1.y = d1.y;
            bend2.x = splitLoc;
            bend2.y = d2.y;
            break;
    }
    // Draw the connections to the world using the calculated points
    lineDraw(maze, start, bend1, '+');
    lineDraw(maze, bend1, bend2, '+');
    lineDraw(maze, bend2, stop, '+');
    return;
}

void wallOffHalls(struct floor* maze){
    int x, y, x1, y1;
    for(y = 0; y < maze->floorHeight; y++){
        for(x = 0; x < maze->floorWidth; x++){
            // Found a hallway!
            if(maze->floorData[x][y]=='+'){
                // Search surrounding spaces
                for(y1 = y - 1; y1 <= y + 1; y1++){
                    for(x1 = x - 1; x1 <= x + 1; x1++){
                        // Fill any empty spaces with a wall (prevents overwrite of already populated spaces (doors, other hall tiles, etc))
                        if(maze->floorData[x1][y1]==' ')
                            maze->floorData[x1][y1] = '#';       
                    }
                }
            }
        }
    }
    return;
}

void populateFloor(struct floor* maze){
    struct position pen; // Point for writing to the maze
    int x, y;

    // Pick 2 random rooms for up and down staircases
    while(true){
        x = randRange(0, 2);
        y = randRange(0, 2);
        pen.x = randRange(maze->rooms[x][y].origin.x + 2, maze->rooms[x][y].corner.x - 2);
        pen.y = randRange(maze->rooms[x][y].origin.y + 2, maze->rooms[x][y].corner.y - 2);
        if(maze->floorEntities[pen.x][pen.y]==' ' && !isBlockingDoor(maze, pen.x, pen.y)){
            maze->floorEntities[pen.x][pen.y] = 'U'; // Upstairs
            maze->floorEntities[pen.x+1][pen.y] = '@'; // Player
            break;
        }
    }
    while(true){
        x = randRange(0, 2);
        y = randRange(0, 2);
        pen.x = randRange(maze->rooms[x][y].origin.x + 2, maze->rooms[x][y].corner.x - 2);
        pen.y = randRange(maze->rooms[x][y].origin.y + 2, maze->rooms[x][y].corner.y - 2);
        if(maze->floorEntities[pen.x][pen.y]==' ' && !isBlockingDoor(maze, pen.x, pen.y)){
            maze->floorEntities[pen.x][pen.y] = 'D'; // Downstairs
            break;
        }
    }

    // Spawn one mob in each room
    for(y = 0; y < 3; y++){
        for(x = 0; x < 3; x++){
            int mobType = randRange(0, 2);
            char toDraw = ' ';
            switch(mobType){
                case 0:
                    toDraw = 'C'; // Cactus
                    break;
                case 1:
                    toDraw = 'B'; // Bat 
                    break;
                case 2:
                    toDraw = 'F'; // Fish
                    break;
                default:
                    toDraw = 'E'; // Error
                    break;
            }
            while(true){
                pen.x = randRange(maze->rooms[x][y].origin.x + 1, maze->rooms[x][y].corner.x - 1);
                pen.y = randRange(maze->rooms[x][y].origin.y + 1, maze->rooms[x][y].corner.y - 1);
                if(maze->floorEntities[pen.x][pen.y]==' ' && !isBlockingDoor(maze, pen.x, pen.y)){
                    maze->floorEntities[pen.x][pen.y] = toDraw;
                    maze->mobCount++;
                    break;
                } 
            }
        }
    }

    // Allocate mob array now that we know amount of mobs for the floor
    maze->mobs = (struct mob*)malloc(maze->mobCount * sizeof(struct mob));

    // Toss some random boxes into each room
    for(y = 0; y < 3; y++){
        for(x = 0; x < 3; x++){
            int max = (int)floor(sqrt(maze->rooms[x][y].roomWidth * maze->rooms[x][y].roomHeight));
            int numBoxes = randRange(0, max);
            while(numBoxes>0){
                pen.x = randRange(maze->rooms[x][y].origin.x + 1, maze->rooms[x][y].corner.x - 1);
                pen.y = randRange(maze->rooms[x][y].origin.y + 1, maze->rooms[x][y].corner.y - 1);
                if(maze->floorEntities[pen.x][pen.y]==' ' && !isBlockingDoor(maze, pen.x, pen.y)){
                    maze->floorEntities[pen.x][pen.y] = '$';
                } 
                numBoxes--;
            }
        }
    }

    return;
}

void printMaze(struct floor* maze){
    int x, y;
    for(y = 0; y < maze->floorWidth; y++){
        for(x = 0; x < maze->floorHeight; x++){
            // If there is no entity in the current space print the level geometry
            if(maze->floorEntities[x][y]==' ')
                printf("%c", maze->floorData[x][y]);
            else
                printf("%c", maze->floorEntities[x][y]);
        }
        printf("\n");
    }
    return;
}

void freeMaze(struct floor* maze){
    int x;
    if(maze->isOutdoors){
        for(x = 0; x < maze->floorWidth; x++){
            free(maze->heightMap[x]);
        }
        free(maze->heightMap);
    }
    for(x = 0; x < maze->floorWidth; x++){
        free(maze->floorData[x]);
        free(maze->floorEntities[x]);
    }
    free(maze->floorData);
    free(maze->floorEntities);
    for(x = 0; x < 3; x++){
        free(maze->rooms[x]);
    }
    free(maze->rooms);
    free(maze);
    return;
}

int randRange(int low, int high){
    return (rand() % (high - low + 1) + low);
}

int getCeilHeight(struct floor* maze, int x, int y){
    int x1, y1;
    bool inRoom = false;
    struct room r;
    // First, sanity check
    if(x>=maze->floorWidth||y>=maze->floorHeight){
        return 0;
    // Check for the void
    } else if(maze->floorData[x][y]==' '){
        return 0;
    } else {
        // Check all rooms
        for(y1 = 0; y1 < 3; y1++){
            for(x1 = 0; x1 < 3; x1++){
                if(x>=maze->rooms[x1][y1].origin.x && y>=maze->rooms[x1][y1].origin.y
                    && x<=maze->rooms[x1][y1].corner.x && y<=maze->rooms[x1][y1].corner.y){
                        inRoom = true;
                        r = maze->rooms[x1][y1];
                        break;
                }
            }
            if(inRoom){
                break;
            }
        }
        if(inRoom){
            return r.ceilHeight;
        } else {
            return 3;
        }
    }
}

bool isBlockingDoor(struct floor* maze, int x, int y){
    int x1, y1;
    for(y1 = y - 1; y1 <= y + 1; y1++){
        for(x1 = x - 1; x1 <= x + 1; x1++){
            if(maze->floorData[x1][y1]=='/')
                return true;
        }
    }
    return false;
}

void charDraw(struct floor* maze, struct position p, char toDraw){
    //if(p.x<maze->floorWidth&&p.y<maze->floorHeight)
    maze->floorData[p.x][p.y] = toDraw;
    return;
}

void lineDraw(struct floor* maze, struct position p1, struct position p2, char toDraw){
    int upper, lower, i;

    if(DEBUG==0)
        printf("Drawing line (%d,%d) -> (%d,%d)...\n", p1.x, p1.y, p2.x, p2.y);

    // Draw a vertical line
    if(p1.x == p2.x){
        if(p1.y<p2.y){
            lower = p1.y;
            upper = p2.y;
        } else {
            lower = p2.y;
            upper = p1.y;
        }

        for(i = lower; i <= upper; i++){
            maze->floorData[p1.x][i] = toDraw;
        }
    // Draw a horizontal line
    } else if(p1.y == p2.y){
        if(p1.x<p2.x){
            lower = p1.x;
            upper = p2.x;
        } else {
            lower = p2.x;
            upper = p1.x;
        } 

        for(i = lower; i <= upper; i++){
            maze->floorData[i][p1.y] = toDraw;
        }
    // Cannot draw a non-straight (up-down OR left-right) line
    } else {
        fprintf(stderr, "ERROR: Cannot draw a non-vertical/horizontal line! Unable to draw %c between (%d,%d) and (%d,%d)!\n", toDraw, p1.x, p1.y, p2.x, p2.y);
        return;
    }
    return;
}

void rectDraw(struct floor* maze, struct position p1, struct position p2, char toDraw){
    /*
     * p1---p3
     * |    |
     * p4---p2
     */
    struct position p3;
    struct position p4;

    // Setup the other 2 corners
    p3.x = p2.x;
    p3.y = p1.y;
    p4.x = p1.x;
    p4.y = p2.y;

    // Draw the lines
    lineDraw(maze, p1, p3, toDraw);
    lineDraw(maze, p1, p4, toDraw);
    lineDraw(maze, p2, p4, toDraw);
    lineDraw(maze, p2, p3, toDraw);

    return;
}

void rectPatternFill(struct floor* maze, struct position p1, struct position p2, char c1, char c2){
    struct position pen; // Position we are writing at
    int alt = 0; // Alternation tracker (write char 1 when it's 0, and char 2 when it's 1)

    for(pen.x = p1.x; pen.x < p2.x; pen.x++){
        for(pen.y = p1.y; pen.y < p2.y; pen.y++){
            if(alt==0){
                charDraw(maze, pen, c1);
                alt = 1;
            } else {
                charDraw(maze, pen, c2);
                alt = 0;
            }
        }
    }

    return;
}

/*************************\
 * A STAR IMPLEMENTATION *
\*************************/

struct TILE** initTileMap(struct floor* f){
    struct TILE** tileMap = malloc(sizeof(struct TILE*) * f->floorHeight);
    int i;
    for(i = 0; i < f->floorHeight; i++){
        tileMap[i] = malloc(sizeof(struct TILE) * f->floorWidth);
    }

    int x, y;
    for(y = 0; y < f->floorHeight; y++){
        for(x = 0; x < f->floorWidth; x++){
            tileMap[x][y].f = __INT_MAX__;
            tileMap[x][y].g = __INT_MAX__;
            tileMap[x][y].h = __INT_MAX__;
            struct position p;
            p.x = x;
            p.y = y;
            if(positionClear(f, p)) tileMap[x][y].traversable = true;
            else tileMap[x][y].traversable = false;
            tileMap[x][y].isClosed = false;
            tileMap[x][y].prev.x = -1;
            tileMap[x][y].prev.y = -1;
        }
    }

    return tileMap;
}

void genSuccessors(struct HEAP* heap, struct TILE** tileMap, struct TILE* origin){
    if(origin == NULL || heap == NULL) return;

    struct TILE* north = malloc(sizeof(struct TILE));
    north->pos.x = origin->pos.x;
    north->pos.y = origin->pos.y - 1;
    north->f = __INT_MAX__;
    north->prev.x = origin->pos.x;
    north->prev.y = origin->pos.y;
    north->traversable = tileMap[north->pos.x][north->pos.y].traversable;
    insertTile(heap, north);

    struct TILE* south = malloc(sizeof(struct TILE));
    south->pos.x = origin->pos.x;
    south->pos.y = origin->pos.y + 1;
    south->f = __INT_MAX__;
    south->prev.x = origin->pos.x;
    south->prev.y = origin->pos.y;
    south->traversable = tileMap[south->pos.x][south->pos.y].traversable;
    insertTile(heap, south);

    struct TILE* east = malloc(sizeof(struct TILE));
    east->pos.x = origin->pos.x + 1;
    east->pos.y = origin->pos.y;
    east->f = __INT_MAX__;
    east->prev.x = origin->pos.x;
    east->prev.y = origin->pos.y;
    east->traversable = tileMap[east->pos.x][east->pos.y].traversable;
    insertTile(heap, east);

    struct TILE* west = malloc(sizeof(struct TILE));
    west->pos.x = origin->pos.x - 1;
    west->pos.y = origin->pos.y;
    west->f = __INT_MAX__;
    west->prev.x = origin->pos.x;
    west->prev.y = origin->pos.y;
    west->traversable = tileMap[west->pos.x][west->pos.y].traversable;
    insertTile(heap, west);
    
    return;
}

struct path* aStar(struct floor* f, struct position start, struct position end){
    // Step 0 - Sanity checks
    if(f == NULL){
        fprintf(stderr, "ERROR: Floor reference provided for A Star is NULL!\n");
    }
    if(start.x < 0 || start.y < 0 || start.x > f->floorWidth || start.y > f->floorHeight){
        fprintf(stderr, "ERROR: Start position (%d, %d) is outside floor boundaries!\n", start.x, start.y);
    }
    if(end.x < 0 || end.y < 0 || end.x > f->floorWidth || end.y > f->floorHeight){
        fprintf(stderr, "ERROR: End position (%d, %d) is outside floor boundaries!\n", end.x, end.y);
    }

    // Step 0A - Initialize the Tilemap
    struct TILE** tileMap = initTileMap(f);
    // Step 1 - Initialize the OPEN list
    struct HEAP open_list = initHeap();
    // Step 2A - Setup and add starting tile then add to open list
    tileMap[start.x][start.y].h = hueristic(start, end);
    tileMap[start.x][start.y].g = 0;
    tileMap[start.x][start.y].f = tileMap[start.x][start.y].h;
    tileMap[start.x][start.y].pos.x = start.x;
    tileMap[start.x][start.y].pos.y = start.y;
    tileMap[start.x][start.y].prev.x = -1; // Signal end of path
    tileMap[start.x][start.y].prev.y = -1;
    tileMap[start.x][start.y].traversable = true;
    insertTile(&open_list, &tileMap[start.x][start.y]);

    // Step 3 - While the OPEN list is not empty
    while(open_list.size > 0){
        // Step 3A - Pop lowest cost node off open list (q)
        struct TILE* q = pop(&open_list);
        // Step 3B - Initialize the SUCCESSOR list and set parent to q
        struct HEAP successors = initHeap();
        genSuccessors(&successors, tileMap, q);
        // Step 3C - Iterate over each successor
        while(successors.size > 0){
            struct TILE* s = pop(&successors);
            // Step 3C(i) - If successor is goal, stop search
            if(posMatch(end, s->pos)){
                // Goal reached!
                struct path* toRet;
                toRet = buildPath(tileMap, &tileMap[start.x][start.y], s);
                // CLEANUP
                int y;
                for(y = 0; y < f->floorHeight; y++){
                    free(tileMap[y]);
                }
                free(tileMap);
                return toRet;
            }
            // Step 3C(ii) - Check if tile is traversable or has been closed
            if(!s->traversable || tileMap[s->pos.x][s->pos.y].isClosed){
                free(s);
                continue;
            }
            // Update scores
            s->g = q->g + 1;
            s->h = hueristic(s->pos, end);
            s->f = s->g + s->h;
            // Check if the new f score is better than what's registered on the tilemap
            if(tileMap[s->pos.x][s->pos.y].f <= s->f){
                free(s);
                continue; // Skip the node
            } else {
                // Update the TileMap to include the new node data and add it to the open list
                tileMap[s->pos.x][s->pos.y].f = s->f;
                tileMap[s->pos.x][s->pos.y].g = s->g;
                tileMap[s->pos.x][s->pos.y].h = s->h;
                tileMap[s->pos.x][s->pos.y].traversable = s->traversable;
                tileMap[s->pos.x][s->pos.y].pos.x = s->pos.x;
                tileMap[s->pos.x][s->pos.y].pos.y = s->pos.y;
                tileMap[s->pos.x][s->pos.y].prev = s->prev;
                insertTile(&open_list, &tileMap[s->pos.x][s->pos.y]);
                free(s);
            }
        }
        // Step3D - Flag this tile as closed
        tileMap[q->pos.x][q->pos.y].isClosed = true;
    }
    return NULL; // ERROR, no path found
}

// Build a path list from start to finish
struct path* buildPath(struct TILE** tileMap, struct TILE* start, struct TILE* end){
    // Setup path
    struct path* toRet = malloc(sizeof(struct path));
    toRet->currPoint = 1; // Skip start
    // Get number of points
    int size = 1;
    struct TILE* t = end;
    struct position nextPos;
    nextPos.x = t->prev.x;
    nextPos.y = t->prev.y;
    while(nextPos.x != -1){
        // Loadup tile at nextpos
        t = &tileMap[nextPos.x][nextPos.y];
        nextPos.x = t->prev.x;
        nextPos.y = t->prev.y;
        size++;
    }
    toRet->numPoints = size;
    // Allocate points
    toRet->points = malloc(sizeof(struct position) * size);
    // Build path in reverse
    int i = 0;
    t = end;
    for(i = size - 1; i >= 0; i--){
        toRet->points[i].x = t->pos.x;
        toRet->points[i].y = t->pos.y;
        t = &tileMap[t->prev.x][t->prev.y];
    }
    return toRet;
}

// Initialize minHeap
struct HEAP initHeap(){
    struct HEAP h;
    h.size = 0;
    return h;
}

// Sorted insertion into minHeap
void insertTile(struct HEAP* heap, struct TILE* data){
    // Sanity check
    if(heap->size > 0){
        heap->nodes = realloc(heap->nodes, (heap->size + 1) * sizeof(struct TILE));
    } else {
        heap->nodes = malloc(sizeof(struct TILE));
    }
    // Init data
    struct TILE* toAdd = malloc(sizeof(struct TILE));
    toAdd->f = data->f;
    toAdd->g = data->g;
    toAdd->h = data->h;
    toAdd->pos.x = data->pos.x;
    toAdd->pos.y = data->pos.y;
    toAdd->prev.x = data->prev.x;
    toAdd->prev.y = data->prev.y;
    toAdd->traversable = data->traversable;

    // Insert to the right
    int i = (heap->size)++;
    while(i > 0 && toAdd->f < heap->nodes[PARENT(i)].f){
        heap->nodes[i] = heap->nodes[PARENT(i)];
        i = PARENT(i);
    }
    heap->nodes[i] = *toAdd;
    return;
}

// Swap two nodes in the heap
void swap(struct TILE* a, struct TILE* b){
    struct TILE t = *a;
    *a = *b;
    *b = t;
    return;
}

// Recursively sort out the heap
void heapify(struct HEAP* heap, int i){
    int smallest;
    if(LCHILD(i) < heap->size && heap->nodes[LCHILD(i)].f < heap->nodes[i].f){
        smallest = LCHILD(i);
    } else {
        smallest = i;
    }
    if(RCHILD(i) < heap->size && heap->nodes[RCHILD(i)].f < heap->nodes[smallest].f){
        smallest = RCHILD(i);
    }
    if(smallest != i){
        swap(&(heap->nodes[i]), &(heap->nodes[smallest]));
        heapify(heap, smallest);
    }
    return;
}

// Pop the head off the heap
struct TILE* pop(struct HEAP* heap){
    if(heap->size <= 0){
        fprintf(stderr, "ERROR: Cannot pop from an empty heap!!\n");
    } 
    struct TILE* toRet = malloc(sizeof(struct TILE));
    struct TILE toCopy = heap->nodes[0];
    toRet->f = toCopy.f;
    toRet->g = toCopy.g;
    toRet->h = toCopy.h;
    toRet->pos.x = toCopy.pos.x;
    toRet->pos.y = toCopy.pos.y;
    toRet->prev = toCopy.prev;
    toRet->traversable = toCopy.traversable;
    heap->nodes[0] = heap->nodes[--(heap->size)];
    heap->nodes = realloc(heap->nodes, heap->size * sizeof(struct TILE));
    heapify(heap, 0);
    return toRet;
}

// Clear memory
void delHeap(struct HEAP* h){
    if(h && h->nodes){
        free(h->nodes);
    } 
    return;
}

// Check list for a node with a given position and compare against f score.
// If it exists and f score is lower than provided, returns true otherwise false.
bool skipPos(struct HEAP* heap, struct position p, int f){
    int i;
    for(i = 0; i < heap->size; i++){
        if(posMatch(heap->nodes[i].pos, p) && heap->nodes[i].f <= f){
            return true;
        }
    }
    return false;
}

// Check if a position is within the floor bounds
bool posValid(struct floor* f, struct position p){
    if(p.x < 0 || p.y < 0 || p.x > f->floorWidth || p.y > f->floorHeight){
        return false;
    } 
    return true;
}

// Check if 2 positions are equal
bool posMatch(struct position a, struct position b){
    if(a.x == b.x && a.y == b.y) return true;
    return false;
}

// Hueristic calculation to get distance to goal
int hueristic(struct position p, struct position goal){
    int toRet;
    toRet = abs(p.x - goal.x) + abs(p.y - goal.y);
    return toRet;
}

bool positionClear(struct floor* maze, struct position p){
    char lvlCheck = maze->floorData[p.x][p.y];
    char entCheck = maze->floorEntities[p.x][p.y];
    switch(lvlCheck){
        case '#':
        case ' ': // The void
            return false;
        default:
            break;
    }
    switch(entCheck){
        case 'C':
        case 'B':
        case 'F':
        case '$':
        case 'U':
        case 'D':
            return false;
        default:
            break;
    }
    return true;
}

struct position getRoomAtPosition(struct floor* maze, struct position p){
    int x, y;
    struct position toRet;
    toRet.x = -1;
    toRet.y = -1;
    for(y = 0; y <= 2; y++){
        for(x = 0; x <= 2; x++){
            if(maze->rooms[x][y].origin.x < p.x && maze->rooms[x][y].origin.y < p.y && maze->rooms[x][y].corner.x > p.x && maze->rooms[x][y].corner.y > p.y){
                toRet.x = x;
                toRet.y = y;
            }
        }
    }
    return toRet;
}

struct position randPosInSameRoom(struct floor* maze, struct position p){
    struct position rmPos;
    rmPos = getRoomAtPosition(maze, p);
    struct position toRet;
    // If we're not in a room return a position in the floor
    if(rmPos.x == -1 && rmPos.y == -1){
        return randPosInFloor(maze);
    } else {
        while(true){
            toRet.x = randRange(maze->rooms[rmPos.x][rmPos.y].origin.x + 1, maze->rooms[rmPos.x][rmPos.y].corner.x - 1);
            toRet.y = randRange(maze->rooms[rmPos.x][rmPos.y].origin.y + 1, maze->rooms[rmPos.x][rmPos.y].corner.y - 1);
            if(positionClear(maze, toRet)){
                break;
            }
        }
    }
    return toRet;
}

struct position randPosInFloor(struct floor* maze){
    struct position toRet;
    while(true){
        toRet.x = randRange(1, maze->floorWidth - 1);
        toRet.y = randRange(1, maze->floorHeight - 1);
        if(positionClear(maze, toRet)){
            break;
        }
    }
    return toRet;
}