#ifndef __bounding_interval_hierarchy_h
#define __bounding_interval_hierarchy_h

// Bounding Interval Hierarchy
// EuroGraphics Symposium on Rendering, 2006
//
// Carsten Wächter and Alexander Keller
//
// http://ainc.de/Research/BIH.pdf

#include "AACell.h"
#include "List.h"
#include "TypeDefs.h"
#include "Vectors.h"

#define BIH_NODE_X_AXIS		0
#define BIH_NODE_Y_AXIS		1
#define BIH_NODE_Z_AXIS		2

struct _BIHLeaf
{
	// 8B
	ui32 firstObjectId;
	ui32 objectCount;
};

struct _BIHNode
{
	// 8B
	f32 leftPlane;
	f32 rightPlane;

	// 8B
	ui32 leftId;
	ui32 righId;

	// 4B
	uint32 axis			: 2;
	uint32 leftLeaf		: 1;
	uint32 rightLeaf	: 1;
	uint32 reserved		: 28; // reserved, because of 4B padding
};

// TODO rozdelit BIHNode na Node a Leaf
#pragma pack(push, 4)
struct BIHNode
{
	uint32 axis		: 2;
	uint32 isLeaf	: 1;
	uint32 reserved : 29; // reserved, because of 4B padding

	union
	{
		// leaf
		struct
		{
			ui32 firstObjectId;
			ui32 objectCount;
			
			uint64 _leaf_reserved; // (because node is bigger than leaf, can hold additional data about leaf)
#ifdef REAL_AS_DOUBLE
			uint64 _leaf_reserved1;
#endif
		};

		// node
		struct
		{
			ui32 leftNodeId;
			ui32 rightNodeId;

			real leftPlane;
			real rightPlane;
		};
	};

	void Clear()
	{
		leftNodeId = rightNodeId = 0;
		isLeaf = 0;
	}
};
#pragma pack(pop)

DLL_EXPORT_ARRAY_OF(BIHNode);
DLL_EXPORT_LIST_OF(BIHNode);


struct AthenaStorage;
struct HitResult;
struct ObjectId;
struct Ray;
struct Objects;

class DLL_EXPORT BIH
{
public:

	void Initialize(Objects* objects, MemoryManager* memoryManagerInstance);
	bool Update(uint maxObjectsPerLeaf, uint maxDepth, AthenaStorage* athenaStorage);
	void Destroy();

	void Hit(const Ray& ray, HitResult& hitResult) const;
	bool Collide(const Ray& ray, 
		real from = EPSILON, real to = _INFINITY, const ObjectId* objectIdToSkip = null) const;

	inline const uint GetCurrentDepth() const { return currentDepth; }
	inline const uint GetCurrentNodeCount() const { return currentNumOfLeaves + currentNumOfNodes; }

private:

	void ShowStats();

	void UpdateRootCell();
	bool GetObjectInfoById(const ObjectId& objectId, AACell& objectCell, v3f& objectPosition) const;

	void HitNode(const Ray& ray, const BIHNode& parentNode, const AACell& nodeCell, HitResult& hitResult) const;
	void HitLeaf(const Ray& ray, uint firstObjectId, uint objectCount, HitResult& hitResult) const;
	
	bool CollideNode(const Ray& ray, const BIHNode& parentNode, const AACell& parentNodeCell,
		real from, real to, const ObjectId* objectIdToSkip) const;
	bool CollideLeaf(const Ray& ray, uint firstObjectId, uint objectCount,
		real from, real to, const ObjectId* objectIdToSkip) const;

	void CreateNode(uint32 nodeId, uint firstObjectId, uint objectCount, const AACell& cell, uint depth = 0);

	// sorts all unknown objects to left and returns number of these objects
	uint PreSortObjects();
	// sort objects by splitPlane to left/right and returns number of objects on left side
	uint SortObjects(uint firstObjectId, uint objectCount, real splitPlane, uint8 axis);

private:

	// statistics
	uint64 numOfNodesUsed, numOfLeavesUsed;
	uint64 numOfObjectsUsed;
	uint64 numOfTreesConstructed;
	uint64 depthSum;

	uint currentNumOfNodes, currentNumOfLeaves;
	uint currentDepth, currentMaxDepth, currentMinDepth;

	// properties
	uint maxObjectsPerLeaf;
	uint maxDepth;
	
	// main tree properties
	AACell rootCell;

	Objects* objects;
	list_of<BIHNode> nodes;
};

#endif __bounding_interval_hierarchy_h
