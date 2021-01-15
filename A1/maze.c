#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>

#include "maze.h"

struct floor* initMaze(int floorWidth, int floorHeight){
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

    // Allocate floor data
    toRet->floorData = (char**)malloc(toRet->floorWidth * sizeof(char*));

    // Sanity check
    if(toRet->floorData==NULL){
        fprintf(stderr, "ERROR: Could not allocate base for floorData array! Aborting...\n");
        free(toRet);
        return NULL;
    }

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

    // Seed the random number generator
    srand(time(NULL));
    // First set entire maze to 'empty' space
    for(x = 0; x < maze->floorWidth; x++){
        for(y = 0; y < maze->floorHeight; y++){
            maze->floorData[x][y] = ' ';
        }
    }

    // Generate cell borders
    if(DEBUG==0)
        printf("Setting up cell borders...\n");
    genCells(maze);

    // Generate rooms
    for(x = 0; x < 3; x++){
        for(y = 0; y < 3; y++){
            if(DEBUG==0)
                printf("Generating room (%d,%d)...\n", x, y);
            genRoom(maze, x, y);
        }
    }

    // Connect all rooms
    genCorridors(maze);

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
    lineDraw(maze, p1, p2, '%');

    p1.x = maze->hd2;
    p1.y = 0;
    p2.x = maze->hd2;
    p2.y = (maze->floorHeight) - 1;
    lineDraw(maze, p1, p2, '%');

    p1.x = 0;
    p1.y = maze->vd1;
    p2.x = (maze->floorWidth) - 1;
    p2.y = maze->vd1;
    lineDraw(maze, p1, p2, '%');

    p1.x = 0;
    p1.y = maze->vd2;
    p2.x = (maze->floorWidth) - 1;
    p2.y = maze->vd2;
    lineDraw(maze, p1, p2, '%');
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

    // Lastly, generate the doors for the room
    genDoors(maze, toAdd);

    // Add the room to the maze
    maze->rooms[x][y] = toAdd;

    return;
}

void genDoors(struct floor* maze, struct room r){

    if(r.cellpos.y != 0){
        r.northDoor.x = randRange(r.origin.x + 1, r.corner.x - 1);
        r.northDoor.y = r.origin.y;
        charDraw(maze, r.northDoor, '/');
    }

    if(r.cellpos.y != 2){
        r.southDoor.x = randRange(r.origin.x + 1, r.corner.x - 1);
        r.southDoor.y = r.corner.y;
        charDraw(maze, r.southDoor, '/');
    }

    if(r.cellpos.x != 0){
        r.eastDoor.x = r.origin.x;
        r.eastDoor.y = randRange(r.origin.y + 1, r.corner.y - 1);
        charDraw(maze, r.eastDoor, '/');
    }

    if(r.cellpos.x != 2){
        r.westDoor.x = r.corner.x;
        r.westDoor.y = randRange(r.origin.y + 1, r.corner.y - 1);
        charDraw(maze, r.westDoor, '/');
    }

    return;
}

void genCorridors(struct floor* maze){
    int splitLoc; // Location between the 2 rooms where the corridor 'bends/snakes'
    
    // Connect 0,0 to neighbours

    // Connect 2,0 to neighbours

    // Connect 1,1 to neighbours

    // Connect 0,2 to neighbours

    // Connect 2,2 to neighbours

    return;
}

void printMaze(struct floor* maze){
    int x, y;
    for(x = 0; x < maze->floorWidth; x++){
        for(y = 0; y < maze->floorHeight; y++){
            printf("%c", maze->floorData[x][y]);
        }
        printf("\n");
    }
    return;
}

void freeMaze(struct floor* maze){
    int x;
    for(x = 0; x < maze->floorWidth; x++){
        free(maze->floorData[x]);
    }
    free(maze->floorData);
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

void charDraw(struct floor* maze, struct position p, char toDraw){
    //if(p.x<maze->floorWidth&&p.y<maze->floorHeight)
    maze->floorData[p.x][p.y] = toDraw;
    return;
}

void lineDraw(struct floor* maze, struct position p1, struct position p2, char toDraw){
    int upper, lower, i;

    if(DEBUG==0)
        printf("Drawing line...\n");

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