class PhysicsWorld
{
	// Settings for calculating physics
	btBroadphaseInterface* broadphase;
	btDefaultCollisionConfiguration* collisionConfiguration;
	btCollisionDispatcher* dispatcher;
	btSequentialImpulseConstraintSolver* solver;

	btDiscreteDynamicsWorld* dynamicsWorld;		// The physics world

	btStaticPlaneShape* surfaceShape;	// Surface shape template
	btBoxShape** blockShape;			// Array for block shape templates
	btRigidBody* surfaceRigidBody;		// Static surface object
	btRigidBody** blockRigidBody;		// Array for Block object

	clock_t time;	// Timer for stepping in real-time

	void constructTower();

public:
	void createWorld();
	void deleteWorld();
	void resetWorld();
	void stepWorld(btTransform* trans);
	btVector3 getBoxExtents();
	float getSurfaceHeight();
	boolean isActive(int objectIndex);
	boolean checkContact(int objectIndex);
	void pushObject(int objectIndex, double impulse, double* mouseRay);
	void turnObject(int objectIndex, double impulse);
	void dragObject(int objectIndex, double* mouseRay, double* objectSelect);
	void raiseObjectTo(int objectIndex, double height);
	void stopObject(int objectIndex);
	void centerObject(int objectIndex);
};