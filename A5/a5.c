
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
#include "visible.h"

extern GLubyte  world[WORLDX][WORLDY][WORLDZ];

   /* Collection of floors for holding world data */
static struct floor_stack levelStack;

   /* x y coordinates offset for cloud moving */
static float xCloudOffset, yCloudOffset;
   /* Flags to indicate if the player has grabbed the sword, bow or armour items */
static bool hasSword, hasBow, hasArmour;
   /*  Draw height for clouds */
static int cloudHeight = 49; 
   /* Time since last game tick */
static int oldTime = 0;
   /* Draw distance for entities */
static float drawDist = 35.0; // Updated per floor

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
 * Indicate to mob if they are OK to move to a given cell (Also opens closed doors)
 */
bool clearToMove(struct position toCheck){
   char lvlLook = levelStack.floors[levelStack.currentFloor]->floorData[toCheck.x][toCheck.y];
   char entLook = levelStack.floors[levelStack.currentFloor]->floorEntities[toCheck.x][toCheck.y];

   switch(lvlLook){
      case '/': // Closed door, lets open it!
         world[toCheck.x][26][toCheck.y] = 0; 
         world[toCheck.x][27][toCheck.y] = 0; 
         levelStack.floors[levelStack.currentFloor]->floorData[toCheck.x][toCheck.y] = '|';
      case '|':
      case '.':
      case ',':
      case '+':
         break;
      case '#':
         return false;
      default:
         return false; // Found unexpected result
   }
   switch(entLook){
      case ' ':
         break;
      default:
         return false;
   }
   // Check active mobs (Shouldn't overlap with ourselves)
   int id;
   for(id = 0; id < levelStack.floors[levelStack.currentFloor]->mobCount; id++){
      if(!levelStack.floors[levelStack.currentFloor]->mobs[id].is_active){
         continue;
      } else if(posMatch(levelStack.floors[levelStack.currentFloor]->mobs[id].location, toCheck) || 
         posMatch(levelStack.floors[levelStack.currentFloor]->mobs[id].next_location, toCheck)){
         return false; // Space is occupied
      }
   }
   return true;
}
/*
 * Returns direction from point a to b (cardinal)
 */
int dirPointToPoint(struct position a, struct position b){
   if(a.x == b.x && a.y > b.y) return NORTH;
   else if(a.x == b.x && a.y < b.y) return SOUTH;
   else if(a.y == b.y && a.x < b.x) return EAST;
   else if(a.y == b.y && a.x > b.x) return WEST;
   else return NORTH; // Error out safely
}
/*
 * Return direction from current position to viewport
 */
int dirToPlayer(struct position p){
   float vX, vY, vZ;
   getViewPosition(&vX, &vY, &vZ);
   vX = -vX;
   vY = -vY;
   vZ = -vZ;
   int x = floor(vX) - p.x;
   int y = floor(vZ) - p.y;
   float angle = atan2(y, x);
   if( (angle>=-M_PI/4 && angle <= M_PI/4) ){
      return EAST;
   } else if(angle >= M_PI/4 && angle <= (3*M_PI)/4){
      return NORTH;
   } else if(angle >= (3*M_PI)/4 || angle <= -(3*M_PI)/4){
      return WEST;
   } else if(angle <= -(M_PI)/4 && angle >= -(3*M_PI)/4){
      return SOUTH;
   } else {
      fprintf(stderr, "ERROR: Could not properly determine angle between points (%d, %d) and (%d, %d)! Retrieved angle: %lf\n", 
         (int)floor(vX), (int)floor(vZ), p.x, p.y, angle);
      return 0;
   }
}
/*
 * Rotate a mob to face a new direction
 */
void faceDirection(int id, struct mob *m, int new_dir){
   switch(m->facing){
      case NORTH:
         switch(new_dir){
            case NORTH:
               break; // Moving same as initial direction, do nothing
            case SOUTH:
               m->rotY -= 180;
               break; 
            case EAST:
               m->rotY += 90;
               break;
            case WEST:
               m->rotY -= 90;
               break;
            default:
               fprintf(stderr, "ERROR: Invalid new direction for mob %d: %d\n!", id, new_dir);
               break;
         }
         break;
      case SOUTH:
         switch(new_dir){
            case NORTH:
               m->rotY -= 180;
               break;
            case SOUTH: 
               break; // Moving same as initial direction, do nothing
            case EAST:
               m->rotY -= 90;
               break;
            case WEST:
               m->rotY += 90;
               break;
            default:
               fprintf(stderr, "ERROR: Invalid new direction for mob %d: %d\n!", id, new_dir);
               break;
         }
         break;
      case EAST:
         switch(new_dir){
            case NORTH:
               m->rotY -= 90;
               break;
            case SOUTH:
               m->rotY += 90;
               break;
            case EAST:
               break; // Moving same as initial direction, do nothing
            case WEST:
               m->rotY -= 180;
               break;
            default:
               fprintf(stderr, "ERROR: Invalid new direction for mob %d: %d\n!", id, new_dir);
               break;
         }
         break;
      case WEST:
         switch(new_dir){
            case NORTH:
               m->rotY += 90;
               break;
            case SOUTH:
               m->rotY -= 90;
               break;
            case EAST:
               m->rotY -= 180;
               break;
            case WEST:
               break; // Moving same as initial direction, do nothing
            default:
               fprintf(stderr, "ERROR: Invalid new direction for mob %d: %d\n!", id, new_dir);
               break;
         }
         break;
      default:
         fprintf(stderr, "ERROR: Invalid facing direction for mob %d: %d\n!", id, m->facing);
         break;
   }
   // Update new facing direction
   m->facing = new_dir;
   // Apply new rotation direction
   setRotateMesh(id, m->rotX, m->rotY, m->rotZ);
   return;
}
/*
 * Check if player is within attack range of mob position
 */
bool isPlayerAttackable(struct position p){
   float pX, pY, pZ;
   getViewPosition(&pX, &pY, &pZ);
   pX = -pX;
   pY = -pY;
   pZ = -pZ;
   int x, y;
   x = floor(pX);
   y = floor(pZ);
   if(abs(x - p.x)<=1 && abs(y - p.y)<=1){
      return true;
   } else {
      return false;
   }
}
/*
 * Have mob attack player
 */
void attackPlayer(int id, struct mob *m){
   // "Combat"
   int chance = randRange(0, 1);
   faceDirection(id, m, dirToPlayer(m->location));
   if(chance){
      printf("Mob %d hit player!\n", id);
      // TODO: Combat calculations here, etc.
   } else {
      printf("Mob %d swung at the player and missed!\n", id);
   }
   return;
}
/*
 * Turn logic for the cactus
 */
void cactusTurn(int id, struct mob *m){
   // Cactus can only be in one of 2 states, IDLE or ATTACKING.
   // Determined by Player entering attack range.
   if(isPlayerAttackable(m->location)){
      m->state = ATTACKING;
   } else {
      m->state = IDLE;
   }
   switch(m->state){
      case IDLE:
         break;
      case ATTACKING:
         attackPlayer(id, m);
         break;
   }
   return;
}
/*
 * Turn logic for the fish
 */
void fishTurn(int id, struct mob *m){
   if(isPlayerAttackable(m->location) && m->is_aggro){
      m->state = ATTACKING;
      m->is_moving = false; // If we're attacking, we can't move into the player space.
   } else if(m->is_aggro){
      m->state = PURSUING;
   }
   switch(m->state){
      case IDLE: // Don't sit still
         if(m->is_aggro){
            m->state = PURSUING;
         } else {
            m->state = ROAMING;
         }
         break;
      case ROAMING:
         if(m->my_path == NULL || m->my_path->numPoints <=0 || m->my_path->currPoint >= m->my_path->numPoints){
            struct position toGo = randPosInSameRoom(levelStack.floors[levelStack.currentFloor], m->location);
            m->my_path = aStar(levelStack.floors[levelStack.currentFloor], m->location, toGo);
            // Make sure we got a valid path back (can get to position)
            if(m->my_path == NULL){
               m->state = IDLE; // Kick into IDLE
               break;
            }
         } 
         if(m->my_path->currPoint < m->my_path->numPoints && clearToMove(m->my_path->points[m->my_path->currPoint])){
            m->is_moving = true;
            m->destX = m->my_path->points[m->my_path->currPoint].x + 0.5;
            m->destY = m->worldY;
            m->destZ = m->my_path->points[m->my_path->currPoint].y + 0.5;
            m->next_location.x = m->my_path->points[m->my_path->currPoint].x;
            m->next_location.y = m->my_path->points[m->my_path->currPoint].y;
            m->my_path->currPoint++;
            // Rotate to face new direction
            int dir = dirPointToPoint(m->location, m->next_location);
            faceDirection(id, m, dir);
         } else if(!clearToMove(m->my_path->points[m->my_path->currPoint])){
            m->state = STUCK;
            m->stuckCount = 0;
         }
         break;
      case PURSUING:;
         struct position playerPos;
         playerPos.x = -(levelStack.floors[levelStack.currentFloor]->px);
         playerPos.y = -(levelStack.floors[levelStack.currentFloor]->pz);
         int stepsToPlayer = hueristic(playerPos, m->location);
         if(stepsToPlayer < 16 || m->my_path == NULL || m->my_path->numPoints <=0 
            || m->my_path->currPoint >= m->my_path->numPoints || hueristic(playerPos, m->my_path->points[m->my_path->numPoints-1]) > 8){
            m->my_path = aStar(levelStack.floors[levelStack.currentFloor], m->location, playerPos);
            // Make sure we got a valid path back (can get to position)
            if(m->my_path == NULL){
               m->state = IDLE; // Kick into IDLE
               break;
            }
         } 
         if(m->my_path->currPoint < m->my_path->numPoints && clearToMove(m->my_path->points[m->my_path->currPoint])){
            m->is_moving = true;
            m->destX = m->my_path->points[m->my_path->currPoint].x + 0.5;
            m->destY = m->worldY;
            m->destZ = m->my_path->points[m->my_path->currPoint].y + 0.5;
            m->next_location.x = m->my_path->points[m->my_path->currPoint].x;
            m->next_location.y = m->my_path->points[m->my_path->currPoint].y;
            m->my_path->currPoint++;
            // Rotate to face new direction
            int dir = dirPointToPoint(m->location, m->next_location);
            faceDirection(id, m, dir);
         } else if(!clearToMove(m->my_path->points[m->my_path->currPoint])){
            m->state = STUCK;
            m->stuckCount = 0;
         }
         break;
      case ATTACKING:;
         int dir = dirToPlayer(m->location);
         faceDirection(id, m, dir);
         attackPlayer(id, m);
         break;
      case STUCK:
         // Check if we can move 
         if(clearToMove(m->my_path->points[m->my_path->currPoint])){
            if(m->is_aggro) m->state = PURSUING;
            else m->state = ROAMING;
         } else {
            m->stuckCount++;
            // If we've been stuck too long (3+ turns) recalculate the path to our current goal
            if(m->stuckCount>=2){
               // If we have a valid path
               if(m->my_path != NULL){
                  m->my_path = aStar(levelStack.floors[levelStack.currentFloor], m->location, m->my_path->points[m->my_path->numPoints-1]);
               } else {
                  // Kick into IDLE, we've lost our path!
                  m->state = IDLE;
               }
            }
         }
         break;
      default:
         break;
   }
   return;
}
/*
 * Turn logic for the bat
 */
void batTurn(int id, struct mob *m){
   if(isPlayerAttackable(m->location) && m->is_aggro){
      m->state = ATTACKING;
      m->is_moving = false; // If we're attacking, we can't move into the player space.
   } else if(m->is_aggro){
      m->state = PURSUING;
   }
   switch(m->state){
      case IDLE: // Don't sit still
         if(m->is_aggro){
            m->state = PURSUING;
         } else {
            m->state = ROAMING;
         }
         break;
      case ROAMING:
         if(m->my_path == NULL || m->my_path->numPoints <=0 || m->my_path->currPoint >= m->my_path->numPoints){
            struct position toGo = randPosInFloor(levelStack.floors[levelStack.currentFloor]);
            m->my_path = aStar(levelStack.floors[levelStack.currentFloor], m->location, toGo);
            // Make sure we got a valid path back (can get to position)
            if(m->my_path == NULL){
               m->state = IDLE; // Kick into IDLE
               break;
            }
         } 
         if(m->my_path->currPoint < m->my_path->numPoints && clearToMove(m->my_path->points[m->my_path->currPoint])){
            m->is_moving = true;
            m->destX = m->my_path->points[m->my_path->currPoint].x + 0.5;
            m->destY = m->worldY;
            m->destZ = m->my_path->points[m->my_path->currPoint].y + 0.5;
            m->next_location.x = m->my_path->points[m->my_path->currPoint].x;
            m->next_location.y = m->my_path->points[m->my_path->currPoint].y;
            m->my_path->currPoint++;
            // Rotate to face new direction
            int dir = dirPointToPoint(m->location, m->next_location);
            faceDirection(id, m, dir);
         } else if(!clearToMove(m->my_path->points[m->my_path->currPoint])){
            m->state = STUCK;
            m->stuckCount = 0;
         }
         break;
      case PURSUING:;
         struct position playerPos;
         playerPos.x = -(levelStack.floors[levelStack.currentFloor]->px);
         playerPos.y = -(levelStack.floors[levelStack.currentFloor]->pz);
         int stepsToPlayer = hueristic(playerPos, m->location);
         if(stepsToPlayer < 16 || m->my_path == NULL || m->my_path->numPoints <=0 
            || m->my_path->currPoint >= m->my_path->numPoints || hueristic(playerPos, m->my_path->points[m->my_path->numPoints-1]) > 8){
            m->my_path = aStar(levelStack.floors[levelStack.currentFloor], m->location, playerPos);
            // Make sure we got a valid path back (can get to player)
            if(m->my_path == NULL){
               m->state = IDLE; // Kick into IDLE
               break;
            }
         } 
         if(m->my_path->currPoint < m->my_path->numPoints && clearToMove(m->my_path->points[m->my_path->currPoint])){
            m->is_moving = true;
            m->destX = m->my_path->points[m->my_path->currPoint].x + 0.5;
            m->destY = m->worldY;
            m->destZ = m->my_path->points[m->my_path->currPoint].y + 0.5;
            m->next_location.x = m->my_path->points[m->my_path->currPoint].x;
            m->next_location.y = m->my_path->points[m->my_path->currPoint].y;
            m->my_path->currPoint++;
            // Rotate to face new direction
            int dir = dirPointToPoint(m->location, m->next_location);
            faceDirection(id, m, dir);
         } else if(!clearToMove(m->my_path->points[m->my_path->currPoint])){
            m->state = STUCK;
            m->stuckCount = 0;
         }
         break;
      case ATTACKING:;
         int dir = dirToPlayer(m->location);
         faceDirection(id, m, dir);
         attackPlayer(id, m);
         break;
      case STUCK:
         // Check if we can move
         if(m->my_path != NULL && clearToMove(m->my_path->points[m->my_path->currPoint])){
            if(m->is_aggro) m->state = PURSUING;
            else m->state = ROAMING;
         } else {
            m->stuckCount++;
            // If we've been stuck too long (3+ turns) recalculate the path to our current goal
            if(m->stuckCount>=2){
               // If we have a valid path
               if(m->my_path != NULL){
                  m->my_path = aStar(levelStack.floors[levelStack.currentFloor], m->location, m->my_path->points[m->my_path->numPoints-1]);
               } else {
                  // Kick into IDLE, we've lost our path!
                  m->state = IDLE;
               }
            }
         }
         break;
      default:
         break;
   }
   return;
}
/*
 * Visibility check for each mob
 */
void mobVisibleUpdate(){
   // Get a reference to the mob list
   struct mob* list = levelStack.floors[levelStack.currentFloor]->mobs;
   // Get size of list
   int listSize = levelStack.floors[levelStack.currentFloor]->mobCount;
   // Track current id
   int id;
   // Update (Extract) our viewing frustum
   ExtractFrustum();
   float px, py, pz;
   getViewPosition(&px, &py, &pz);
   px = -px;
   py = -py;
   pz = -pz;
   // Run a visible update check on each mob!
   for(id = 0; id < listSize; id++){
      bool inFrust = CubeInFrustum2(list[id].worldX, list[id].worldY, list[id].worldZ, 1);
      float dist = lengthTwoPoints(list[id].worldX, list[id].worldY, list[id].worldZ, px, py, pz);
      // Only update if we're changing status (Switching visible to not visible and vice versa)
      if(!list[id].is_visible && inFrust && dist <= drawDist){
         drawMesh(id);
         list[id].is_visible = true;
         if(!list[id].is_aggro && list[id].symbol!='F'){ // Fish does not aggro on sight, aggros when player enters their room.
            list[id].is_aggro = true;
         }
      } else if(list[id].is_visible && (!inFrust || dist > drawDist)) {
         hideMesh(id);
         list[id].is_visible = false;
      }
   }
   // Job's Done!
   return;
}
/*
 * Linear Interpolation function
 * Interpolates between v0 and v1 by t amount
 */
float lerp(float v0, float v1, float t) {
   // Check if it's close enough to result that we can just return result
   if(fabs(v0 - v1) <= 0.1){
      return v1;
   } else {
      return v0 * (1.0 - t) + v1*t;
   }
}
/*
 * Check if a given space is 'empty'
 */
bool isEmpty(int x, int y){
   char lvlCheck = levelStack.floors[levelStack.currentFloor]->floorData[x][y];
   char entCheck = levelStack.floors[levelStack.currentFloor]->floorEntities[x][y];
   if(lvlCheck == '#' || lvlCheck == '/'){
      return false;
   }
   if(entCheck == 'C' || entCheck == 'B' || entCheck == 'F' || entCheck == '$' || entCheck == 'U' || entCheck == 'D' || entCheck == '@'){
      return false;
   }
   float pX, pY, pZ;
   getViewPosition(&pX, &pY, &pZ);
   pX = -pX;
   pY = -pY;
   pZ = -pZ;
   if((int)pX == x && (int)pZ == y){
      return false;
   }
   return true;
}
/*
 * Check if it's valid to move in a given direction
 */
bool isValidMove(int x, int y, int dir){
   int nx, ny;
   nx = x;
   ny = y;
   switch(dir){
      case NORTH:
         ny++;
         break;
      case SOUTH:
         ny--;
         break;
      case EAST:
         nx++;
         break;
      case WEST:
         nx--;
         break;
   }
   return isEmpty(nx, ny);
}

/*
 * Process all updates for mobs (UPDATED)
 */
void mobUpdate(int delta){
   // Get a reference to the mob list
   struct mob* list = levelStack.floors[levelStack.currentFloor]->mobs;
   // Get size of list
   int listSize = levelStack.floors[levelStack.currentFloor]->mobCount;
   // Track current id
   int id;
   // Iterate over all mobs
   for(id = 0; id < listSize; id++){
      // Skip dead mobs
      if(!list[id].is_active){
         continue;
      }
      // Check if mob needs to take a turn
      if(!list[id].is_moving && list[id].my_turn){
         // Process turns
         switch(list[id].symbol){
            case 'B':
               batTurn(id, &list[id]);
               break;
            case 'C':
               cactusTurn(id, &list[id]);
               break;
            case 'F':
               fishTurn(id, &list[id]);
               break;
            default:
               fprintf(stderr, "ERROR: Unknown mob type %c!\n Cannot take turn.\n", list[id].symbol);
               break;
         }
         // Turn is over
         list[id].my_turn = false;
      // Check if mob is in the middle of moving (lerp)
      } else if(list[id].is_moving){
         // Lerp toward our new position
         list[id].worldX = lerp(list[id].worldX, list[id].destX, delta/25.0);
         list[id].worldY = lerp(list[id].worldY, list[id].destY, delta/25.0);
         list[id].worldZ = lerp(list[id].worldZ, list[id].destZ, delta/25.0);
         // Update position
         setTranslateMesh(id, list[id].worldX, list[id].worldY, list[id].worldZ);
         // Check if mob has reached destination tile
         if(list[id].worldX == list[id].destX &&
            list[id].worldY == list[id].destY &&
            list[id].worldZ == list[id].destZ){
               list[id].location.x = list[id].next_location.x;
               list[id].location.y = list[id].next_location.y;
               list[id].is_moving = false;
               // Check if this was the last tile
               if(list[id].my_path != NULL){
                  if(list[id].my_path->currPoint>=list[id].my_path->numPoints){
                     if(list[id].is_aggro){
                        list[id].state = PURSUING;
                     } else {
                        list[id].state = ROAMING;
                     }
                     free(list[id].my_path->points);
                     free(list[id].my_path);
                     list[id].my_path = NULL;
                  }
               }
            }
      }
   }
   mobVisibleUpdate();
   // Job's Done!
   return;
}

/*
 * Process all updates for items
 */
void itemUpdate(int delta){
   // Get a reference to the item list
   struct item* list = levelStack.floors[levelStack.currentFloor]->items;
   // Get size of list
   int listSize = levelStack.floors[levelStack.currentFloor]->itemCount;
   // Track current id
   int id;
   // Iterate over all mobs
   for(id = 0; id < listSize; id++){
      // Check for rotate-able item (Coin, Sword, Key, Armour, or Bow)
      char s = list[id].symbol;
      if(s == '*' || s == 'S' || s == 'A' || s == 'K' || s == '}'){
         list[id].rotY += delta/4.0;
         setRotateMesh(list[id].meshID, list[id].rotX, list[id].rotY, list[id].rotZ);
      }
   }
}

/*
 * Utility function, indicates if a specific tile is currently visible
 */
bool isVisible(int x, int y){
   // If we're in draw all mode or a cave just return true
   if(displayMap == 1 || levelStack.floors[levelStack.currentFloor]->floorType==CAVE){
      return true;
   // Otherwise check against visibility array
   } else {
      return levelStack.floors[levelStack.currentFloor]->isVisible[x][y];
   }
}

/*
 * Utility function, updates visible array when player is moving to a new coordinate
 */
void updateVisible(int x, int y){
   // Check if a point is in a room
   int x1, y1;
   // Check all rooms
   for(y1 = 0; y1 <= 2; y1++){
      for(x1 = 0; x1 <=2; x1++){
         struct position origin;
         struct position corner;

         origin = levelStack.floors[levelStack.currentFloor]->rooms[x1][y1].origin;
         corner = levelStack.floors[levelStack.currentFloor]->rooms[x1][y1].corner;

         // We've found the room!
         if(origin.x <= x && origin.y <= y && corner.x >= x && corner.y >= y){
            // Flag all tiles in the room as visible
            int x2, y2;
            for(y2 = origin.y; y2 <= corner.y; y2++){
               for(x2 = origin.x; x2 <= corner.x; x2++){
                  levelStack.floors[levelStack.currentFloor]->isVisible[x2][y2] = 1;
               }
            }
            // Look for a fish mob in this room and if it's not aggro'd, aggro it
            int id;
            struct mob* list = levelStack.floors[levelStack.currentFloor]->mobs;
            for(id = 0; id < levelStack.floors[levelStack.currentFloor]->mobCount; id++){
               // If mob is not active or fish type skip
               if(!list[id].is_active || list[id].symbol!='F') continue;
               int mx = list[id].location.x;
               int my = list[id].location.y;
               if(origin.x <= mx && origin.y <= my && corner.x >= mx && corner.y >= my){
                  if(!list[id].is_aggro && list[id].symbol=='F'){
                     list[id].is_aggro = true;
                     list[id].state = PURSUING;
                  }
               }
            }
            return;
         }
      }
   }

   // If no rooms were found, assume we're in a corridor and update a 3x3 area around the coordinate
   for(y1 = -1; y1 <= 1; y1++){
      for(x1 = -1; x1 <=1; x1++){
         levelStack.floors[levelStack.currentFloor]->isVisible[x + x1][y + y1] = 1;
      }
   }

   return;
}

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
 * Clear the world array and meshes so they can be repainted
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
   // Get size of list
   int listSize = levelStack.floors[levelStack.currentFloor]->mobCount;
   // Track current id
   int id;
   // Iterate over all mobs
   for(id = 0; id < listSize; id++){
      unsetMeshID(id);
   }
   // Get size of list
   listSize = levelStack.floors[levelStack.currentFloor]->mobCount + levelStack.floors[levelStack.currentFloor]->itemCount;
   // Iterate over all items
   for(; id < listSize; id++){
      unsetMeshID(id);
   }
   return;
}

/*
 * Flag all mobs to it's their turn
 */
void signalMobTurn(){
   // Get a reference to the mob list
   struct mob* list = levelStack.floors[levelStack.currentFloor]->mobs;
   // Get size of list
   int listSize = levelStack.floors[levelStack.currentFloor]->mobCount;
   // Track current id
   int id;
   // Iterate over all mobs
   for(id = 0; id < listSize; id++){
      // Skip deactivated mobs
      if(!list[id].is_active) continue;
      // Flag turn
      list[id].my_turn = true;
      // If we're in the cave check if we're within 10 tiles of the player (and a fish)
      if(levelStack.floors[levelStack.currentFloor]->floorType==CAVE && list[id].symbol=='F' && !list[id].is_aggro){
         float px, py, pz;
         getViewPosition(&px, &py, &pz);
         px = -px;
         py = -py;
         pz = -pz;
         float dist = lengthTwoPoints(list[id].worldX, list[id].worldY, list[id].worldZ, px, py, pz);
         // Only update if we're changing status (Switching visible to not visible and vice versa)
         if(dist <= 10){
            list[id].is_aggro = true;
         }
      }
   }
   return;
}

/*
 * Check if player has moved into a new tile
 */
void turnCheck(){
   // Check if a turn was made
   float pX, pY, pZ;
   getViewPosition(&pX, &pY, &pZ);
   int iX = (int)pX;
   int iY = (int)pY;
   int iZ = (int)pZ;
   int cX = levelStack.floors[levelStack.currentFloor]->px;
   int cY = levelStack.floors[levelStack.currentFloor]->py;
   int cZ = levelStack.floors[levelStack.currentFloor]->pz;
   // If we've moved more than 1 whole tile, mobs get a turn
   if(cX != iX || cY != iY || cZ != iZ){
      signalMobTurn();
      // Save new player position
      levelStack.floors[levelStack.currentFloor]->px = iX;
      levelStack.floors[levelStack.currentFloor]->py = iY;
      levelStack.floors[levelStack.currentFloor]->pz = iZ;
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
   bool newFloor = false;
   int ceilHeight;
   int drawHeight = 25; // World draw height (starting)
   // Disable fleight 
   flycontrol = 0;

   // Check if we are hitting a new floor
   if(levelStack.maxFloors < floorNum + 1){
      newFloor = true;
      levelStack.maxFloors++;
      levelStack.floors = realloc(levelStack.floors, sizeof(struct floor*) * levelStack.maxFloors);
      if(floorNum == 0){
         levelStack.floors[floorNum] = initMaze(100, 100, OUTSIDE);
         levelStack.floors[floorNum]->stairLocked = false;
      } else if(floorNum%2==0){
         levelStack.floors[floorNum] = initMaze(100, 100, CAVE);
         levelStack.floors[floorNum]->stairLocked = true;
      } else {
         levelStack.floors[floorNum] = initMaze(100, 100, DUNGEON);
         levelStack.floors[floorNum]->stairLocked = true;
      }
   } 
   levelStack.currentFloor = floorNum;

   // Grab reference to the current floor
   dungeonFloor = levelStack.floors[floorNum];

   // Get new draw distance
   drawDist = dungeonFloor->drawDist;

   // Next build the world data

   // Check if we're outdoors
   if(dungeonFloor->floorType==OUTSIDE){
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
   // Check if we're in a cave
   } else if(dungeonFloor->floorType==CAVE){
      // Draw 'Walls'
      for(y = 0; y < dungeonFloor->floorHeight; y++){
         for(x = 0; x < dungeonFloor->floorWidth; x++){
            if(y==0||y==dungeonFloor->floorHeight-1||x==0||x==dungeonFloor->floorWidth-1){
               dungeonFloor->floorData[x][y] = '#';
               for(i = drawHeight; i < drawHeight + 8; i++){
                  world[x][i][y] = CAVE_CEILING_ID;
               }
            }
         }
      }
      // Draw the ceiling
      for(y = 0; y < dungeonFloor->floorHeight; y++){
         for(x = 0; x < dungeonFloor->floorWidth; x++){
            // Generate the dome
            float tx, ty, tz;
            tx = ((float)x/(float)(dungeonFloor->floorWidth/2)) - 1.0;
            tz = ((float)y/(float)(dungeonFloor->floorHeight/2)) - 1.0;
            ty = 1.0 - (pow(tx,2.0) + pow(tz, 2.0))/2.0;
            // Offset the ceiling
            int ceilHeight = (drawHeight) + floor(ty*16.0);
            ceilHeight += (int)(8.0*dungeonFloor->heightMap[x][y]);
            for(i = ceilHeight; i <= ceilHeight+5; i++){
               world[x][i][y] = CAVE_CEILING_ID;
               if(ceilHeight <= drawHeight + 1) 
                  dungeonFloor->floorData[x][y] = '#';
            }
         }
      }
      // Draw the floor
      for(y = 0; y < dungeonFloor->floorHeight; y++){
         for(x = 0; x < dungeonFloor->floorWidth; x++){
            world[x][drawHeight][y] = CAVE_FLOOR_ID;
         }
      }
      // Track MobIDs and ItemIDs
      int mobID = 0;
      int itemID = 0;
      // If we've been here before reload all active mob and item meshes
      if(!newFloor){
         for(mobID = 0; mobID < dungeonFloor->mobCount; mobID++){
            if(dungeonFloor->mobs[mobID].is_active){
               float mobx, moby, mobz;
               int toLoad;
               mobx = dungeonFloor->mobs[mobID].worldX;
               moby = dungeonFloor->mobs[mobID].worldY;
               mobz = dungeonFloor->mobs[mobID].worldZ;
               switch(dungeonFloor->mobs[mobID].symbol){
                  case 'F': // We should only have fish in the cave
                     toLoad = 1;
                     break;
                  default:
                     toLoad = 0; // Load the cow in event of an error
                     break;
               }
               setMeshID(mobID, toLoad, mobx, moby, mobz);
               setScaleMesh(mobID, 0.25);
            }
         }
         for(itemID = 0; itemID < dungeonFloor->itemCount; itemID++){
            if(dungeonFloor->items[itemID].is_active){
               float itemx, itemy, itemz;
               int toLoad;
               itemx = dungeonFloor->items[itemID].worldX;
               itemy = dungeonFloor->items[itemID].worldY;
               itemz = dungeonFloor->items[itemID].worldZ;
               switch(dungeonFloor->items[itemID].symbol){
                  case 'O':
                     // Open Chest
                     toLoad = 5;
                     break;
                  case 'A':
                     // Armour
                     toLoad = 6;
                     break;
                  case 'S':
                     // Sword
                     toLoad = 4;
                     break;
                  case 'K':
                     // Key
                     toLoad = 8;
                     break;
                  case '*':
                     // Coin
                     toLoad = 11;
                     break;
                  case '}':
                     // Bow
                     toLoad = 14;
                     break;
                  default:
                     toLoad = 17; // Load the skull in event of an error
                     break;
               }
               int meshID = dungeonFloor->items[itemID].meshID;
               setMeshID(meshID, toLoad, itemx, itemy, itemz);
               setScaleMesh(meshID, 0.5);
            }
         }
      }
      for(y = 0; y < dungeonFloor->floorHeight; y++){
         for(x = 0; x < dungeonFloor->floorWidth; x++){
            char entity = dungeonFloor->floorEntities[x][y];
            // Player found! Setup at coordinates
            if(entity=='@'){
               if(DEBUG==0)
                  printf("Setting player 0 at (%d, %d, %d)...\n", x, drawHeight+1, y);
               // Setup viewport
               setOldViewPosition(-x - 0.5, -drawHeight - 2, -y - 0.5);
               setViewPosition(-x - 0.5, -drawHeight - 2, -y - 0.5);
               setViewOrientation(0, 0, 0);
               // Wipe player reference point so it can be saved when they move to a different staircase
               dungeonFloor->floorEntities[x][y] = ' ';
            // Box found!
            } else if(entity=='$'){
               world[x][drawHeight+1][y] = BOX_ID; // Draw a box
            } else if(entity=='U'){
               world[x][drawHeight+1][y] = USTAIRS_ID; // Draw a upward staircase
            } else if(entity=='D'){
               world[x][drawHeight+1][y] = DSTAIRS_ID; // Draw a downward staircase
            // Load up mobs only for a new floor (Otherwise restored earlier from mob list)
            } else if((entity=='C' || entity=='B' || entity=='F') && newFloor){
               int toLoad;
               switch(entity){
                  case 'F': // We should only have fish in the cave
                     toLoad = 1;
                     break;
                  default:
                     toLoad = 0; // Load the cow in event of an error
                     break;
               }
               // Load up a mob (mesh) at this location!
               float mobx, moby, mobz;
               mobx = x + 0.5;
               moby = drawHeight + 1.5;
               mobz = y + 0.5;
               setMeshID(mobID, toLoad, mobx, moby, mobz);
               setScaleMesh(mobID, 0.25);
               // Save mob info to mob list
               dungeonFloor->mobs[mobID].worldX = mobx;
               dungeonFloor->mobs[mobID].worldY = moby;
               dungeonFloor->mobs[mobID].worldZ = mobz;

               dungeonFloor->mobs[mobID].facing = NORTH;
               dungeonFloor->mobs[mobID].rotX = 0.0;
               dungeonFloor->mobs[mobID].rotY = 0.0;
               dungeonFloor->mobs[mobID].rotZ = 0.0;

               // Mob starts out not moving
               dungeonFloor->mobs[mobID].is_moving = false;
               dungeonFloor->mobs[mobID].my_turn = false;
               dungeonFloor->mobs[mobID].is_aggro = false;
               dungeonFloor->mobs[mobID].state = IDLE;
               dungeonFloor->mobs[mobID].my_path = NULL; // Start w/ no path

               dungeonFloor->mobs[mobID].location.x = x;
               dungeonFloor->mobs[mobID].location.y = y;
               dungeonFloor->mobs[mobID].symbol = entity;
               dungeonFloor->mobs[mobID].is_active = true;
               // Wipe entity reference point (Similar to player) so we can draw float points to the map directly (Save on floor change)
               dungeonFloor->floorEntities[x][y] = ' ';
               // Cycle ID forward
               mobID++;
            // Load up items only for a new floor (Otherwise restored earlier from mob list)
            } else if((entity=='O' || entity=='A' || entity=='S' || entity=='K' || entity=='}' || entity=='*') && newFloor){
               int toLoad;
               switch(entity){
                  case 'O':
                     // Open Chest
                     toLoad = 5;
                     break;
                  case 'A':
                     // Armour
                     toLoad = 6;
                     break;
                  case 'S':
                     // Sword
                     toLoad = 4;
                     break;
                  case 'K':
                     // Key
                     toLoad = 8;
                     break;
                  case '*':
                     // Coin
                     toLoad = 11;
                     break;
                  case '}':
                     // Bow
                     toLoad = 14;
                     break;
                  default:
                     toLoad = 17; // Load the skull in event of an error
                     break;
               }
               // Load up a item (mesh) at this location!
               float itemx, itemy, itemz;
               itemx = x + 0.5;
               itemy = drawHeight + 1.5;
               itemz = y + 0.5;
               int meshID = itemID + dungeonFloor->mobCount;
               setMeshID(meshID, toLoad, itemx, itemy, itemz);
               setScaleMesh(meshID, 0.5);
               // Save mob info to mob list
               dungeonFloor->items[itemID].meshID = meshID;
               dungeonFloor->items[itemID].worldX = itemx;
               dungeonFloor->items[itemID].worldY = itemy;
               dungeonFloor->items[itemID].worldZ = itemz;

               dungeonFloor->items[itemID].rotX = 0.0;
               dungeonFloor->items[itemID].rotY = 0.0;
               dungeonFloor->items[itemID].rotZ = 0.0;

               dungeonFloor->items[itemID].location.x = x;
               dungeonFloor->items[itemID].location.y = y;
               dungeonFloor->items[itemID].symbol = entity;
               dungeonFloor->items[itemID].is_active = true;
               // Wipe entity reference point (Similar to player) so we can draw float points to the map directly (Save on floor change)
               dungeonFloor->floorEntities[x][y] = ' ';
               // Cycle ID forward
               itemID++;
            }
         }
      }

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

      // Track MobIDs
      int mobID = 0;
      int itemID = 0;
      // If we've been here before reload all active mob meshes
      if(!newFloor){
         for(mobID = 0; mobID < dungeonFloor->mobCount; mobID++){
            if(dungeonFloor->mobs[mobID].is_active){
               float mobx, moby, mobz;
               int toLoad;
               mobx = dungeonFloor->mobs[mobID].worldX;
               moby = dungeonFloor->mobs[mobID].worldY;
               mobz = dungeonFloor->mobs[mobID].worldZ;
               switch(dungeonFloor->mobs[mobID].symbol){
                  case 'C':
                     toLoad = 3;
                     break;
                  case 'B':
                     toLoad = 2;
                     break;
                  case 'F':
                     toLoad = 1;
                     break;
                  default:
                     toLoad = 0; // Load the cow in event of an error
                     break;
               }
               setMeshID(mobID, toLoad, mobx, moby, mobz);
               setScaleMesh(mobID, 0.25);
            }
         }
         for(itemID = 0; itemID < dungeonFloor->itemCount; itemID++){
            if(dungeonFloor->items[itemID].is_active){
               float itemx, itemy, itemz;
               int toLoad;
               itemx = dungeonFloor->items[itemID].worldX;
               itemy = dungeonFloor->items[itemID].worldY;
               itemz = dungeonFloor->items[itemID].worldZ;
               switch(dungeonFloor->items[itemID].symbol){
                  case 'O':
                     // Open Chest
                     toLoad = 5;
                     break;
                  case 'A':
                     // Armour
                     toLoad = 6;
                     break;
                  case 'S':
                     // Sword
                     toLoad = 4;
                     break;
                  case 'K':
                     // Key
                     toLoad = 8;
                     break;
                  case '*':
                     // Coin
                     toLoad = 11;
                     break;
                  case '}':
                     // Bow
                     toLoad = 14;
                     break;
                  default:
                     toLoad = 17; // Load the skull in event of an error
                     break;
               }
               int meshID = dungeonFloor->items[itemID].meshID;
               setMeshID(meshID, toLoad, itemx, itemy, itemz);
               setScaleMesh(meshID, 0.5);
            }
         }
      }
      // Next iterate over the entity list
      for(y = 0; y < dungeonFloor->floorHeight; y++){
         for(x = 0; x < dungeonFloor->floorWidth; x++){
            char entity = dungeonFloor->floorEntities[x][y];
            // Player found! Setup at coordinates
            if(entity=='@'){
               if(DEBUG==0)
                  printf("Setting player 0 at (%d, %d, %d)...\n", x, drawHeight+1, y);
               // Setup viewport
               setOldViewPosition(-x - 0.5, -drawHeight - 2, -y - 0.5);
               setViewPosition(-x - 0.5, -drawHeight - 2, -y - 0.5);
               setViewOrientation(0, 0, 0);
               // Wipe player reference point so it can be saved when they move to a different staircase
               dungeonFloor->floorEntities[x][y] = ' ';
            // Box found!
            } else if(entity=='$'){
               world[x][drawHeight+1][y] = BOX_ID; // Draw a box
            } else if(entity=='U'){
               world[x][drawHeight+1][y] = USTAIRS_ID; // Draw a upward staircase
            } else if(entity=='D'){
               world[x][drawHeight+1][y] = DSTAIRS_ID; // Draw a downward staircase
            // Load up mobs only for a new floor (Otherwise restored earlier from mob list)
            } else if((entity=='C' || entity=='B' || entity=='F') && newFloor){
               int toLoad;
               switch(entity){
                  case 'C':
                     toLoad = 3;
                     break;
                  case 'B':
                     toLoad = 2;
                     break;
                  case 'F':
                     toLoad = 1;
                     break;
                  default:
                     toLoad = 0; // Load the cow in event of an error
                     break;
               }
               // Load up a mob (mesh) at this location!
               float mobx, moby, mobz;
               mobx = x + 0.5;
               moby = drawHeight + 1.5;
               mobz = y + 0.5;
               setMeshID(mobID, toLoad, mobx, moby, mobz);
               setScaleMesh(mobID, 0.25);
               // Save mob info to mob list
               dungeonFloor->mobs[mobID].worldX = mobx;
               dungeonFloor->mobs[mobID].worldY = moby;
               dungeonFloor->mobs[mobID].worldZ = mobz;

               dungeonFloor->mobs[mobID].facing = NORTH;
               dungeonFloor->mobs[mobID].rotX = 0.0;
               dungeonFloor->mobs[mobID].rotY = 0.0;
               dungeonFloor->mobs[mobID].rotZ = 0.0;

               // Mob starts out not moving
               dungeonFloor->mobs[mobID].is_moving = false;
               dungeonFloor->mobs[mobID].my_turn = false;
               dungeonFloor->mobs[mobID].is_aggro = false;
               dungeonFloor->mobs[mobID].state = IDLE;
               dungeonFloor->mobs[mobID].my_path = NULL; // Start w/ no path

               dungeonFloor->mobs[mobID].location.x = x;
               dungeonFloor->mobs[mobID].location.y = y;
               dungeonFloor->mobs[mobID].symbol = entity;
               dungeonFloor->mobs[mobID].is_active = true;
               // Wipe entity reference point (Similar to player) so we can draw float points to the map directly (Save on floor change)
               dungeonFloor->floorEntities[x][y] = ' ';
               // Cycle ID forward
               mobID++;
            // Load up items only for a new floor (Otherwise restored earlier from mob list)
            } else if((entity=='O' || entity=='A' || entity=='S' || entity=='K' || entity=='}' || entity=='*') && newFloor){
               int toLoad;
               switch(entity){
                  case 'O':
                     // Open Chest
                     toLoad = 5;
                     break;
                  case 'A':
                     // Armour
                     toLoad = 6;
                     break;
                  case 'S':
                     // Sword
                     toLoad = 4;
                     break;
                  case 'K':
                     // Key
                     toLoad = 8;
                     break;
                  case '*':
                     // Coin
                     toLoad = 11;
                     break;
                  case '}':
                     // Bow
                     toLoad = 14;
                     break;
                  default:
                     toLoad = 17; // Load the skull in event of an error
                     break;
               }
               // Load up a item (mesh) at this location!
               float itemx, itemy, itemz;
               itemx = x + 0.5;
               itemy = drawHeight + 1.5;
               itemz = y + 0.5;
               int meshID = itemID + dungeonFloor->mobCount;
               setMeshID(meshID, toLoad, itemx, itemy, itemz);
               setScaleMesh(meshID, 0.5);
               // Save mob info to mob list
               dungeonFloor->items[itemID].worldX = itemx;
               dungeonFloor->items[itemID].worldY = itemy;
               dungeonFloor->items[itemID].worldZ = itemz;

               dungeonFloor->items[itemID].rotX = 0.0;
               dungeonFloor->items[itemID].rotY = 0.0;
               dungeonFloor->items[itemID].rotZ = 0.0;

               dungeonFloor->items[itemID].location.x = x;
               dungeonFloor->items[itemID].location.y = y;
               dungeonFloor->items[itemID].symbol = entity;
               dungeonFloor->items[itemID].is_active = true;
               dungeonFloor->items[itemID].meshID = meshID;
               // Wipe entity reference point (Similar to player) so we can draw float points to the map directly (Save on floor change)
               dungeonFloor->floorEntities[x][y] = ' ';
               // Cycle ID forward
               itemID++;
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
      // Lock Check
      if(world[(int)x][(int)(y-1)][(int)z] == DSTAIRS_ID && levelStack.floors[levelStack.currentFloor]->stairLocked){
         if(levelStack.floors[levelStack.currentFloor]->hasKey){
            levelStack.floors[levelStack.currentFloor]->hasKey = false;
            levelStack.floors[levelStack.currentFloor]->stairLocked = false;
         }
      }
      // Check for stairs
      if(world[(int)x][(int)(y-1)][(int)z] == DSTAIRS_ID && !levelStack.floors[levelStack.currentFloor]->stairLocked){
         if(DEBUG == 0){
            printf("Going downstairs!\n");
         }
         // Save player position
         if(levelStack.floors[levelStack.currentFloor]->floorType==OUTSIDE){
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
         if(!levelStack.floors[levelStack.currentFloor]->floorType==OUTSIDE){
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
      if(levelStack.floors[levelStack.currentFloor]->floorType==OUTSIDE){
         // Perform climb check
         if(world[(int)nX][(int)nY][(int)nZ] == 0){
            // Climb the box
            setViewPosition(-nX, -nY - 1, -nZ);
         // Perform slide check
         } else {
            // Check if we've hit a wall (Slide check)
            if(hit != 0 && world[(int)nX][(int)nY + 1][(int)nZ] != 0){
               // Find out which direction won't put us in the wall (X or Z)
               if(world[(int)nX][(int)nY][(int)oZ] != 0 && world[(int)oX][(int)oY][(int)z] == 0){
                  setViewPosition(-oX, -oY, -z);
               } else if(world[(int)oX][(int)nY][(int)nZ] != 0 && world[(int)x][(int)oY][(int)oZ] == 0){
                  setViewPosition(-x, -oY, -oZ);
               } else {
                  setViewPosition(-oX, -oY, -oZ);
               }
            }
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
            if(world[(int)nX][(int)nY + 1][(int)nZ] == 0 && world[(int)nX][(int)nY][(int)nZ] == BOX_ID){
               // Climb the box
               setViewPosition(-nX, -nY - 1, -nZ);
            // Perform slide check
            } else {
               // Check if we've hit a wall (Slide check)
               if(hit != 0 && world[(int)nX][(int)nY + 1][(int)nZ] != 0){
                  // Find out which direction won't put us in the wall (X or Z)
                  if(world[(int)nX][(int)nY][(int)oZ] != 0 && world[(int)x][(int)y][(int)z] == 0 && world[(int)oX][(int)oY][(int)z] == 0){
                     setViewPosition(-oX, -oY, -z);
                  } else if(world[(int)oX][(int)nY][(int)nZ] != 0 && world[(int)x][(int)y][(int)z] == 0 && world[(int)x][(int)oY][(int)oZ] == 0){
                     setViewPosition(-x, -oY, -oZ);
                  } else {
                     setViewPosition(-oX, -oY, -oZ);
                  }
               }
            }
         }
      }
   }
   
   // Check if we're running into any mobs
   // Get a reference to the mob list
   struct mob* list = levelStack.floors[levelStack.currentFloor]->mobs;
   // Get size of list
   int listSize = levelStack.floors[levelStack.currentFloor]->mobCount;
   // Track current id
   int id;
   // Iterate over all mobs
   for(id = 0; id < listSize; id++){
      if(!list[id].is_active){
         continue;
      }
      if((int)nX == list[id].location.x && (int)nZ == list[id].location.y){
         // "Combat"
         int chance = randRange(0, 1);
         if(chance){
            printf("Player hit mob %d! It has died.\n", id);
            list[id].is_active = false;
            list[id].is_visible = false;
            levelStack.floors[levelStack.currentFloor]->floorEntities[list[id].location.x][list[id].location.y] = ' ';
            unsetMeshID(id);
         } else {
            printf("Player swung at mob %d and missed!\n", id);
            // Reset position 
            setViewPosition(-oX, -oY, -oZ);
         }
         // Flag mobs to take their turn
         signalMobTurn();
      }
   }

   // Check if we're running into any items
   // Get a reference to the item list
   struct item* itemList = levelStack.floors[levelStack.currentFloor]->items;
   // Get size of list
   listSize = levelStack.floors[levelStack.currentFloor]->itemCount;
   // Iterate over all items
   for(id = 0; id < listSize; id++){
      if(!itemList[id].is_active){
         continue;
      }
      if((int)nX == itemList[id].location.x && (int)nZ == itemList[id].location.y){
         // Check what item we're picking up
         switch(itemList[id].symbol){
            case 'O':
               printf("Congratulations! You've found a box of gold!\n");
               break;
            case 'A':
               hasArmour = true;
               break;
            case 'S':
               hasSword = true;
               break;
            case 'K':
               levelStack.floors[levelStack.currentFloor]->hasKey = true;
               break;
            case '*':
               printf("Congratulations! You've found a gigantic gold coin!\n");
               break;
            case '}':
               hasBow = true;
               break;
            default:
               fprintf(stderr, "ERROR: Attempting to pick up unknown item %c!\n", itemList[id].symbol);
               break;
         }
         // Regardless of what was picked up, it is now 'inactive'
         itemList[id].is_active = false;
         unsetMeshID(itemList[id].meshID);
      }
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
   // Set colours
   GLfloat green[] = {0.0, 0.6, 0.0, 0.25};
   GLfloat darkgreen[] = {0.0, 0.3, 0.0, 0.5};
   GLfloat brown[] = {0.8, 0.4, 0.0, 0.5};
   GLfloat darkbrown[] = {0.4, 0.2, 0.0, 0.5};
   GLfloat cyan[] = {0.00, 0.72, 0.92, 0.75}; // Bow
   GLfloat yellow[] = {0.5, 0.5, 0.0, 0.5};
   GLfloat gold[] = {1.0, 0.85, 0.0, 0.75}; // Key
   GLfloat blue[] = {0.0, 0.0, 0.6, 0.75};
   GLfloat darkblue[] = {0.0, 0.0, 0.2, 0.33};
   GLfloat red[] = {0.5, 0.0, 0.0, 0.75};
   GLfloat lightred[] = {0.5, 0.0, 0.0, 0.25};
   GLfloat pink[] = {1.0, 0.40, 0.79, 0.75}; 
   GLfloat lightpink[] = {1.0, 0.40, 0.79, 0.25}; 
   GLfloat purple[] = {0.5, 0.1, 1.0, 0.75};
   GLfloat lightpurple[] = {0.5, 0.1, 1.0, 0.25};
   GLfloat white[] = {1.0, 1.0, 1.0, 0.5};
   GLfloat grey[] = {0.5, 0.5, 0.5, 0.5};
   GLfloat black[] = {0.0, 0.0, 0.0, 0.5};

   GLfloat airforceblue[] = {0.00, 0.19, 0.56, 1.0}; // Armour icon
   GLfloat steelteal[] = {0.37, 0.54, 0.55, 1.0}; // Sword Icon
   GLfloat amber[] = {1.00, 0.75, 0.0, 1.0}; // Key Icon

   if (testWorld) {
		/* draw some sample 2d shapes */
      if (displayMap == 1) {
         set2Dcolour(green);
         draw2Dline(0, 0, 500, 500, 15);
         draw2Dtriangle(0, 0, 200, 200, 0, 200);

         set2Dcolour(black);
         draw2Dbox(500, 380, 524, 388);
      }
   } else {
	/* your code goes here */
      int width = levelStack.floors[levelStack.currentFloor]->floorWidth;
      int height = levelStack.floors[levelStack.currentFloor]->floorHeight;
      int floorType = levelStack.floors[levelStack.currentFloor]->floorType;
      char** data = levelStack.floors[levelStack.currentFloor]->floorData;
      char** entities = levelStack.floors[levelStack.currentFloor]->floorEntities;
      struct mob* mobs = levelStack.floors[levelStack.currentFloor]->mobs;
      struct item* items = levelStack.floors[levelStack.currentFloor]->items;
      int numMobs = levelStack.floors[levelStack.currentFloor]->mobCount;
      int numItems = levelStack.floors[levelStack.currentFloor]->itemCount;
      int x, y, id;

      // Calculate offsets
      int xStep = screenWidth / width;
      int yStep = screenHeight / height;

      // Draw Icons
      if(hasSword){
         set2Dcolour(steelteal);
         draw2Dbox(22, 48, 26, 54);
         draw2Dbox(16, 54, 32, 60);
         draw2Dbox(20, 60, 28, 76);
         draw2Dtriangle(20, 76, 24, 80, 28, 76);
      }
      if(hasArmour){
         set2Dcolour(airforceblue);
         draw2Dbox(8, 8, 40, 32);
         draw2Dbox(8, 32, 20, 40);
         draw2Dbox(28, 32, 40, 40);
      }
      if(levelStack.floors[levelStack.currentFloor]->hasKey){
         set2Dcolour(amber);
         draw2Dbox(screenWidth - 8, 24, screenWidth - 24, 40);
         draw2Dbox(screenWidth - 14, 8, screenWidth - 18, 24);
         draw2Dbox(screenWidth - 18, 8, screenWidth - 22, 12);
         draw2Dbox(screenWidth - 18, 16, screenWidth - 22, 20);
      }

      // Check if map is going to be rendered or if we can skip
      if(displayMap == 0){
         return;
      }

      // Draw Player (And if outdoors, stairs)
      if(displayMap == 1 || displayMap == 2){
         set2Dcolour(blue);
         float px, py, pz;
         getViewPosition(&px, &py, &pz);
         px = -px;
         py = -py;
         pz = -pz;
         draw2Dbox((px - 0.5) * xStep, (pz - 0.5) * yStep, (px + 0.5) * xStep, (pz + 0.5) * yStep);
         // Draw green block and stairs down if outdoors
         if(floorType==OUTSIDE){
            // Draw stairs down
            set2Dcolour(black);
            float sx, sy, sz;
            sx = levelStack.floors[levelStack.currentFloor]->sx;
            sy = levelStack.floors[levelStack.currentFloor]->sy;
            sz = levelStack.floors[levelStack.currentFloor]->sz;
            draw2Dbox((sx - 0.5) * xStep, (sz - 0.5) * yStep, (sx + 0.5) * xStep, (sz + 0.5) * yStep);
            // Draw green over the whole map and return
            set2Dcolour(green);
            draw2Dbox(xStep, yStep, screenWidth - xStep, screenHeight - yStep);
            return;
         }

         // Draw our mobs
         for(id = 0; id < numMobs; id++){
            // Check if mob is dead
            if(!mobs[id].is_active){
               continue;
            }
            // Check if we're not rendering this mob (fog-of-war)
            if(!isVisible(mobs[id].location.x, mobs[id].location.y)){
               continue; // Skip to next mob
            }
            // Set colour based on mob type
            switch(mobs[id].symbol){
               case 'C': // Cactus
                  set2Dcolour(pink);
                  break; 
               case 'B': // Bat
                  set2Dcolour(red);
                  break;
               case 'F': // Fish
                  set2Dcolour(purple);
                  break;
               default: // ERROR
                  set2Dcolour(grey);
                  break;
            }
            // Get mobs location
            float mx, my, mz;
            mx = mobs[id].worldX;
            my = mobs[id].worldY;
            mz = mobs[id].worldZ;
            // Draw at mob location
            draw2Dbox((mx - 0.5) * xStep, (mz - 0.5) * yStep, (mx + 0.5) * xStep, (mz + 0.5) * yStep);
         }

         // Draw our items
         for(id = 0; id < numItems; id++){
            // Check if item is active
            if(!items[id].is_active){
               continue;
            }
            // Check if we're not rendering this item (fog-of-war)
            if(!isVisible(items[id].location.x, items[id].location.y)){
               continue; // Skip to next mob
            }
            // Set colour based on mob type
            switch(items[id].symbol){
               case 'K':
                  // Key
                  set2Dcolour(gold);
                  break;
               case '}':
                  // Coin
                  set2Dcolour(cyan);
                  break;
               default:
                  continue; // This is a item we're not showing on the map
                  break;
            }
            // Get items location
            float ix, iy, iz;
            ix = items[id].worldX;
            iy = items[id].worldY;
            iz = items[id].worldZ;
            // Draw at item location
            draw2Dbox((ix - 0.5) * xStep, (iz - 0.5) * yStep, (ix + 0.5) * xStep, (iz + 0.5) * yStep);
         }

         // Iterate over the whole map
         for(y = 0; y < height; y++){
            for(x = 0; x < width; x++){
               // If the space has something to draw and is visible
               if((data[x][y] != ' ' || entities[x][y] != ' ') && isVisible(x,y)){
                  // Check for all items of interest
                  // Walls
                  if(data[x][y] == '#'){
                     set2Dcolour(darkgreen);
                  // Closed Doors
                  } else if(data[x][y] == '/'){
                     set2Dcolour(darkbrown);
                  // Open Doors
                  } else if(data[x][y] == '|'){
                     set2Dcolour(brown);
                  // Upstairs
                  } else if(entities[x][y] == 'U'){
                     set2Dcolour(white);
                  // Downstairs
                  } else if(entities[x][y] == 'D'){
                     set2Dcolour(black);
                  // Open room area
                  } else {
                     set2Dcolour(green);
                  }
                  draw2Dbox(x * xStep, y * yStep, (x + 1) * xStep, (y + 1) * yStep);
               }
            }
         }
      }
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

      // Check if we're on level 0 (outdoors)
      if(levelStack.floors[levelStack.currentFloor]->floorType==OUTSIDE){
         // Animate clouds
         animateClouds(delta);
      } else if(levelStack.floors[levelStack.currentFloor]->floorType==CAVE){
         // Turn check
         turnCheck();
         // Update mobs
         mobUpdate(delta);
         // Update items
         itemUpdate(delta);
      } else if(levelStack.floors[levelStack.currentFloor]->floorType==DUNGEON){
         // Update visibility
         updateVisible((int)-x, (int)-z);
         // Turn check
         turnCheck();
         // Update mobs
         mobUpdate(delta);
         // Update items
         itemUpdate(delta);
      } else {
         fprintf(stderr, "ERROR: Unknown floor type %d!\n", levelStack.floors[levelStack.currentFloor]->floorType);
      }
   }
   return;
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
      // Set item flags
      hasSword = false;
      hasArmour = false;
      hasBow = false;

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
      setUserColour(CAVE_FLOOR_ID, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);
      setUserColour(CAVE_CEILING_ID, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);
      

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
      setAssignedTexture(CAVE_FLOOR_ID, CAVE_FLOOR_TEX);
      setAssignedTexture(CAVE_CEILING_ID, CAVE_CEILING_TEX);


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

