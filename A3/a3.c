
/* Derived from scene.c in the The OpenGL Programming Guide */
/* Keyboard and mouse rotation taken from Swiftless Tutorials #23 Part 2 */
/* http://www.swiftless.com/tutorials/opengl/camera2.html */

/* Frames per second code taken from : */
/* http://www.lighthouse3d.com/opengl/glut/index.php?fps */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "graphics.h"
#include "maze.h"
#include "perlin.h"
#include "textures.h"

extern GLubyte  world[WORLDX][WORLDY][WORLDZ];

   /* Collection of floors for holding world data */
static struct floor_stack levelStack;

   /* x y coordinates offset for cloud moving */
static float xCloudOffset, yCloudOffset;
   /*  Draw height for clouds */
static int cloudHeight = 49; 
   /* Time since last game tick */
static int oldTime = 0;

	/* mouse function called by GLUT when a button is pressed or released */
void mouse(int, int, int, int);

	/* initialize graphics library */
extern void graphicsInit(int *, char **);

	/* lighting control */
extern void setLightPosition(GLfloat, GLfloat, GLfloat);
extern GLfloat* getLightPosition();

	/* viewpoint control */
extern void setViewPosition(float, float, float);
extern void getViewPosition(float *, float *, float *);
extern void getOldViewPosition(float *, float *, float *);
extern void setOldViewPosition(float, float, float);
extern void setViewOrientation(float, float, float);
extern void getViewOrientation(float *, float *, float *);

	/* add cube to display list so it will be drawn */
extern void addDisplayList(int, int, int);

	/* mob controls */
extern void createMob(int, float, float, float, float);
extern void setMobPosition(int, float, float, float, float);
extern void hideMob(int);
extern void showMob(int);

	/* player controls */
extern void createPlayer(int, float, float, float, float);
extern void setPlayerPosition(int, float, float, float, float);
extern void hidePlayer(int);
extern void showPlayer(int);

	/* tube controls */
extern void createTube(int, float, float, float, float, float, float, int);
extern void hideTube(int);
extern void showTube(int);

	/* 2D drawing functions */
extern void  draw2Dline(int, int, int, int, int);
extern void  draw2Dbox(int, int, int, int);
extern void  draw2Dtriangle(int, int, int, int, int, int);
extern void  set2Dcolour(float []);

	/* texture functions */
extern int setAssignedTexture(int, int);
extern void unsetAssignedTexture(int);
extern int getAssignedTexture(int);
extern void setTextureOffset(int, float, float);


	/* flag which is set to 1 when flying behaviour is desired */
extern int flycontrol;
	/* flag used to indicate that the test world should be used */
extern int testWorld;
	/* flag to print out frames per second */
extern int fps;
	/* flag to indicate the space bar has been pressed */
extern int space;
	/* flag indicates the program is a client when set = 1 */
extern int netClient;
	/* flag indicates the program is a server when set = 1 */
extern int netServer; 
	/* size of the window in pixels */
extern int screenWidth, screenHeight;
	/* flag indicates if map is to be printed */
extern int displayMap;
	/* flag indicates use of a fixed viewpoint */
extern int fixedVP;

	/* frustum corner coordinates, used for visibility determination  */
extern float corners[4][3];

	/* determine which cubes are visible e.g. in view frustum */
extern void ExtractFrustum();
extern void tree(float, float, float, float, float, float, int);

	/* allows users to define colours */
extern int setUserColour(int, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat,
    GLfloat, GLfloat, GLfloat);
void unsetUserColour(int);
extern void getUserColour(int, GLfloat *, GLfloat *, GLfloat *, GLfloat *,
    GLfloat *, GLfloat *, GLfloat *, GLfloat *); 

	/* mesh creation, translatio, rotation functions */
extern void setMeshID(int, int, float, float, float);
extern void unsetMeshID(int);
extern void setTranslateMesh(int, float, float, float);
extern void setRotateMesh(int, float, float, float);
extern void setScaleMesh(int, float);
extern void drawMesh(int);
extern void hideMesh(int);

/********* end of extern variable declarations **************/

/*
 * Wipe the cloud layer only
 */

void wipeClouds(){
   int x, z;
   for(x = 0; x < 100; x++){
      for(z = 0; z < 100; z++){
         world[x][cloudHeight][z] = 0;
      }
   }
   return;
}

/*
 * Animate clouds for a given tick
 */
void animateClouds(int delta){
   int x, y;
   // Update cloud offset
   xCloudOffset+= 10.0/2500.0 * delta;
   yCloudOffset+= 10.0/2500.0 * delta;
   // Wipe the old clouds
   wipeClouds();
   // Draw the new ones
   for(int y = 0; y < 100; y++){
      for(int x = 0; x < 100; x++){
         float val = perlin2d((float)(x + xCloudOffset), (float)(y + yCloudOffset), 0.1, 1);
         if(val > 0.75){
            world[x][cloudHeight][y] = 10;
         }
      }
   }
   return;
}

/*
 * If an outdoor level is loaded, this will get the height at a given x y coordinate
 */
int getHeight(int x, int y){
   int i;
   for(i = 25; i < 50; i++){
      if(world[x][i][y] == 0){
         return i - 1;
      }
   }
   return 0; // PROBLEM!
}

/*
 * Clear the world array so it can be repainted
 */
void wipeWorld(){
   int x, y, z;
   for(x = 0; x < 100; x++){
      for(y = 0; y < 50; y++){
         for(z = 0; z < 100; z++){
            world[x][y][z] = 0;
         }
      }
   }
   return;
}

/*
 * Render the world at a given floor number
 */
void buildFloor(int floorNum){
   /* Data struct containing all information for a single floor in the dungeon */
   struct floor* dungeonFloor; 

   int i, x, y;
   int ceilHeight;
   int drawHeight = 25; // World draw height (starting)
   // Disable fleight 
   flycontrol = 0;

   // Check if we are hitting a new floor
   if(levelStack.maxFloors < floorNum + 1){
      levelStack.maxFloors++;
      levelStack.floors = realloc(levelStack.floors, sizeof(struct floor*) * levelStack.maxFloors);
      if(floorNum == 0){
         levelStack.floors[floorNum] = initMaze(100, 100, true);
      } else {
         levelStack.floors[floorNum] = initMaze(100, 100, false);
      }
   } 
   levelStack.currentFloor = floorNum;

   // Grab reference to the current floor
   dungeonFloor = levelStack.floors[floorNum];

   if(DEBUG==0 && !dungeonFloor->isOutdoors){
      printf("Retrieved world of size: %d %d\n", dungeonFloor->floorWidth, dungeonFloor->floorHeight);
      printMaze(dungeonFloor);
   }
   // Next build the world data

   // Check if we're outdoors
   if(dungeonFloor->isOutdoors){
      int maxHeight = 22; // Maximum height for the terrain
      int snowHeight = 16; // Snow occurs at 16 above base
      int grassHeight = 8; // Grass occurs at 8 above base

      for(y = 0; y < dungeonFloor->floorHeight; y++){
         for(x = 0; x < dungeonFloor->floorWidth; x++){
            // Get the height map at this coordinate
            int yCap = drawHeight + floor(((dungeonFloor->heightMap[x][y])*maxHeight));
            // Draw from the top to the bottom, picking the appropriate colour as we go
            for(i = yCap; i >= drawHeight; i--){
               if(i >= snowHeight + drawHeight){
                  world[x][i][y] = SNOW_ID;
               } else if(i >= grassHeight + drawHeight && i < snowHeight + drawHeight){
                  world[x][i][y] = GRASS_ID; 
               } else if(i < grassHeight + drawHeight){
                  world[x][i][y] = DIRT_ID;
               }
            }
         }
      }

      // Randomly place the stairs and player somewhere in the middle of the map if they haven't been
      while(dungeonFloor->sx == -1){
         // Pick a random spot
         x = randRange(20, 80);
         y = randRange(20, 80);
         // Get height
         int h = getHeight(x, y);

         // Make sure the stairs are placed in a dirt covered area (Contrast)
         if(h <= grassHeight + drawHeight){
               // Get coordinates for stairs
               dungeonFloor->sx = x;
               dungeonFloor->sy = h + 1;
               dungeonFloor->sz = y;
               // Get coordinates for player
               dungeonFloor->px = randRange(x - 5, x + 5);
               dungeonFloor->pz = randRange(y - 5, y + 5);
               dungeonFloor->py = getHeight(dungeonFloor->px, dungeonFloor->pz) + 1;
         }
      }
      // Put the player and stairs in
      setViewPosition(-dungeonFloor->px - 0.5, -dungeonFloor->py - 2, -dungeonFloor->pz - 0.5);
      setViewOrientation(0, 0, 0);
      world[dungeonFloor->sx][dungeonFloor->sy][dungeonFloor->sz] = DSTAIRS_ID;

   // Otherwise we're indoors, generate the rooms etc
   } else {
      for(y = 0; y < dungeonFloor->floorHeight; y++){
         for(x = 0; x < dungeonFloor->floorWidth; x++){
            ceilHeight = getCeilHeight(dungeonFloor, x, y);
            // Floor tile 1
            if(dungeonFloor->floorData[x][y]=='.'){
               world[x][drawHeight][y] = TILE1_ID;
               world[x][drawHeight+ceilHeight+1][y] = CEIL_ID;
            }
            // Floor tile 2
            else if(dungeonFloor->floorData[x][y]==','){
               world[x][drawHeight][y] = TILE2_ID;
               world[x][drawHeight+ceilHeight+1][y] = CEIL_ID;
            }
            // Corridors
            else if(dungeonFloor->floorData[x][y]=='+'){
               world[x][drawHeight][y] = CORR_FLR_ID; 
               world[x][drawHeight+ceilHeight+1][y] = CORR_CEIL_ID;
            }
            // Walls
            else if(dungeonFloor->floorData[x][y]=='#'){
               for(i = 0; i <= ceilHeight + 1; i++){
                  world[x][drawHeight+i][y] = WALL_ID;
               }
            }
            // Doors
            else if(dungeonFloor->floorData[x][y]=='/'){
               world[x][drawHeight][y] = DOOR_FLR_ID; // Draw floor below the door
               world[x][drawHeight+1][y] = DOOR_UP_ID; // Draw the door itself
               world[x][drawHeight+2][y] = DOOR_LOW_ID; // Draw the door itself
               world[x][drawHeight+3][y] = DOOR_DEC_ID; // Draw the art above the door 
               for(i = 4; i <= ceilHeight + 1; i++){
                  world[x][drawHeight+i][y] = WALL_ID;
               }
            }
            // Open Doors
            else if(dungeonFloor->floorData[x][y]=='|'){
               world[x][drawHeight][y] = DOOR_FLR_ID; // Draw floor below the door
               world[x][drawHeight+1][y] = 0; // Draw the open door
               world[x][drawHeight+2][y] = 0; // Draw the open door
               world[x][drawHeight+3][y] = DOOR_DEC_ID; // Draw the art above the door 
               for(i = 4; i <= ceilHeight + 1; i++){
                  world[x][drawHeight+i][y] = WALL_ID;
               }
            }
         }
      }
      // Next iterate over the entity list (For now just player placement & boxes)
      for(y = 0; y < dungeonFloor->floorHeight; y++){
         for(x = 0; x < dungeonFloor->floorWidth; x++){
            // Player found! Setup at coordinates
            if(dungeonFloor->floorEntities[x][y]=='@'){
               if(DEBUG==0)
                  printf("Setting player 0 at (%d, %d, %d)...\n", x, drawHeight+1, y);
               // Setup viewport
               setOldViewPosition(-x - 0.5, -drawHeight - 2, -y - 0.5);
               setViewPosition(-x - 0.5, -drawHeight - 2, -y - 0.5);
               setViewOrientation(0, 0, 0);
               // Wipe player reference point so it can be saved when they move to a different staircase
               dungeonFloor->floorEntities[x][y] = ' ';
            // Box found!
            } else if(dungeonFloor->floorEntities[x][y]=='B'){
               world[x][drawHeight+1][y] = BOX_ID; // Draw a box
            } else if(dungeonFloor->floorEntities[x][y]=='U'){
               world[x][drawHeight+1][y] = USTAIRS_ID; // Draw a upward staircase
            } else if(dungeonFloor->floorEntities[x][y]=='D'){
               world[x][drawHeight+1][y] = DSTAIRS_ID; // Draw a downward staircase
            }
         }
      }
   }
   return;
}

	/*** collisionResponse() ***/
	/* -performs collision detection and response */
	/*  sets new xyz  to position of the viewpoint after collision */
	/* -can also be used to implement gravity by updating y position of vp*/
	/* note that the world coordinates returned from getViewPosition()
	   will be the negative value of the array indices */
void collisionResponse() {

	/* your code for collisions goes here */

   // Get what we are about to hit
   int hit;
   // Current viewpoint coords
   float x, y, z;
   // Old viewpoint coords
   float oX, oY, oZ;
   // New viewport coords (extrapolate where we're heading)
   float nX, nY, nZ;

   // Populate our coord values
   getViewPosition(&x, &y, &z);
   getOldViewPosition(&oX, &oY, &oZ);
   
   // Invert values to make calculations easier
   x = -x;
   y = -y;
   z = -z;
   oX = -oX;
   oY = -oY;
   oZ = -oZ;

   // Calculate our predicted position if we keep going this way
   nX = x + 2*(x - oX);
   nY = y + 2*(y - oY);
   nZ = z + 2*(z - oZ);

   // Perform collision check at the predicted space
   hit = world[(int)nX][(int)nY][(int)nZ];

   // Set consistent floor position if standing on solid ground
   if(world[(int)x][(int)(y-1)][(int)z] != 0){
      // Check for stairs
      if(world[(int)x][(int)(y-1)][(int)z] == DSTAIRS_ID){
         if(DEBUG == 0){
            printf("Going downstairs!\n");
         }
         // Save player position
         if(levelStack.floors[levelStack.currentFloor]->isOutdoors){
            // Save player location as being north of the stairs
            levelStack.floors[levelStack.currentFloor]->px = x + 1;
            levelStack.floors[levelStack.currentFloor]->py = getHeight((int)(x + 1), (int)z);
            levelStack.floors[levelStack.currentFloor]->pz = z;
            
         // We're indoors, just use entity array to save position
         } else {
            // TODO: Check for a valid space around the staircase to store player location
            levelStack.floors[levelStack.currentFloor]->floorEntities[(int)x + 1][(int)z] = '@';
         }
         // Wipe array
         wipeWorld();
         // Loadup next floor
         buildFloor(levelStack.currentFloor + 1); 
         // Bail
         return; 
      } else if(world[(int)x][(int)(y-1)][(int)z] == USTAIRS_ID){
         if(!levelStack.floors[levelStack.currentFloor]->isOutdoors){
            if(DEBUG == 0){
               printf("Going upstairs!\n");
            }
            // Save player location
            levelStack.floors[levelStack.currentFloor]->floorEntities[(int)x + 1][(int)z] = '@';
            // Wipe array
            wipeWorld();
            // Loadup next floor
            buildFloor(levelStack.currentFloor - 1);  
            // Bail
            return; 
         }
      }
      setViewPosition(-x, -(floor(y)+1), -z);
   }

   // Collision!
   if(hit != 0){
      // Check if we're indoors or outdoors for collision logic
      if(levelStack.floors[levelStack.currentFloor]->isOutdoors){
         // Perform climb check
         if(world[(int)nX][(int)nY][(int)nZ] == 0){
            // Climb the box
            setViewPosition(-nX, -nY - 1, -nZ);
         // Perform floor check
         } else {
            // Revert our position back to the old position
            setViewPosition(-oX, -oY, -oZ);
         }
      } else {
         if(hit == DOOR_LOW_ID || hit == DOOR_UP_ID){
            // Mark door as opened in world data
            levelStack.floors[levelStack.currentFloor]->floorData[(int)nX][(int)nZ] = '|';
            // Open this door block
            world[(int)nX][(int)nY][(int)nZ] = 0;
            // Open the door block above or below this one
            if(world[(int)nX][(int)nY - 1][(int)nZ] == DOOR_UP_ID) {
               world[(int)nX][(int)nY - 1][(int)nZ] = 0;
            } else {
               world[(int)nX][(int)nY + 1][(int)nZ] = 0;
            }
         // We hit a solid block
         } else {
            // Perform climb check
            if(world[(int)nX][(int)nY + 1][(int)nZ] == 0){
               // Climb the box
               setViewPosition(-nX, -nY - 1, -nZ);
            // Perform floor check
            } else {
               // Revert our position back to the old position
               setViewPosition(-oX, -oY, -oZ);
            }
         }
      }
      // General collision logic goes here
      // We hit a door, open it
       
   }
   return;
}


	/******* draw2D() *******/
	/* draws 2D shapes on screen */
	/* use the following functions: 			*/
	/*	draw2Dline(int, int, int, int, int);		*/
	/*	draw2Dbox(int, int, int, int);			*/
	/*	draw2Dtriangle(int, int, int, int, int, int);	*/
	/*	set2Dcolour(float []); 				*/
	/* colour must be set before other functions are called	*/
void draw2D() {

   if (testWorld) {
		/* draw some sample 2d shapes */
      if (displayMap == 1) {
         GLfloat green[] = {0.0, 0.5, 0.0, 0.5};
         set2Dcolour(green);
         draw2Dline(0, 0, 500, 500, 15);
         draw2Dtriangle(0, 0, 200, 200, 0, 200);

         GLfloat black[] = {0.0, 0.0, 0.0, 0.5};
         set2Dcolour(black);
         draw2Dbox(500, 380, 524, 388);
      }
   } else {

	/* your code goes here */

   }

}


	/*** update() ***/
	/* background process, it is called when there are no other events */
	/* -used to control animations and perform calculations while the  */
	/*  system is running */
	/* -gravity must also implemented here, duplicate collisionResponse */
void update() {
int i, j, k;
float *la;
float x, y, z;

	/* sample animation for the testworld, don't remove this code */
	/* demo of animating mobs */
   if (testWorld) {

	/* update old position so it contains the correct value */
	/* -otherwise view position is only correct after a key is */
	/*  pressed and keyboard() executes. */

      getViewPosition(&x, &y, &z);
      setOldViewPosition(x,y,z);

	/* sample of rotation and positioning of mob */
	/* coordinates for mob 0 */
      static float mob0x = 50.0, mob0y = 25.0, mob0z = 52.0;
      static float mob0ry = 0.0;
      static int increasingmob0 = 1;
	/* coordinates for mob 1 */
      static float mob1x = 50.0, mob1y = 25.0, mob1z = 52.0;
      static float mob1ry = 0.0;
      static int increasingmob1 = 1;
	/* counter for user defined colour changes */
      static int colourCount = 0;
      static GLfloat offset = 0.0;

	/* offset counter for animated texture */
      static float textureOffset = 0.0;

	/* scaling values for fish mesh */
      static float fishScale = 1.0;
      static int scaleCount = 0;
      static GLfloat scaleOffset = 0.0;

	/* move mob 0 and rotate */
	/* set mob 0 position */
      setMobPosition(0, mob0x, mob0y, mob0z, mob0ry);

	/* move mob 0 in the x axis */
      if (increasingmob0 == 1)
         mob0x += 0.2;
      else 
         mob0x -= 0.2;
      if (mob0x > 50) increasingmob0 = 0;
      if (mob0x < 30) increasingmob0 = 1;

	/* rotate mob 0 around the y axis */
      mob0ry += 1.0;
      if (mob0ry > 360.0) mob0ry -= 360.0;

	/* move mob 1 and rotate */
      setMobPosition(1, mob1x, mob1y, mob1z, mob1ry);

	/* move mob 1 in the z axis */
	/* when mob is moving away it is visible, when moving back it */
	/* is hidden */
      if (increasingmob1 == 1) {
         mob1z += 0.2;
         showMob(1);
      } else {
         mob1z -= 0.2;
         hideMob(1);
      }
      if (mob1z > 72) increasingmob1 = 0;
      if (mob1z < 52) increasingmob1 = 1;

	/* rotate mob 1 around the y axis */
      mob1ry += 1.0;
      if (mob1ry > 360.0) mob1ry -= 360.0;

	/* change user defined colour over time */
      if (colourCount == 1) offset += 0.05;
      else offset -= 0.01;
      if (offset >= 0.5) colourCount = 0;
      if (offset <= 0.0) colourCount = 1;
      setUserColour(9, 0.7, 0.3 + offset, 0.7, 1.0, 0.3, 0.15 + offset, 0.3, 1.0);

	/* sample tube creation  */
	/* draws a purple tube above the other sample objects */
       createTube(1, 45.0, 30.0, 45.0, 50.0, 30.0, 50.0, 6);

	/* move texture for lava effect */
      textureOffset -= 0.01;
      setTextureOffset(18, 0.0, textureOffset);

	/* make fish grow and shrink (scaling) */
      if (scaleCount == 1) scaleOffset += 0.01;
      else scaleOffset -= 0.01;
      if (scaleOffset >= 0.5) scaleCount = 0;
      if (scaleOffset <= 0.0) scaleCount = 1;
      setScaleMesh(1, 0.5 + scaleOffset);

	/* make cow with id == 2 appear and disappear */
	/* use scaleCount as switch to flip draw/hide */
	/* rotate cow while it is visible */
      if (scaleCount == 0) {
         drawMesh(2);
         setRotateMesh(2, 0.0, 180.0 + scaleOffset * 100.0, 0.0);
      } else {
         hideMesh(2);
      }

    /* end testworld animation */


   } else {

	/* your code goes here */
      // Get time since last update
      int startTime = glutGet(GLUT_ELAPSED_TIME);
      int delta = startTime - oldTime;
      oldTime = startTime;

      // Get old position data
      getViewPosition(&x, &y, &z);
      setOldViewPosition(x,y,z);

      // Apply gravity
      y += 0.1;

      // Update view position
      setViewPosition(x, y, z);

      // Perform a collision check
      collisionResponse();

      // Animate clouds if we're outside
      if(levelStack.floors[levelStack.currentFloor]->isOutdoors){
         animateClouds(delta);
      }
   }
}


	/* called by GLUT when a mouse button is pressed or released */
	/* -button indicates which button was pressed or released */
	/* -state indicates a button down or button up event */
	/* -x,y are the screen coordinates when the mouse is pressed or */
	/*  released */ 
void mouse(int button, int state, int x, int y) {

   if (button == GLUT_LEFT_BUTTON)
      printf("left button - ");
   else if (button == GLUT_MIDDLE_BUTTON)
      printf("middle button - ");
   else
      printf("right button - ");

   if (state == GLUT_UP)
      printf("up - ");
   else
      printf("down - ");

   printf("%d %d\n", x, y);
}



int main(int argc, char** argv)
{
int i, j, k;
	/* initialize the graphics system */
   graphicsInit(&argc, argv);


	/* the first part of this if statement builds a sample */
	/* world which will be used for testing */
	/* DO NOT remove this code. */
	/* Put your code in the else statment below */
	/* The testworld is only guaranteed to work with a world of
		with dimensions of 100,50,100. */
   if (testWorld == 1) {
	/* initialize world to empty */
      for(i=0; i<WORLDX; i++)
         for(j=0; j<WORLDY; j++)
            for(k=0; k<WORLDZ; k++)
               world[i][j][k] = 0;

	/* some sample objects */
	/* build a red platform */
      for(i=0; i<WORLDX; i++) {
         for(j=0; j<WORLDZ; j++) {
            world[i][24][j] = 3;
         }
      }
	/* create some green and blue cubes */
      world[50][25][50] = 1;
      world[49][25][50] = 1;
      world[49][26][50] = 1;
      world[52][25][52] = 2;
      world[52][26][52] = 2;

	/* create user defined colour and draw cube */
      setUserColour(9, 0.7, 0.3, 0.7, 1.0, 0.3, 0.15, 0.3, 1.0);
      world[54][25][50] = 9;


	/* blue box shows xy bounds of the world */
      for(i=0; i<WORLDX-1; i++) {
         world[i][25][0] = 2;
         world[i][25][WORLDZ-1] = 2;
      }
      for(i=0; i<WORLDZ-1; i++) {
         world[0][25][i] = 2;
         world[WORLDX-1][25][i] = 2;
      }

	/* create two sample mobs */
	/* these are animated in the update() function */
      createMob(0, 50.0, 25.0, 52.0, 0.0);
      createMob(1, 50.0, 25.0, 52.0, 0.0);

	/* create sample player */
      createPlayer(0, 52.0, 27.0, 52.0, 0.0);

	/* texture examples */

	/* create textured cube */
	/* create user defined colour with an id number of 11 */
      setUserColour(11, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);
	/* attach texture 22 to colour id 11 */
      setAssignedTexture(11, 22);
	/* place a cube in the world using colour id 11 which is texture 22 */
      world[59][25][50] = 11;

	/* create textured cube */
      setUserColour(12, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);
      setAssignedTexture(12, 27);
      world[61][25][50] = 12;

	/* create textured cube */
      setUserColour(10, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);
      setAssignedTexture(10, 26);
      world[63][25][50] = 10;

	/* create textured floor */
      setUserColour(13, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);
      setAssignedTexture(13, 8);
      for (i=57; i<67; i++)
         for (j=45; j<55; j++)
            world[i][24][j] = 13;

	/* create textured wall */
      setUserColour(14, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);
      setAssignedTexture(14, 18);
      for (i=57; i<67; i++)
         for (j=0; j<4; j++)
            world[i][24+j][45] = 14;

	/* create textured wall */
      setUserColour(15, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);
      setAssignedTexture(15, 42);
      for (i=45; i<55; i++)
         for (j=0; j<4; j++)
            world[57][24+j][i] = 15;

		// two cubes using the same texture but one is offset
		// cube with offset texture 33
      setUserColour(16, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);
      setAssignedTexture(16, 33);
      world[65][25][50] = 16;
      setTextureOffset(16, 0.5, 0.5);
		// cube with non-offset texture 33
      setUserColour(17, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);
      setAssignedTexture(17, 33);
      world[66][25][50] = 17;

		// create some lava textures that will be animated
      setUserColour(18, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);
      setAssignedTexture(18, 24);
      world[62][24][55] = 18;
      world[63][24][55] = 18;
      world[64][24][55] = 18;
      world[62][24][56] = 18;
      world[63][24][56] = 18;
      world[64][24][56] = 18;

		// draw cow mesh and rotate 45 degrees around the y axis
		// game id = 0, cow mesh id == 0
      setMeshID(0, 0, 48.0, 26.0, 50.0);
      setRotateMesh(0, 0.0, 45.0, 0.0);

		// draw fish mesh and scale to half size (0.5)
		// game id = 1, fish mesh id == 1
      setMeshID(1, 1, 51.0, 28.0, 50.0);
      setScaleMesh(1, 0.5);

		// draw cow mesh and rotate 45 degrees around the y axis
		// game id = 2, cow mesh id == 0
      setMeshID(2, 0, 59.0, 26.0, 47.0);

		// draw bat
		// game id = 3, bat mesh id == 2
      setMeshID(3, 2, 61.0, 26.0, 47.0);
      setScaleMesh(3, 0.5);
		// draw cactus
		// game id = 4, cactus mesh id == 3
      setMeshID(4, 3, 63.0, 26.0, 47.0);
      setScaleMesh(4, 0.5);


   } else {

	/* your code to build the world goes here */
      // Setup clouds
      xCloudOffset = 0;
      yCloudOffset = 0;

      /* Register all our custom colours and textures */
      /* COLOURS */
      setUserColour(DSTAIRS_ID, 0.2, 0.2, 0.2, 1.0, 0.6, 0.6, 0.6, 1.0);
      setUserColour(CLOUD_ID, 0.7, 0.7, 0.7, 1.0, 0.95, 0.95, 0.95, 1.0);
      setUserColour(GRASS_ID, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0); 
      setUserColour(DIRT_ID, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);
      setUserColour(SNOW_ID, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);
      setUserColour(USTAIRS_ID, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);
      setUserColour(TILE1_ID, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);
      setUserColour(TILE2_ID, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);
      setUserColour(CEIL_ID, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);
      setUserColour(CORR_FLR_ID, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);
      setUserColour(CORR_CEIL_ID, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);
      setUserColour(WALL_ID, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);
      setUserColour(DOOR_FLR_ID, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);
      setUserColour(DOOR_LOW_ID, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);
      setUserColour(DOOR_UP_ID, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);
      setUserColour(DOOR_DEC_ID, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);
      setUserColour(BOX_ID, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);
      

      /* TEXTURES */
      setAssignedTexture(DSTAIRS_ID, DSTAIRS_TEX);
      setAssignedTexture(GRASS_ID, GRASS_TEX); 
      setAssignedTexture(CLOUD_ID, CLOUD_TEX);
      setAssignedTexture(DIRT_ID, DIRT_TEX);
      setAssignedTexture(SNOW_ID, SNOW_TEX);
      setAssignedTexture(USTAIRS_ID, USTAIRS_TEX);
      setAssignedTexture(TILE1_ID, TILE1_TEX);
      setAssignedTexture(TILE2_ID, TILE2_TEX);
      setAssignedTexture(CEIL_ID, CEIL_TEX);
      setAssignedTexture(CORR_FLR_ID, CORR_FLR_TEX);
      setAssignedTexture(CORR_CEIL_ID, CORR_CEIL_TEX);
      setAssignedTexture(WALL_ID, WALL_TEX);
      setAssignedTexture(DOOR_FLR_ID, DOOR_FLR_TEX);
      setAssignedTexture(DOOR_LOW_ID, DOOR_LOW_TEX);
      setAssignedTexture(DOOR_UP_ID, DOOR_UP_TEX);
      setAssignedTexture(DOOR_DEC_ID, DOOR_DEC_TEX);
      setAssignedTexture(BOX_ID, BOX_TEX);


      // Load up level 0 to start
      levelStack.currentFloor = 0;
      levelStack.maxFloors = 0;
      buildFloor(levelStack.currentFloor);
   }


	/* starts the graphics processing loop */
	/* code after this will not run until the program exits */
   glutMainLoop();
   return 0; 
}

