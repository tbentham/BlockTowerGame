#include <iostream>
#include <windows.h>
#include <stdio.h>
#include <cmath>
#include <ctime>
#include <GL/gl.h>					// OpenGL (Open Graphics Library)
#include <GL/glut.h>				// GLUT (GL Utility Kit)
#include <btBulletDynamicsCommon.h>	// Bullet physics simulation library
#include "Camera.h"
#include "PhysicsWorld.h"

#define PI 3.14159265				// Estimated value of pi, for converting angles
#define BLOCK_NO 54					// Number of blocks in the tower
