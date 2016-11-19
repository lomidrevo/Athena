#ifndef __octree_h
#define __octree_h

#include "AACell.h"
#include "Array.h"
#include "thread"
#include "TypeDefs.h"
#include "Vectors.h"

#define OCTREE_NODE_POSITION_VALUES(_) \
	_(x0y0z0, = 1) \
	_(x1y0z0, = 2) \
	_(x0y1z0, = 4) \
	_(x1y1z0, = 8) \
	_(x0y0z1, = 16) \
	_(x1y0z1, = 32) \
	_(x0y1z1, = 64) \
	_(x1y1z1, = 128)
DECLARE_ENUM(OctreeNodePosition, OCTREE_NODE_POSITION_VALUES)
#undef OCTREE_NODE_POSITION_VALUES


struct OctreeNode
{
	union
	{
		ui8 nodeMask;
		struct
		{
			ui8 nodeMask_x0y0z0 : 1;
			ui8 nodeMask_x1y0z0 : 1;
			ui8 nodeMask_x0y1z0 : 1;
			ui8 nodeMask_x1y1z0 : 1;
			ui8 nodeMask_x0y0z1 : 1;
			ui8 nodeMask_x1y0z1 : 1;
			ui8 nodeMask_x0y1z1 : 1;
			ui8 nodeMask_x1y1z1 : 1;
		};
	};
	ui8 childNodeCount;
	
	ui32 firstChildNodeId;

	inline b32 IsParentNode() const { return childNodeCount && !firstChildNodeId; }

	void Clear()
	{
		nodeMask = childNodeCount = 0;
		firstChildNodeId = 0;
	}
};

DLL_EXPORT_ARRAY_OF(OctreeNode);
DLL_EXPORT_LIST_OF(OctreeNode);
DLL_EXPORT_ARRAY_OF(list_of<OctreeNode>);


class MemoryManager;
struct Ray;
struct HitResult;
struct Objects;
struct AthenaStorage;

class DLL_EXPORT Octree
{
public:

	void Initialize(Objects* objects, MemoryManager* memoryManagerInstance);
	void Destroy(MemoryManager* memoryManagerInstance);
	bool Update(array_of<std::thread>& threads, ui32 maxDepth, AthenaStorage* athenaStorage);

	void Hit(const Ray& ray, HitResult& hitResult) const;
	bool Collide(const Ray& ray, real from = EPSILON, real to = _INFINITY) const;

	uint GetCurrentNodeCount() const { return currentNumOfNodesUsed; }

private:

	void HitNode(const Ray& ray, const list_of<OctreeNode>& nodes, ui32 nodeId, const AACell& nodeCell,
		HitResult& hitResult) const;
	bool CollideNode(const Ray& ray, const list_of<OctreeNode>& nodes, uint32 nodeId, const AACell& nodeCell,
		real from, real to) const;

	void ShowStats();

	void UpdateRootCell();

	static void ProcessNode(list_of<OctreeNode>& nodes, const Objects* objects, ui32* nodesUsed,
		const AACell& nodeCell, ui32 nodeId, ui32 maxDepth);
	
	static bool IsNodeEmpty(const Objects* objects, const AACell& nodeCell);
	ui64 GetTotalNodeCount(ui32 depth);

private:

	// statistics
	ui64 totalThreadsUsed;
	ui64 numOfNodesUsed;
	ui64 numOfEmptyNodes;

	ui64 numOfObjectsUsed;
	ui64 numOfTreesConstructed;
	ui64 depthSum;

	uint currentDepth, currentMaxDepth, currentMinDepth, currentNumOfNodesUsed;

	// main tree properties
	AACell rootCell;
	array_of<list_of<OctreeNode>> threadNodes;

	Objects* objects;
	static ui8 octreeNodePositions[8];
};

#endif __octree_h
