
#include "Voxelizer.h"
#include "Timers.h"
#include "SparseVoxelOctree.h"

#include <stdio.h>
// Common
#include <Log.h>
#include <Vectors.h>
#include <Timer.h>
#include <StringHelpers.h>

using namespace Common::Strings;


SVO::SVO()
{

}


SVO::~SVO()
{

}


SVO_BlockDescriptor SVO::CreateBlock(const VoxelOctree* voxelOctree)
{
	// TODO: check later if this size number was not exceeded while processing of child nodes
	int nodeBufferMaxLength = voxelOctree->nodeCount * 2;

	// create node buffer
	SVO_Node* nodeBuffer = new SVO_Node[nodeBufferMaxLength];
	memset(nodeBuffer, 0, nodeBufferMaxLength * sizeof(*nodeBuffer));

	// create lookup-entry buffer
	SVO_MaterialLookUpEntry* lookUpEntryBuffer = new SVO_MaterialLookUpEntry[nodeBufferMaxLength];
	memset(lookUpEntryBuffer, 0, nodeBufferMaxLength * sizeof(*lookUpEntryBuffer));

	// create material buffer
	SVO_Material* materialBuffer = new SVO_Material[voxelOctree->nodeCount];
	memset(materialBuffer, 0, voxelOctree->nodeCount * sizeof(*materialBuffer));

	// create and prepare svo block descriptor
	SVO_BlockDescriptor blockDescriptor;
	blockDescriptor.buffer = (int*)nodeBuffer;

	// block start is aligned to pageSize
	const int blockOffset = ((int)nodeBuffer % SVO_PAGE_SIZE == 0) ? 0 : (int)ALIGN_TO(nodeBuffer, SVO_PAGE_SIZE) - (int)nodeBuffer;
	
	// relative address of block info (one per block)
	SVO_BlockInfo* blockInfo = (SVO_BlockInfo*)((int)nodeBuffer + blockOffset + sizeof(int*));
	blockInfo->firstPageOffset = ((int)nodeBuffer + blockOffset) - (int)blockInfo;
	blockInfo->nodeCount = voxelOctree->nodeCount;

	// relative pointer to nodes from this block info (in this example nodes start right after block info)
	blockInfo->nodesOffset = sizeof(SVO_BlockInfo);
	
	// set first page header (at block start)
	*(int*)((int)nodeBuffer + blockOffset) = (int)blockInfo - (int)nodeBuffer - blockOffset;

	// copy root cell
	blockInfo->cell = voxelOctree->rootCell;

	// get root octree node from memory pool
	const OctreeNode* octreeRootNode = Voxelizer::Instance()->GetMemPool()->Get(voxelOctree->rootNodeId);

	// set root svo node
	SVO_Node* svoRootNode = (SVO_Node*)((int)blockInfo + blockInfo->nodesOffset);
	svoRootNode->validMask = octreeRootNode->nodeMask;
	svoRootNode->leafMask = octreeRootNode->nodeMask;
	svoRootNode->farPtr = 0;
	svoRootNode->childPtr = sizeof(SVO_Node);

	// process recursively child nodes
	const int nodeBufferLength = 1/*first page header*/ + sizeof(SVO_BlockInfo) / sizeof(SVO_Node) + 1/*root svo_node*/ + ProcessChildNodes(octreeRootNode, svoRootNode, svoRootNode + 1);
	const int materialBufferLength = voxelOctree->nodeCount * (sizeof(SVO_Material) / sizeof(SVO_Node));

	// total effective length of block (without alignment)
	blockInfo->blockLength = nodeBufferLength/*nodes*/ + nodeBufferLength/*lookup*/ + materialBufferLength/*materials*/;

	// set offsets for other tables
	blockInfo->materialLookUpEntryOffset = (nodeBufferLength - 1) * sizeof(SVO_Node);
	blockInfo->materialTableOffset = (2 * nodeBufferLength - 1) * sizeof(SVO_Node);

	// create page headers betwneen nodes
	FixPageHeaders(blockDescriptor);

	// copy all 3 buffers together
	blockDescriptor.bufferLength = blockInfo->blockLength + SVO_PAGE_SIZE;
	blockDescriptor.buffer = new int[blockDescriptor.bufferLength];
	memset(blockDescriptor.buffer, 0, blockDescriptor.bufferLength * sizeof(int));

	int* alignedBuffer = ((int)blockDescriptor.buffer % SVO_PAGE_SIZE == 0) ? blockDescriptor.buffer : ALIGN_TO(blockDescriptor.buffer, SVO_PAGE_SIZE);

	// copy nodes buffer
	memcpy(alignedBuffer, (int*)((int)nodeBuffer + blockOffset), nodeBufferLength * sizeof(int));
	
	// copy look-up entry table buffer
	memcpy(alignedBuffer + nodeBufferLength, lookUpEntryBuffer, nodeBufferLength * sizeof(int));

	// copy material buffer
	memcpy(alignedBuffer + nodeBufferLength + nodeBufferLength, (int*)materialBuffer, materialBufferLength * sizeof(int));
	
	SAFE_DELETE(nodeBuffer);
	SAFE_DELETE(lookUpEntryBuffer);
	SAFE_DELETE(materialBuffer);

	char tmpString[256];
	memset(tmpString, 0, 256);
	LOG_TL(LogLevel::Info, "Block created [start: 0x%.8X(aligned to 0x%.8X), memory used: %s, numOfNodes: %d, voxelSize: ~%dB]",
		(int)blockDescriptor.buffer,
		(int)blockDescriptor.GetBlockInfo() + blockDescriptor.GetBlockInfo()->firstPageOffset,
		GetMemSizeString(tmpString, blockDescriptor.bufferLength * sizeof(SVO_Node)), 
		blockDescriptor.GetBlockInfo()->nodeCount, 
		blockDescriptor.bufferLength * sizeof(SVO_Node) / voxelOctree->nodeCount);

	return blockDescriptor;
}


int SVO::ProcessChildNodes(const OctreeNode* octreeNode, const SVO_Node* parentNode, SVO_Node* nextSvoNode)
{
	int nextNodeOffset = octreeNode->childCount;

	const OctreeNode* firstChildNode = Voxelizer::Instance()->GetMemPool()->Get(octreeNode->firstChildNodeId);
	for (int i = 0; i < octreeNode->childCount; i++)
	{
		const OctreeNode* childNode = firstChildNode + i;
		SVO_Node* svoNode = nextSvoNode + i;
		
		// fill material data
		// ...

		if (childNode->childCount != 0)
		{
			// node
			svoNode->validMask = childNode->nodeMask;
			svoNode->leafMask = childNode->nodeMask;

			SVO_Node* firstChildSvoNode = svoNode + nextNodeOffset - i;
			
			if (SVO_PAGE_HEADER((int)firstChildSvoNode) == (int)firstChildSvoNode)
			{
				firstChildSvoNode++;
				nextNodeOffset++;
			}
			else if (((int)firstChildSvoNode % SVO_PAGE_SIZE) + (sizeof(SVO_Node) * childNode->childCount) > SVO_PAGE_SIZE)
			{
				const int offset = ((SVO_PAGE_SIZE - (int)firstChildSvoNode % SVO_PAGE_SIZE) + sizeof(SVO_Node)) / sizeof(SVO_Node);
				firstChildSvoNode += offset;
				nextNodeOffset += offset;
			}

			svoNode->farPtr = 0;
			svoNode->childPtr = (int)firstChildSvoNode - (int)svoNode;

			nextNodeOffset += ProcessChildNodes(childNode, svoNode, firstChildSvoNode);
		}
		else
		{
			// leaf
			svoNode->childPtr = -1;
		}
	}

	return nextNodeOffset;
}


void SVO::FixPageHeaders(SVO_BlockDescriptor& blockDescriptor)
{
	const SVO_BlockInfo* blockInfo = blockDescriptor.GetBlockInfo();
	int nodeTableSize = blockInfo->materialLookUpEntryOffset;

	// set page headers with relative address of block info
	for (int i = 0; i < nodeTableSize; i+= SVO_PAGE_SIZE)
	{
		int* pageHeader = (int*)((int)blockInfo + blockInfo->firstPageOffset + i);

		if (*pageHeader != 0 && (int)pageHeader + *pageHeader != (int)blockInfo)
			LOG_TL(LogLevel::Warning, "Page header corrupted! 0x%.8X = %d", (int)pageHeader, *pageHeader);
		
		*pageHeader = (int)blockInfo - (int)pageHeader;
	}
}


bool SVO::CheckBlock(const SVO_BlockDescriptor& blockDescriptor)
{
	const SVO_BlockInfo* blockInfo = blockDescriptor.GetBlockInfo();

	// check page headers
	int nodeTableSize = blockInfo->materialLookUpEntryOffset; 
	for (int i = 0; i < nodeTableSize; i+= SVO_PAGE_SIZE)
	{
		const int* pageHeader = (int*)((int)blockInfo + blockInfo->firstPageOffset + i);
		if ((int)pageHeader + *pageHeader != (int)blockInfo)
		{
			LOG_TL(LogLevel::Error, "Page header at 0x%.8X:%d is wrong! 0x%.8X != BlockInfo(0x%.8X)", (int)pageHeader, *pageHeader, (int)pageHeader + *pageHeader, (int)blockInfo);
			return false;
		}
	}

	// traverse through all nodes and count


	return true;
}


void SVO::WriteToFile(const char* fileName, const SVO_BlockDescriptor& blockDescriptor)
{
	FILE* svoFile = fopen(fileName, "wb");
	int lengthWritten = fwrite(blockDescriptor.GetBlock(), sizeof(int), blockDescriptor.GetBlockInfo()->blockLength, svoFile);
	
	if (lengthWritten != blockDescriptor.GetBlockInfo()->blockLength)
	{
		LOG_TL(LogLevel::Error, "fwrite returned %d instead of %d", lengthWritten, blockDescriptor.GetBlockInfo()->blockLength);
	}
	else
	{
		char tmpString[256];
		memset(tmpString, 0, 256);

		LOG_TL(LogLevel::Info, "Block successfully written to '%s' (%s)", fileName, GetMemSizeString(tmpString, blockDescriptor.GetBlockInfo()->blockLength * sizeof(int)));
	}

	fclose(svoFile);
}

SVO_BlockDescriptor SVO::ReadFromFile(const char* fileName)
{
	SVO_BlockDescriptor blockDescriptor;

	FILE* svoFile = fopen(fileName, "rb");
	fseek(svoFile, 0, SEEK_END);

	int blockLength = ftell(svoFile) / sizeof(int);
	rewind(svoFile);

	blockDescriptor.bufferLength = blockLength + SVO_PAGE_SIZE;
	blockDescriptor.buffer = new int[blockDescriptor.bufferLength];
	int* alignedBuffer = ((int)blockDescriptor.buffer % SVO_PAGE_SIZE == 0) ? blockDescriptor.buffer : ALIGN_TO(blockDescriptor.buffer, SVO_PAGE_SIZE);

	int lengthRead = fread(alignedBuffer, sizeof(int), blockLength, svoFile);
	if (lengthRead != blockLength)
	{
		LOG_TL(LogLevel::Error, "fread returned %d instead of %d", lengthRead, blockDescriptor.GetBlockInfo()->blockLength);
	}
	else
	{
		char tmpString[256];
		memset(tmpString, 0, 256);

		LOG_TL(LogLevel::Info, "Block successfully read from '%s' (%s)", fileName, GetMemSizeString(tmpString, blockDescriptor.GetBlockInfo()->blockLength * sizeof(int)));
	}

	fclose(svoFile);

	return blockDescriptor;
}
