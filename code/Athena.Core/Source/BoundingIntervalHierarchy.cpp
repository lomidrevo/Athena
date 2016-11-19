#include "Athena.h"
#include "BoundingIntervalHierarchy.h"
#include "HitResult.h"
#include "Rendering.h"
#include "Scene.h"
#include "Timer.h"
#include "Timers.h"

#define NODE_MEM_POOL_PAGE_SIZE	1024


void BIH::Destroy()
{
	if (objects)
	{
		ShowStats();

		objects = null;
		nodes.Destroy();

		LOG_DEBUG("BIH::Destroy");
	}
}

void BIH::Initialize(Objects* objects, MemoryManager* memoryManagerInstance)
{
	numOfLeavesUsed = numOfNodesUsed = numOfObjectsUsed = depthSum = 0;
	numOfTreesConstructed = 0;
	currentNumOfLeaves = currentNumOfNodes = 0;
	currentDepth = currentMaxDepth = 0;
	currentMinDepth = 0;

	nodes.Initialize(memoryManagerInstance, "BIHNode", NODE_MEM_POOL_PAGE_SIZE);

	this->objects = objects;
}

bool BIH::Update(uint maxObjectsPerLeaf, uint maxDepth, AthenaStorage* athenaStorage)
{
	TIMED_BLOCK(&athenaStorage->timers[TimerId::BihConstruction]);

	// clear memory pool
	nodes.Clear();
	currentNumOfLeaves = currentNumOfNodes = 0;
	currentDepth = 0;

	if (athenaStorage->renderingParameters.tracingMethod != TracingMethod::BoundingIntervalHierarchy)
		return false;

	// min value is 1
	if (!maxObjectsPerLeaf)
		maxObjectsPerLeaf = 1;

	// set tree properties
	this->maxObjectsPerLeaf = maxObjectsPerLeaf;
	this->maxDepth = maxDepth;

	if (!objects->everything.currentCount || !maxDepth)
		return false;

	// get root bounding box
	UpdateRootCell();

	BIHNode rootNode;
	rootNode.Clear();

	const uint unknownObjectsCount = PreSortObjects();
	// recursively create tree
	CreateNode((ui32)nodes.Add(rootNode), unknownObjectsCount, objects->everything.currentCount - unknownObjectsCount, 
		rootCell);
	
	numOfTreesConstructed++;
	numOfNodesUsed += currentNumOfNodes;
	numOfLeavesUsed += currentNumOfLeaves;
	numOfObjectsUsed += objects->everything.currentCount;
	depthSum += currentDepth;

	if (currentDepth < currentMinDepth)
		currentMinDepth = currentDepth;

	if (currentDepth > currentMaxDepth)
		currentMaxDepth = currentDepth;

	return true;
}

template <typename T> void AddObjectListToCell(const list_of<T>& objectList, AACell& cell)
{
	for (uint32 i = 0; i < (uint32)objectList.currentCount; ++i)
	{
		const T& object = objectList[i];
		cell.minCorner.Set(
			MIN2(object.cell.minCorner.x + object.position.x, cell.minCorner.x),
			MIN2(object.cell.minCorner.y + object.position.y, cell.minCorner.y),
			MIN2(object.cell.minCorner.z + object.position.z, cell.minCorner.z));
		cell.maxCorner.Set(
			MAX2(object.cell.maxCorner.x + object.position.x, cell.maxCorner.x),
			MAX2(object.cell.maxCorner.y + object.position.y, cell.maxCorner.y),
			MAX2(object.cell.maxCorner.z + object.position.z, cell.maxCorner.z));
	}
}

void BIH::UpdateRootCell()
{
	rootCell.maxCorner.Set(-_INFINITY, -_INFINITY, -_INFINITY);
	rootCell.minCorner.Set(_INFINITY, _INFINITY, _INFINITY);

	AddObjectListToCell<Box>(objects->boxes, rootCell);
	AddObjectListToCell<Sphere>(objects->spheres, rootCell);
	AddObjectListToCell<SphereLightSource>(objects->sphereLights, rootCell);
	AddObjectListToCell<BoxLightSource>(objects->boxLights, rootCell);
	AddObjectListToCell<Mesh>(objects->meshes, rootCell);
}

void BIH::CreateNode(uint32 nodeId, uint firstObjectId, uint objectCount, const AACell& cell, uint depth)
{
	depth++;
	
	if (objectCount <= maxObjectsPerLeaf || depth == maxDepth)
	{
		BIHNode& node = nodes[nodeId];

		node.isLeaf = 1;
		node.firstObjectId = (uint32)firstObjectId;
		node.objectCount = (uint32)objectCount;

		currentNumOfLeaves++;

		if (depth > currentDepth)
			currentDepth = depth;

		return;
	}

	currentNumOfNodes++;

	v3f cellSize = cell.maxCorner - cell.minCorner;

	// find longest dimension
	nodes[nodeId].axis = BIH_NODE_X_AXIS;
	
	if (cellSize.y > cellSize.x)
		nodes[nodeId].axis = BIH_NODE_Y_AXIS;

	if (cellSize.z > cellSize[nodes[nodeId].axis])
		nodes[nodeId].axis = BIH_NODE_Z_AXIS;

	const real splitPlane = cellSize[nodes[nodeId].axis] * .5 + cell.minCorner.Get(nodes[nodeId].axis);

	// sort objects by split plane
	uint numOfObjectsOnLeft = SortObjects(firstObjectId, objectCount, splitPlane, nodes[nodeId].axis);

	nodes[nodeId].leftPlane = cell.minCorner.Get(nodes[nodeId].axis);
	nodes[nodeId].rightPlane = cell.maxCorner.Get(nodes[nodeId].axis);

	AACell objectCell;
	v3f objectPosition;

	// left interval
	if (numOfObjectsOnLeft)
	{
		// get left plane
		for (uint i = 0; i < numOfObjectsOnLeft; i++)
		{
			if (!GetObjectInfoById(objects->everything[firstObjectId + i], objectCell, objectPosition))
				continue;
			
			const real objectMaxPlane = objectCell.maxCorner[nodes[nodeId].axis] +
				objectPosition[nodes[nodeId].axis];

			if (objectMaxPlane > nodes[nodeId].leftPlane)
				nodes[nodeId].leftPlane = objectMaxPlane;
		}

		AACell leftCell = cell;
		leftCell.maxCorner[nodes[nodeId].axis] = nodes[nodeId].leftPlane;
		
		BIHNode leftNode;
		leftNode.Clear();
		//leftnode.parentNodeId = nodeId;

		nodes[nodeId].leftNodeId = (ui32)nodes.Add(leftNode);
		CreateNode(nodes[nodeId].leftNodeId, firstObjectId, numOfObjectsOnLeft, leftCell, depth);
	}

	// right interval
	if (objectCount - numOfObjectsOnLeft)
	{
		// get right plane
		for (uint i = numOfObjectsOnLeft; i < objectCount; i++)
		{
			if (!GetObjectInfoById(objects->everything[firstObjectId + i], objectCell, objectPosition))
				continue;

			const real objectMinPlane = objectCell.minCorner[nodes[nodeId].axis] +
				objectPosition[nodes[nodeId].axis];
			if (objectMinPlane < nodes[nodeId].rightPlane)
				nodes[nodeId].rightPlane = objectMinPlane;
		}

		AACell rightCell = cell;
		rightCell.minCorner[nodes[nodeId].axis] = nodes[nodeId].rightPlane;

		BIHNode rightNode;
		rightNode.Clear();
		//rightNode.parentNodeId = nodeId;

		nodes[nodeId].rightNodeId = (ui32)nodes.Add(rightNode);
		CreateNode(nodes[nodeId].rightNodeId, firstObjectId + numOfObjectsOnLeft, objectCount - numOfObjectsOnLeft,
			rightCell, depth);
	}
}

uint BIH::PreSortObjects()
{
	AACell objectCell;
	v3f objectPosition;
	uint lastUnknown = 0;
	for (uint i = 0; i < objects->everything.currentCount; ++i)
	{
		// if object is unknown
		if (!GetObjectInfoById(objects->everything[i], objectCell, objectPosition))
		{
			// find first known object from left
			for (; lastUnknown < i; ++lastUnknown)
			{
				if (GetObjectInfoById(objects->everything[lastUnknown], objectCell, objectPosition))
				{
					// swap objects, so unknown objects are from 0 to lastUnknown
					ObjectId tmpObjectId = objects->everything[lastUnknown];
					objects->everything[lastUnknown] = objects->everything[i];
					objects->everything[i] = tmpObjectId;
					// point lastUnknown to next object
					break;
				}
			}
			lastUnknown++;
		}
	}

	return lastUnknown;
}

uint BIH::SortObjects(uint firstObjectId, uint objectCount, real splitPlane, uint8 axis)
{
	int64 leftId = firstObjectId, rightId = firstObjectId + objectCount - 1;
	uint leftCount = 0;

	AACell leftObjectCell, rightObjectCell;
	v3f leftObjectPosition, rightObjectPosition;
	while (leftId <= rightId)
	{
		auto leftFound = GetObjectInfoById(objects->everything[leftId], leftObjectCell, leftObjectPosition);
		ASSERT(leftFound);
		auto rightFound = GetObjectInfoById(objects->everything[rightId], rightObjectCell, rightObjectPosition);
		ASSERT(rightFound);

		const real leftCellMid = (leftObjectCell.maxCorner[axis] - leftObjectCell.minCorner[axis]) * .5 +
			leftObjectCell.minCorner[axis] + leftObjectPosition[axis];
		const real rightCellMid = (rightObjectCell.maxCorner[axis] - rightObjectCell.minCorner[axis]) * .5 +
			rightObjectCell.minCorner[axis] + rightObjectPosition[axis];

		// if both good
		if (splitPlane >= leftCellMid && splitPlane <= rightCellMid)
		{
			leftCount++;

			leftId++;
			rightId--;
			continue;
		}

		// if both bad
		if (splitPlane <= leftCellMid && splitPlane >= rightCellMid)
		{
			// switch object indices
			ObjectId tmpObjectId = objects->everything[leftId];
			objects->everything[leftId] = objects->everything[rightId];
			objects->everything[rightId] = tmpObjectId;

			leftCount++;

			leftId++;
			rightId--;
			continue;
		}

		// if left is good then right is bad, so i have to find next left bad
		if (splitPlane >= leftCellMid)
		{
			leftId++;
			leftCount++;
		}
		else
		{
			rightId--;
		}
	}

	return leftCount;
}

bool BIH::GetObjectInfoById(const ObjectId& objectId, AACell& objectCell, v3f& objectPosition) const
{
	switch (objectId.Type())
	{
		case ObjectType::Sphere: 
		{
			const Sphere& object = objects->spheres[objectId.index];
			objectCell = object.cell;
			objectPosition = object.position;
			return true;
		}
		
		case ObjectType::Box:
		{
			const Box& object = objects->boxes[objectId.index];
			objectCell = object.cell;
			objectPosition = object.position;
			return true;
		}
				
		case ObjectType::Mesh:
		{
			const Mesh& object = objects->meshes[objectId.index];
			objectCell = object.cell;
			objectPosition = object.position;
			return true;
		}

		case ObjectType::SphereLightSource:
		{
			const SphereLightSource& object = objects->sphereLights[objectId.index];
			objectCell = object.cell;
			objectPosition = object.position;
			return true;
		}
		
		case ObjectType::BoxLightSource:
		{
			const BoxLightSource& object = objects->boxLights[objectId.index];
			objectCell = object.cell;
			objectPosition = object.position;
			return true;
		}

		case ObjectType::PointLightSource:
		case ObjectType::Plane:
			break;
	}

	return false;
}

void BIH::Hit(const Ray& ray, HitResult& hitResult) const
{
	hitResult.nodeTestCount = 0;

	if (!Box(rootCell).Collide(ray, EPSILON, hitResult.distance))
		return;

	if (!nodes[0].isLeaf)
		HitNode(ray, nodes[0], rootCell, hitResult);
	else
		HitLeaf(ray, nodes[0].firstObjectId, nodes[0].objectCount, hitResult);
}

void BIH::HitLeaf(const Ray& ray, uint firstObjectId, uint objectCount, HitResult& hitResult) const
{
	hitResult.nodeTestCount++;

	ObjectId innerObjectId;
	for (uint i = 0; i < objectCount; ++i)
	{
		real t = _INFINITY;
		const ObjectId& objectId = objects->everything[firstObjectId + i];
		switch (objectId.Type())
		{
			case ObjectType::Sphere: t = objects->spheres[objectId.index].Hit(ray); break;
			case ObjectType::Box: t = objects->boxes[objectId.index].Hit(ray); break;
			case ObjectType::Mesh: t = objects->meshes[objectId.index].Hit(ray, innerObjectId); break;
			case ObjectType::SphereLightSource: t = objects->sphereLights[objectId.index].Hit(ray); break;
			case ObjectType::BoxLightSource: t = objects->boxLights[objectId.index].Hit(ray); break;

			case ObjectType::Plane:
			case ObjectType::PointLightSource:
			default:
				t = _INFINITY;
		}

		if (t > EPSILON && t < hitResult.distance)
		{
			hitResult.distance = t;
			hitResult.objectId = objectId;
			if (objectId.Type() == ObjectType::Mesh)
				hitResult.innerObjectId = innerObjectId;
		}
	}

	hitResult.intersectionCount += objectCount;
}

void BIH::HitNode(const Ray& ray, const BIHNode& parentNode, const AACell& parentNodeCell, HitResult& hitResult) const
{
	hitResult.nodeTestCount++;

	if (parentNode.leftNodeId)
	{
		AACell childCell = parentNodeCell;
		childCell.maxCorner[parentNode.axis] = parentNode.leftPlane;
		Box childNodeBox;
		childNodeBox.cell = childCell;

		if (childNodeBox.Collide(ray, EPSILON, hitResult.distance))
		{
			const BIHNode& childNode = nodes[parentNode.leftNodeId];
			if (childNode.isLeaf)
				HitLeaf(ray, childNode.firstObjectId, childNode.objectCount, hitResult);
			else
				HitNode(ray, childNode, childNodeBox.cell, hitResult);
		}
	}

	if (parentNode.rightNodeId)
	{
		AACell childCell = parentNodeCell;
		childCell.minCorner[parentNode.axis] = parentNode.rightPlane;
		Box childNodeBox;
		childNodeBox.cell = childCell;

		if (childNodeBox.Collide(ray, EPSILON, hitResult.distance))
		{
			const BIHNode& childNode = nodes[parentNode.rightNodeId];
			if (childNode.isLeaf)
				HitLeaf(ray, childNode.firstObjectId, childNode.objectCount, hitResult);
			else
				HitNode(ray, childNode, childNodeBox.cell, hitResult);
		}
	}
}

bool BIH::Collide(const Ray& ray, real from, real to, const ObjectId* objectIdToSkip) const
{
	if (!Box(rootCell).Collide(ray, from, to))
		return false;

	if (maxDepth <= 0)
		return true;

	if (!nodes[0].isLeaf)
		return CollideNode(ray, nodes[0], rootCell, from, to, objectIdToSkip);
	else
		return CollideLeaf(ray, nodes[0].firstObjectId, nodes[0].objectCount,
			from, to, objectIdToSkip);
}

bool BIH::CollideLeaf(const Ray& ray, uint firstObjectId, uint objectCount,
	real from, real to, const ObjectId* objectIdToSkip) const
{
	bool collision = false;
	for (uint i = 0; i < objectCount; ++i)
	{
		const ObjectId& objectId = objects->everything[firstObjectId + i];
		if ((objectId.Type() != ObjectType::Mesh) &&
			(objectId.IsLight() || (objectIdToSkip != null && objectId._value == objectIdToSkip->_value)))
			continue;

		switch (objectId.Type())
		{
			case ObjectType::Sphere: 
				collision = objects->spheres[objectId.index].Collide(ray, from, to); 
				break;
			case ObjectType::Box: 
				collision = objects->boxes[objectId.index].Collide(ray, from, to);
				break;
			case ObjectType::Mesh:
				collision = objects->meshes[objectId.index].Collide(ray, from, to);
				break;
			case ObjectType::SphereLightSource:
				collision = objects->sphereLights[objectId.index].Collide(ray, from, to);
				break;
			case ObjectType::BoxLightSource:
				collision = objects->boxLights[objectId.index].Collide(ray, from, to);
				break;

			case ObjectType::Plane:
			case ObjectType::PointLightSource:
			default:
				break;
		}

		if (collision)
			return true;
	}

	return false;
}

bool BIH::CollideNode(const Ray& ray, const BIHNode& parentNode, const AACell& parentNodeCell, 
	real from, real to, const ObjectId* objectIdToSkip) const
{
	if (parentNode.leftNodeId)
	{
		AACell childCell = parentNodeCell;
		childCell.maxCorner[parentNode.axis] = parentNode.leftPlane;
		Box childNodeBox;
		childNodeBox.cell = childCell;

		if (childNodeBox.Collide(ray, from, to))
		{
			const BIHNode& childNode = nodes[parentNode.leftNodeId];
			if (childNode.isLeaf)
			{
				if (CollideLeaf(ray, childNode.firstObjectId, childNode.objectCount, from, to, objectIdToSkip))
					return true;
			}
			else
			{
				if (CollideNode(ray, childNode, childNodeBox.cell, from, to, objectIdToSkip))
					return true;
			}
		}
	}

	if (parentNode.rightNodeId)
	{
		AACell childCell = parentNodeCell;
		childCell.minCorner[parentNode.axis] = parentNode.rightPlane;
		Box childNodeBox;
		childNodeBox.cell = childCell;

		if (childNodeBox.Collide(ray, from, to))
		{
			const BIHNode& childNode = nodes[parentNode.rightNodeId];
			if (childNode.isLeaf)
			{
				if (CollideLeaf(ray, childNode.firstObjectId, childNode.objectCount, from, to, objectIdToSkip))
					return true;
			}
			else
			{
				if (CollideNode(ray, childNode, childNodeBox.cell, from, to, objectIdToSkip))
					return true;
			}
		}
	}

	return false;
}

void BIH::ShowStats()
{
	using namespace Common::Strings;

	char tmpBuffer[256] = {};

	if (numOfTreesConstructed)
	{
		const uint avgMemUsed =
			(uint)((real)(numOfNodesUsed + numOfLeavesUsed) * sizeof(BIHNode)) / numOfTreesConstructed;

		LOG_TL(LogLevel::Info, "Bounding interval hierarchy statistics:");
		LOG_TL(LogLevel::Info, "\ttrees constructed:\t%I64d", numOfTreesConstructed);
		//LOG_TL(LogLevel::Info, "\tconstruction time:\t%.3fms (avg %.3fms/tree)",
		//	constructionTime, constructionTime / numOfTreesConstructed);
		LOG_TL(LogLevel::Info, "\tdepth min/max/avg:\t%d/%d/%d", 
			currentMinDepth, currentMaxDepth, depthSum / numOfTreesConstructed);
		LOG_TL(LogLevel::Info, "\tobjects used:\t\t%I64d (avg %d/tree)", numOfObjectsUsed, 
			numOfObjectsUsed / numOfTreesConstructed);
		LOG_TL(LogLevel::Info, "\tnodes|leaves used:\t%I64d|%I64d (avg %d|%d/tree %s)", 
			numOfNodesUsed, numOfLeavesUsed, 
			numOfNodesUsed / numOfTreesConstructed, numOfLeavesUsed / numOfTreesConstructed,
			GetMemSizeString(tmpBuffer, avgMemUsed));
	}	
}

