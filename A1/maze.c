#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "maze.h"

struct world* initMaze(int worldWidth, int worldHeight){
    struct world* toRet;
    int i;

    if(DEBUG==0)
        printf("Initializing maze...\n");

    // Allocate struct
    toRet = malloc(sizeof(struct world));
    if(toRet == NULL){
        fprintf(stderr, "ERROR: World struct allocation failure! Aborting!\n");
        return NULL;
    }

    if(DEBUG==0)
        printf("World struct allocated...\n");

    // Populate parameters
    toRet->worldWidth = worldWidth;
    toRet->worldHeight = worldHeight;

    // Allocate world data
    toRet->worldData = (char**)malloc(toRet->worldWidth * sizeof(char*));
    for(i = 0; i < toRet->worldWidth; i++){
        toRet->worldData[i] = (char*)malloc(toRet->worldHeight * sizeof(char));
        if(toRet->worldData[i] == NULL){
            fprintf(stderr, "ERROR: World data for column %d could not be allocated! Aborting!", i);
            // Attempt cleanup!
            while(i>=0){
                free(toRet->worldData[--i]);
            }
            free(toRet);
            return NULL;
        }
    }

    if(DEBUG==0)
        printf("World data allocated...\n");

    // Populate world data
    genMaze(toRet);

    // Job's done
    return toRet;
}

void genMaze(struct world* maze){
    int x, y;
    
    if(DEBUG==0)
        printf("Generating maze...\nSeeding number generator...\n");

    // Seed the random number generator
    srand(time(NULL));
    // First set entire maze to 'empty' space
    for(x = 0; x < maze->worldWidth; x++){
        for(y = 0; y < maze->worldHeight; y++){
            maze->worldData[x][y] = ' ';
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

    return;
}

void genCells(struct world* maze){
    int idealHorizontalCellSize; // Ideal horizontal size of a cell
    int idealVerticalCellSize; // Ideal horizontal size of a cell
    float cellPercentShift = 0.10; // Percent amount a cell border is allowed to be 'shifted' relative to world size
    int horizontalCellShift; // Amount of 'units' the cellShiftPercent translates to horizontally
    int verticalCellShift; // Amount of 'units' the cellShiftPercent translates to vertically
    int offset; // Calculated offset for a given cell division

    // Points for drawing with
    struct position p1;
    struct position p2;

    // Begin setting up cells (Parameters)
    if(DEBUG==0)
        printf("Setting cell size parameters...\n");
    idealHorizontalCellSize = (int)(maze->worldWidth/3);
    idealVerticalCellSize = (int)(maze->worldHeight/3);
    horizontalCellShift = (int)(maze->worldWidth*cellPercentShift);
    verticalCellShift = (int)(maze->worldHeight*cellPercentShift);

    // Setup initial (even) cell borders
    if(DEBUG==0)
        printf("Initializing base cell borders...\n");
    maze->vd1 = (int)(maze->worldWidth/3);
    maze->vd2 = (int)((maze->worldWidth/3)*2);
    maze->hd1 = (int)(maze->worldHeight/3);
    maze->hd2 = (int)((maze->worldHeight/3)*2);

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
    p2.y = (maze->worldHeight) - 1;
    lineDraw(maze, p1, p2, '%');

    p1.x = maze->hd2;
    p1.y = 0;
    p2.x = maze->hd2;
    p2.y = (maze->worldHeight) - 1;
    lineDraw(maze, p1, p2, '%');

    p1.x = 0;
    p1.y = maze->vd1;
    p2.x = (maze->worldWidth) - 1;
    p2.y = maze->vd1;
    lineDraw(maze, p1, p2, '%');

    p1.x = 0;
    p1.y = maze->vd2;
    p2.x = (maze->worldWidth) - 1;
    p2.y = maze->vd2;
    lineDraw(maze, p1, p2, '%');
    return;
}

void genRoom(struct world* maze, int x, int y){
    // Cell borders
    int leftBorder, rightBorder, topBorder, bottomBorder;

    if(DEBUG==0)
        printf("Setting cell borders for room (%d,%d)...\n", x, y);

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
            rightBorder = maze->worldWidth - 1;
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
            bottomBorder = maze->worldHeight;
            break;
        default:
            fprintf(stderr, "ERROR: Unknown room location (%d,%d)!\n", x, y);
            break;
    }

    

    return;
}

void printMaze(struct world* maze){
    int x, y;
    for(x = 0; x < maze->worldWidth; x++){
        for(y = 0; y < maze->worldHeight; y++){
            printf("%c", maze->worldData[x][y]);
        }
        printf("\n");
    }
    return;
}

void freeMaze(struct world* maze){
    int x;
    for(x = 0; x < maze->worldWidth; x++){
        free(maze->worldData[x]);
    }
    free(maze->worldData);
    free(maze);
    return;
}

void lineDraw(struct world* maze, struct position p1, struct position p2, char toDraw){
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

        for(i = lower; i < upper; i++){
            maze->worldData[p1.x][i] = toDraw;
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

        for(i = lower; i < upper; i++){
            maze->worldData[i][p1.y] = toDraw;
        }
    // Cannot draw a non-straight (up-down OR left-right) line
    } else {
        fprintf(stderr, "ERROR: Cannot draw a non-vertical/horizontal line! Unable to draw %c between (%d,%d) and (%d,%d)!\n", toDraw, p1.x, p1.y, p2.x, p2.y);
        return;
    }
    return;
}