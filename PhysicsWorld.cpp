#include "BlockTowerGame.h"

void PhysicsWorld::constructTower()
{
	/* Add blocks to the world that form the tower */

	// Define block shape templates of slightly varying heights
	blockShape = new btBoxShape*[2];
	for (int i=0; i<2; i++)
		blockShape[i] = new btBoxShape(btVector3(3.75,0.75+0.01*i,1.25));

	blockRigidBody = new btRigidBody*[BLOCK_NO];

	// Define attributes for a block
	btScalar mass = 5.0f;
	btVector3 blockInertia(0,0,0);
	for (int i=0; i<2; i++)
		blockShape[i]->calculateLocalInertia(mass,blockInertia);
	btScalar friction = 1.0f;
	btScalar restitution = 0.0f;
	btScalar damping = 0.15f;

	// Add blocks in layers of three, with each layer at a right angle to neighbouring layers
	for (int i=0; i<BLOCK_NO; i+=6) {
		for (int j=0; j<std::min(3,BLOCK_NO-i); j++) {
			// Define initial transformation state of block
			btDefaultMotionState* blockMotionState =
				new btDefaultMotionState(btTransform(btQuaternion(btVector3(0,1,0), btScalar(0*PI/180)),btVector3(0,0.75f+i*0.5f,2.5f*(j-1))));
			// Gather construction information for block
			btRigidBody::btRigidBodyConstructionInfo blockRigidBodyCI(mass,blockMotionState,blockShape[rand()%2],blockInertia);
			blockRigidBodyCI.m_restitution = restitution;
			// Create block
			blockRigidBody[i+j] = new btRigidBody(blockRigidBodyCI);
			blockRigidBody[i+j]->setAnisotropicFriction(btVector3(friction, friction*2, friction*1.2));
			blockRigidBody[i+j]->setDamping(damping,damping*2);
			// Add block to world
			dynamicsWorld->addRigidBody(blockRigidBody[i+j]);
		}
		for (int j=3; j<std::min(6,BLOCK_NO-i); j++) {
			btDefaultMotionState* blockMotionState =
				new btDefaultMotionState(btTransform(btQuaternion(btVector3(0,1,0), btScalar(90*PI/180)),btVector3(2.5f*(j-4),2.25f+i*0.5f,0)));
			btRigidBody::btRigidBodyConstructionInfo blockRigidBodyCI(mass,blockMotionState,blockShape[rand()%2],blockInertia);
			blockRigidBodyCI.m_restitution = restitution;
			blockRigidBody[i+j] = new btRigidBody(blockRigidBodyCI);
			blockRigidBody[i+j]->setAnisotropicFriction(btVector3(friction, friction*1.2, friction*2));
			blockRigidBody[i+j]->setDamping(damping,damping*4);
			dynamicsWorld->addRigidBody(blockRigidBody[i+j]);
		}
	}
}


void PhysicsWorld::createWorld()
{
	// Build the broadphase
	broadphase = new btDbvtBroadphase();
	// Set up the collision configuration and dispatcher
	collisionConfiguration = new btDefaultCollisionConfiguration();
	dispatcher = new btCollisionDispatcher(collisionConfiguration);
	// Set up physics solver
	solver = new btSequentialImpulseConstraintSolver;

	// Create physics world
	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher,broadphase,solver,collisionConfiguration);
	dynamicsWorld->setGravity(btVector3(0,-12,0));

	// Create surface shape template
	surfaceShape = new btStaticPlaneShape(btVector3(0,1,0),1);

	// Set transformation state of static surface
	btDefaultMotionState* surfaceMotionState =
		new btDefaultMotionState(btTransform(btQuaternion(btVector3(0,0,1), btScalar(0)),btVector3(0,-1,0)));
	// Collect construction information for surface
	btRigidBody::btRigidBodyConstructionInfo
		surfaceRigidBodyCI(0,surfaceMotionState,surfaceShape,btVector3(0,0,0));
	surfaceRigidBodyCI.m_friction = 1.5;
	// Create surface
	surfaceRigidBody = new btRigidBody(surfaceRigidBodyCI);
	// Add surface to world
	dynamicsWorld->addRigidBody(surfaceRigidBody);

	constructTower();

	time = 0;	// Initialise timer - set proper value after first step
}


void PhysicsWorld::deleteWorld()
{
	/* Delete physics world and all variables */

	for (int i=0; i<BLOCK_NO; i++) {
		dynamicsWorld->removeRigidBody(blockRigidBody[i]);
		delete blockRigidBody[i]->getMotionState();
		delete blockRigidBody[i];
	}
	delete blockRigidBody;

	dynamicsWorld->removeRigidBody(surfaceRigidBody);
	delete surfaceRigidBody->getMotionState();
	delete surfaceRigidBody;

	delete blockShape;

	delete surfaceShape;

	delete dynamicsWorld;
	delete solver;
	delete collisionConfiguration;
	delete dispatcher;
	delete broadphase;
}


void PhysicsWorld::resetWorld()
{
	/* Delete and re-create physics world */

	deleteWorld();
	createWorld();
}


void PhysicsWorld::stepWorld(btTransform* boxTrans)
{
	/* Step the simulation by the amount of time passed since last step */
	if ( time == 0 )
		dynamicsWorld->stepSimulation(1/60.f, 10);		// First step
	else {
		time = clock() - time;	// Get time passed
		dynamicsWorld->stepSimulation(float(time)/float(CLOCKS_PER_SEC), 10);
	}
	time = clock();		// Restart timer for next step

	// Get current transformation states for all blocks
	for (int i=0; i<BLOCK_NO; i++)
		blockRigidBody[i]->getMotionState()->getWorldTransform(boxTrans[i]);
}


btVector3 PhysicsWorld::getBoxExtents()
{
	/* Get dimensions for block shape template */

	return blockShape[0]->getHalfExtentsWithMargin();
}


float PhysicsWorld::getSurfaceHeight()
{
	/* Get height of static surface */

	btTransform surfaceTrans;
	surfaceRigidBody->getMotionState()->getWorldTransform(surfaceTrans);

	return surfaceTrans.getOrigin().getY()+surfaceShape->getPlaneConstant();
}


boolean PhysicsWorld::isActive(int objectIndex)
{
	/* Check if block is active */

	if ( objectIndex >= 0 )
		return blockRigidBody[objectIndex]->isActive();
	else
		return 0;
}


boolean PhysicsWorld::checkContact(int objectIndex)
{
	/* Check if block is in contact with any other block */

	if ( objectIndex >= 0 ) {
		int numManifolds = dynamicsWorld->getDispatcher()->getNumManifolds();
		for (int i=0; i<numManifolds; i++) {
			btPersistentManifold* contactManifold = dynamicsWorld->getDispatcher()->getManifoldByIndexInternal(i);
			if ( ( contactManifold->getBody0() == blockRigidBody[objectIndex] ||
				contactManifold->getBody1() == blockRigidBody[objectIndex] ) &&
				!( contactManifold->getBody0() == surfaceRigidBody ||
				contactManifold->getBody1() == surfaceRigidBody ) &&
				contactManifold->getNumContacts() > 0
			) {
				return 1;
			}
		}
	}

	return 0;
}


void PhysicsWorld::pushObject(int objectIndex, double impulse, double* mouseRay)
{
	/* Apply central, horizontal impulse to block */

	if (objectIndex >= 0) {
		btVector3 boxOrigin = blockRigidBody[objectIndex]->getWorldTransform().getOrigin();
		blockRigidBody[objectIndex]->activate();
		blockRigidBody[objectIndex]->applyCentralImpulse(
			btVector3(
				mouseRay[0]-boxOrigin.getX(),
				0,
				mouseRay[2]-boxOrigin.getZ()
			).normalized()*impulse
		);
	}
}


void PhysicsWorld::turnObject(int objectIndex, double impulse)
{
	/* Apply torque to block */

	if (objectIndex >= 0) {
		blockRigidBody[objectIndex]->activate();
		blockRigidBody[objectIndex]->applyTorqueImpulse(btVector3(0,impulse,0));
	}
}


void PhysicsWorld::dragObject(int objectIndex, double* mouseRay, double* objectSelect)
{
	/* Set linear velocity of block to move towards mouse target */

	if (objectIndex >= 0) {
		btVector3 boxOrigin = blockRigidBody[objectIndex]->getWorldTransform().getOrigin();
		btVector3 boxRotateAxis = blockRigidBody[objectIndex]->getWorldTransform().getRotation().getAxis();
		blockRigidBody[objectIndex]->activate();
		blockRigidBody[objectIndex]->setLinearVelocity(
			btVector3(
				(btScalar)std::min(100.0,mouseRay[0]-boxOrigin.getX()-objectSelect[0]),
				(btScalar)std::min(100.0,mouseRay[1]-boxOrigin.getY()-objectSelect[1]),
				(btScalar)std::min(100.0,mouseRay[2]-boxOrigin.getZ()-objectSelect[2])
			)*3
		);
		// Set angular velocity such that block is turned upright
		blockRigidBody[objectIndex]->setAngularVelocity(
				btVector3(
					-boxRotateAxis.getX(),
					blockRigidBody[objectIndex]->getAngularVelocity().getY()/2,
					-boxRotateAxis.getZ()
				)
		);
	}
}


void PhysicsWorld::raiseObjectTo(int objectIndex, double height)
{
	/* Set linear velocity of block towards given height */

	if ( objectIndex >= 0 ) {
		btVector3 boxOrigin = blockRigidBody[objectIndex]->getWorldTransform().getOrigin();
		blockRigidBody[objectIndex]->activate();
		blockRigidBody[objectIndex]->
			setLinearVelocity(btVector3(0,(height-boxOrigin.getY())*5,0));
		// Nullify angular velocity
		blockRigidBody[objectIndex]->setAngularVelocity(btVector3(0,0,0));
	}
}


void PhysicsWorld::stopObject(int objectIndex)
{
	/* Cancel all block velocity */

	if ( objectIndex >= 0 ) {
		blockRigidBody[objectIndex]->setLinearVelocity(btVector3(0,0,0));
		blockRigidBody[objectIndex]->setAngularVelocity(btVector3(0,0,0));
	}
}


void PhysicsWorld::centerObject(int objectIndex)
{
	/* Apply force to block, towards centre of world */

	if ( objectIndex >= 0 ) {
		btVector3 boxOrigin = blockRigidBody[objectIndex]->getWorldTransform().getOrigin();
		btVector3 boxVelocity = blockRigidBody[objectIndex]->getLinearVelocity();
		blockRigidBody[objectIndex]->
			applyCentralForce(btVector3(-boxOrigin.getX(), 0, -boxOrigin.getZ()));
	}
}
