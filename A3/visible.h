
/*
 * A header to allow me external access to the functions used in visible.c
 * See visible.c (Provided by prof) for sources and information
 */

float lengthTwoPoints(float x1, float y1, float z1, float x2, float y2, float z2);

float lengthVector(float x1, float y1, float z1);

void cross(float x1, float y1, float z1, float x2, float y2, float z2,
	float *x, float *y, float *z);

	/* returns radians */
float dot (float x1, float y1, float z1, float x2, float y2, float z2);

	/* the next two function use Cramer's rule to find the intersection */
	/* of three planes */
	/* used to find outer points of frustum */
	/* http://www.dreamincode.net/code/snippet530.htm */
double finddet(double a1,double a2, double a3,double b1, double b2,double b3, double c1, double c2, double c3);

void intersect(float a1, float b1, float c1, float d1, 
   float a2, float b2, float c2, float d2, 
   float a3, float b3, float c3, float d3,
   float *x, float *y, float *z);

/***********************/

void ExtractFrustum();

int PointInFrustum( float x, float y, float z );

int CubeInFrustum( float x, float y, float z, float size );


int CubeInFrustum2( float x, float y, float z, float size );



/*****/



// if frustum test shows box in view
//    if level == max level then draw contents of cube
//    else call 8 subdivisions, increment level
// assumes all t[xyz] are larger than b[xyz] respectively

void tree(float bx, float by, float bz, float tx, float ty, float tz,
   int level);


        /* determines which cubes are to be drawn and puts them into */
        /* the displayList  */
        /* write your cube culling code here */
void buildDisplayList();
