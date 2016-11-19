#include "Athena.h"
#include "HitResult.h"
#include "Log.h"
#include "MemoryManager.h"
#include "Octree.h"
#include "Ray.h"
#include "Rendering.h"
#include "Scene.h"
#include "Timers.h"
#include "Timer.h"
#include "ZOrder.h"

#define NODE_MEM_POOL_PAGE_SIZE	1024


ui8 Octree::octreeNodePositions[8];


void Octree::Destroy(MemoryManager* memoryManagerInstance)
{
	if (objects)
	{
		ShowStats();

		objects = null;

		for (uint i = 0; i < threadNodes.count; ++i)
			threadNodes[i].Destroy();
		_MEM_FREE_ARRAY(memoryManagerInstance, list_of<OctreeNode>, &threadNodes);

		LOG_DEBUG("Octree::Destroy");
	}
}

void Octree::Initialize(Objects* objects, MemoryManager* memoryManagerInstance)
{
	numOfNodesUsed = numOfObjectsUsed = depthSum = numOfEmptyNodes = 0;
	numOfTreesConstructed = 0;
	currentDepth = currentMaxDepth = 0;
	currentMinDepth = OCTREE_DEFAULT_MAX_DEPTH;
	totalThreadsUsed = 0;

	threadNodes = _MEM_ALLOC_ARRAY(memoryManagerInstance, list_of<OctreeNode>, 8);
	for (uint i = 0; i < threadNodes.count; ++i)
		threadNodes[i].Initialize(memoryManagerInstance, "OctreeNode", NODE_MEM_POOL_PAGE_SIZE);

	octreeNodePositions[0] = OctreeNodePosition::x0y0z0;
	octreeNodePositions[1] = OctreeNodePosition::x1y0z0;
	octreeNodePositions[2] = OctreeNodePosition::x0y1z0;
	octreeNodePositions[3] = OctreeNodePosition::x1y1z0;
	octreeNodePositions[4] = OctreeNodePosition::x0y0z1;
	octreeNodePositions[5] = OctreeNodePosition::x1y0z1;
	octreeNodePositions[6] = OctreeNodePosition::x0y1z1;
	octreeNodePositions[7] = OctreeNodePosition::x1y1z1;

	this->objects = objects;
}

bool Octree::Update(array_of<std::thread>& threads, ui32 maxDepth, AthenaStorage* athenaStorage)
{
	TIMED_BLOCK(&athenaStorage->timers[TimerId::OctreeConstruction]);

	// use multi-threading
	const bool useThreads = threads.count >= 8;

	for (uint i = 0; i < threadNodes.count; ++i)
		threadNodes[i].Clear();

	currentDepth = 0;
	currentNumOfNodesUsed = 0;

	if (!objects->everything.currentCount)
		return false;

	if (athenaStorage->renderingParameters.tracingMethod != TracingMethod::Octree)
		return false;

	// get root bounding box
	UpdateRootCell();

	ui32 nodesUsed[8] = {};
	if (maxDepth)
	{
		const v3f childCellSize = (rootCell.maxCorner - rootCell.minCorner) * .5;
		const v3f rootCellCenter = (rootCell.minCorner + rootCell.maxCorner) * .5;

		AACell childNodeCell;
		for (ui32 threadId = 0; threadId < 8; ++threadId)
		{
			const v3ui zorder3 = ZOrder::Decode3ui(threadId);
			const v3f zorder3f(zorder3.x, zorder3.y, zorder3.z);

			childNodeCell.minCorner = rootCell.minCorner + zorder3f * childCellSize;
			childNodeCell.maxCorner = rootCellCenter + zorder3f * childCellSize;

			if (useThreads)
			{
				threads[threadId] = std::thread(ProcessNode, threadNodes[threadId], objects, &nodesUsed[threadId],
					childNodeCell, (ui32)threadNodes[threadId].Add(), maxDepth);
			}
			else
				ProcessNode(threadNodes[threadId], objects, &nodesUsed[threadId],
					childNodeCell, (ui32)threadNodes[threadId].Add(), maxDepth);
		}

		if (useThreads)
			for (ui32 threadId = 0; threadId < 8; ++threadId)
				threads[threadId].join();
	}

	totalThreadsUsed += 8;
	
	currentNumOfNodesUsed = 1; // root node
	for (ui32 i = 0; i < 8; ++i)
		currentNumOfNodesUsed += nodesUsed[i];

	numOfNodesUsed += currentNumOfNodesUsed;
	numOfEmptyNodes += GetTotalNodeCount(maxDepth) - currentNumOfNodesUsed;
	
	numOfTreesConstructed++;
	numOfObjectsUsed += objects->everything.currentCount;

	currentDepth = maxDepth;
	depthSum += currentDepth;

	if (currentDepth < currentMinDepth)
		currentMinDepth = currentDepth;

	if (currentDepth > currentMaxDepth)
		currentMaxDepth = currentDepth;

	return true;
}

ui64 Octree::GetTotalNodeCount(ui32 depth)
{
	ui64 count = 0;
	for (ui32 i = 0; i <= depth; ++i)
		count += (ui64)pow(pow(2, i), 3);

	return count;
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

void Octree::UpdateRootCell()
{
	rootCell.maxCorner.Set(-_INFINITY, -_INFINITY, -_INFINITY);
	rootCell.minCorner.Set(_INFINITY, _INFINITY, _INFINITY);

	AddObjectListToCell<Box>(objects->boxes, rootCell);
	AddObjectListToCell<Sphere>(objects->spheres, rootCell);
	//AddObjectListToCell<PointLightSource>(objects->lights, rootCell);
	AddObjectListToCell<Mesh>(objects->meshes, rootCell);

	// pre zjednodusenie, bude octree v tvare kocky
	const real max = MAX3(rootCell.maxCorner.x, rootCell.maxCorner.y, rootCell.maxCorner.z);
	const real min = MIN3(rootCell.minCorner.x, rootCell.minCorner.y, rootCell.minCorner.z);
	rootCell.maxCorner.Set(max, max, max);
	rootCell.minCorner.Set(min, min, min);
}

void Octree::ProcessNode(list_of<OctreeNode>& nodes, const Objects* objects, ui32* nodesUsed,
	const AACell& nodeCell, ui32 nodeId, ui32 maxDepth)
{
	nodes[nodeId].Clear();

	if (!maxDepth)
		return;
	maxDepth--;

	const v3f childCellSize = (nodeCell.maxCorner - nodeCell.minCorner) * .5;
	const v3f nodeCellCenter = (nodeCell.minCorner + nodeCell.maxCorner) * .5;

	AACell childNodeCell;
	for (ui32 i = 0; i < 8; ++i)
	{
		const v3ui zorder3 = ZOrder::Decode3ui(i);
		const v3f zorder3f(zorder3.x, zorder3.y, zorder3.z);

		childNodeCell.minCorner = nodeCell.minCorner + zorder3f * childCellSize;
		childNodeCell.maxCorner = nodeCellCenter + zorder3f * childCellSize;
		
		if (!IsNodeEmpty(objects, childNodeCell))
		{
			nodes[nodeId].nodeMask |= octreeNodePositions[i];
			nodes[nodeId].childNodeCount++;
		}
	}
	
	if (!maxDepth)
	{
		// IsParentNode() == true
		// child nodes are leafs, so this will be called parent node
		// processing ends here, because we dont need to allocate memory for child nodes (leaves)
	}
	else if (nodes[nodeId].childNodeCount)
	{
		nodes[nodeId].firstChildNodeId = (ui32)nodes.Add(nodes[nodeId].childNodeCount);

		const ui8 nodeMask = nodes[nodeId].nodeMask;
		ui32 nextChildNodeId = nodes[nodeId].firstChildNodeId;
		for (ui32 i = 0; i < 8; ++i)
		{
			if (nodeMask & octreeNodePositions[i])
			{
				const v3ui zorder3 = ZOrder::Decode3ui(i);
				const v3f zorder3f(zorder3.x, zorder3.y, zorder3.z);

				childNodeCell.minCorner = nodeCell.minCorner + zorder3f * childCellSize;
				childNodeCell.maxCorner = nodeCellCenter + zorder3f * childCellSize;

				ProcessNode(nodes, objects, nodesUsed, childNodeCell, nextChildNodeId, maxDepth);
				nextChildNodeId++;
			}
		}
	}

	*nodesUsed = (ui32)nodes.currentCount;
}

bool Octree::IsNodeEmpty(const Objects* objects, const AACell& nodeCell)
{
	for (uint i = 0; i < objects->everything.currentCount; ++i)
	{
		const ObjectId& objectId = objects->everything[i];
		switch (objects->everything[i].Type())
		{
			case ObjectType::Sphere: 
				if (objects->spheres[objectId.index].AndAACell(nodeCell))
					return false;
				break;
		
			case ObjectType::Box:
				if (objects->boxes[objectId.index].AndAACell(nodeCell))
					return false;
				break;

			case ObjectType::Plane:
			case ObjectType::Mesh:
			case ObjectType::PointLightSource:
				break;
		}
	}

	return true;
}

void Octree::Hit(const Ray& ray, HitResult& hitResult) const
{
	if (!currentDepth)
		return;

	hitResult.nodeTestCount++;

	if (!Box(rootCell).Collide(ray, 0, hitResult.distance))
		return;

	const v3f childCellSize = (rootCell.maxCorner - rootCell.minCorner) * .5;
	const v3f rootCellCenter = (rootCell.minCorner + rootCell.maxCorner) * .5;

	AACell childNodeCell;
	for (ui32 threadId = 0; threadId < 8; ++threadId)
	{
		if (!threadNodes[threadId].currentCount)
			continue;

		const v3ui zorder3 = ZOrder::Decode3ui(threadId);
		const v3f zorder3f(zorder3.x, zorder3.y, zorder3.z);

		childNodeCell.minCorner = rootCell.minCorner + zorder3f * childCellSize;
		childNodeCell.maxCorner = rootCellCenter + zorder3f * childCellSize;

		HitNode(ray, threadNodes[threadId], 0, childNodeCell, hitResult);
	}
}

void Octree::HitNode(const Ray& ray, const list_of<OctreeNode>& nodes, ui32 nodeId, const AACell& nodeCell, 
	HitResult& hitResult) const
{
	if (!nodes[nodeId].childNodeCount)
		return;

	hitResult.nodeTestCount++;

	if (!Box(nodeCell).Collide(ray, 0, hitResult.distance))
		return;

	const v3f childCellSize = (nodeCell.maxCorner - nodeCell.minCorner) * .5;
	const v3f nodeCellCenter = (nodeCell.minCorner + nodeCell.maxCorner) * .5;

	AACell childNodeCell;
	ui32 nextChildNodeId = nodes[nodeId].firstChildNodeId;
	for (ui32 i = 0; i < 8; ++i)
	{
		if (nodes[nodeId].nodeMask & octreeNodePositions[i])
		{
			const v3ui zorder3 = ZOrder::Decode3ui(i);
			const v3f zorder3f(zorder3.x, zorder3.y, zorder3.z);

			childNodeCell.minCorner = nodeCell.minCorner + zorder3f * childCellSize;
			childNodeCell.maxCorner = nodeCellCenter + zorder3f * childCellSize;

			if (nodes[nodeId].IsParentNode())
			{
				hitResult.nodeTestCount++;

				Box childNodeBox(childNodeCell);
				const real distance = childNodeBox.Hit(ray);

				if (distance < hitResult.distance)
				{
					hitResult.distance = distance;
					hitResult.point = ray.origin + ray.direction * hitResult.distance;
					childNodeBox.GetNormalAt(hitResult.point, hitResult.normal);
					
					hitResult.objectId.Set(ObjectType::Voxel, 0);
				}
			}
			else
			{
				HitNode(ray, nodes, nextChildNodeId, childNodeCell, hitResult);
				nextChildNodeId++;
			}
		}
	}
}

bool Octree::Collide(const Ray& ray, real from, real to) const
{
	if (!Box(rootCell).Collide(ray, from, to))
		return false;

	const v3f childCellSize = (rootCell.maxCorner - rootCell.minCorner) * .5;
	const v3f rootCellCenter = (rootCell.minCorner + rootCell.maxCorner) * .5;

	AACell childNodeCell;
	for (ui32 nodePoolId = 0; nodePoolId < 8; ++nodePoolId)
	{
		if (!threadNodes[nodePoolId].currentCount)
			continue;

		const v3ui zorder3 = ZOrder::Decode3ui(nodePoolId);
		const v3f zorder3f(zorder3.x, zorder3.y, zorder3.z);

		childNodeCell.minCorner = rootCell.minCorner + zorder3f * childCellSize;
		childNodeCell.maxCorner = rootCellCenter + zorder3f * childCellSize;

		if (CollideNode(ray, threadNodes[nodePoolId], 0, childNodeCell, from, to))
			return true;
	}

	return false;
}

bool Octree::CollideNode(const Ray& ray, const list_of<OctreeNode>& nodes, uint32 nodeId, const AACell& nodeCell,
	real from, real to) const
{
	if (!nodes[nodeId].childNodeCount)
		return false;

	if (!Box(nodeCell).Collide(ray, from, to))
		return false;

	const v3f childCellSize = (nodeCell.maxCorner - nodeCell.minCorner) * .5;
	const v3f nodeCellCenter = (nodeCell.minCorner + nodeCell.maxCorner) * .5;

	AACell childNodeCell;
	ui32 nextChildNodeId = nodes[nodeId].firstChildNodeId;
	for (ui32 i = 0; i < 8; ++i)
	{
		if (nodes[nodeId].nodeMask & octreeNodePositions[i])
		{
			const v3ui zorder3 = ZOrder::Decode3ui(i);
			const v3f zorder3f(zorder3.x, zorder3.y, zorder3.z);

			childNodeCell.minCorner = nodeCell.minCorner + zorder3f * childCellSize;
			childNodeCell.maxCorner = nodeCellCenter + zorder3f * childCellSize;

			if (nodes[nodeId].IsParentNode())
			{
				Box childNodeBox(childNodeCell);
				if (/*!childNodeBox.IsInside(ray.origin) && */childNodeBox.Collide(ray, from, to))
					return true;
			}
			else
			{
				if (CollideNode(ray, nodes, nextChildNodeId, childNodeCell, from, to))
					return true;
			
				nextChildNodeId++;
			}
		}
	}

	return false;
}

void Octree::ShowStats()
{
	using namespace Common::Strings;

	char tmpBuffer[256] = {};

	if (numOfTreesConstructed)
	{
		const uint avgMemUsed = (uint)((real)numOfNodesUsed * sizeof(BIHNode)) / numOfTreesConstructed;

		LOG_TL(LogLevel::Info, "Octree statistics:");
		LOG_TL(LogLevel::Info, "\ttrees constructed:\t%I64d", numOfTreesConstructed);
		LOG_TL(LogLevel::Info, "\tthreads used:\t\t~%d/tree", totalThreadsUsed / numOfTreesConstructed);
		//LOG_TL(LogLevel::Info, "\tconstruction time:\t%.3fms (avg %.3fms/tree)",
		//	constructionTime, constructionTime / numOfTreesConstructed);
		LOG_TL(LogLevel::Info, "\tdepth min/max/avg:\t%d/%d/%d",
			currentMinDepth, currentMaxDepth, depthSum / numOfTreesConstructed);
		LOG_TL(LogLevel::Info, "\tobjects used:\t\t%I64d (avg %d/tree)", numOfObjectsUsed,
			numOfObjectsUsed / numOfTreesConstructed);
		LOG_TL(LogLevel::Info, "\tnodes used:\t\t%d (avg %d/tree %s)",
			numOfNodesUsed,
			numOfNodesUsed / numOfTreesConstructed,
			GetMemSizeString(tmpBuffer, avgMemUsed));
		LOG_TL(LogLevel::Info, "\ttree fill:\t\t~%.3f%%", 
			((real)numOfNodesUsed / (numOfNodesUsed + numOfEmptyNodes)) * 100);
	}
}

//void Voxelizer::FillVoxelAsAverage(OctreeNode* node)
//{
//	int childNodesCount = 0;
//	int nextChildNodeId = node->firstChildNodeId;
//
//	vector4i color;
//	xyzw_SET(color, 0, 0, 0, 0);
//
//	for (int i = 1; i != 256; i *= 2)
//		if (node->nodeMask & (unsigned char)i)
//		{
//			const Voxel* voxel = &nodeMemPool->Get(nextChildNodeId)->voxel;
//
//			xyzw_ADD_VECTOR(color, voxel->color);
//			xyz_ADD_VECTOR(node->voxel.normal, voxel->normal);
//			
//			nextChildNodeId++;
//			childNodesCount++;
//		}
//
//	const float childNodesCountRec = 1.0f / (float)childNodesCount;
//	xyz_MUL(node->voxel.normal, childNodesCountRec);
//	xyzw_SET(node->voxel.color, 
//		(unsigned char)(color.x / childNodesCount),
//		(unsigned char)(color.y / childNodesCount),
//		(unsigned char)(color.z / childNodesCount),
//		(unsigned char)(color.w / childNodesCount));
//}
//
//int Voxelizer::CountNodes(const OctreeNode* node)
//{
//	if (node->nodeMask == 0)
//		return 1;
//
//	int nodesCount = 1;
//	int nextChildNodeId = node->firstChildNodeId;
//
//	for (int i = 1; i != 256; i *= 2)
//		if (node->nodeMask & (unsigned char)i)
//		{
//			nodesCount += CountNodes(nodeMemPool->Get(nextChildNodeId));
//			nextChildNodeId++;
//		}
//	
//	return nodesCount;
//}
//
//int Octree::GetDepth(const OctreeNode* node)
//{
//	if (node->nodeMask == 0)
//		return 1;
//
//	int maxDepth = 0;
//	int nextChildNodeId = node->firstChildNodeId;
//	
//	for (int i = 1; i != 256; i *= 2)
//		if (node->nodeMask & (unsigned char)i)
//		{
//			int depth = GetDepth(nodeMemPool->Get(nextChildNodeId));
//			
//			if (depth > maxDepth)
//				maxDepth = depth;
//
//			nextChildNodeId++;
//		}
//
//	return maxDepth + 1;
//}
