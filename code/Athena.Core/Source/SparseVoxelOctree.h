#pragma once

#include <Object.h>

// each block start is aligned to this size
#define SVO_PAGE_SIZE						0x1000

// get page header address from node address
#define SVO_PAGE_HEADER(nodeAddress)		(nodeAddress & ~(SVO_PAGE_SIZE - 1))

// get block info from page header address
#define SVO_BLOCK_INFO(pageHeaderAddress)	(SVO_BlockInfo*)((*(int*)pageHeaderAddress) + pageHeaderAddress);

// returns first possible address aligned to given size
#define ALIGN_TO(address, pageLength)		((int*)(((int)address & ~(pageLength - 1)) + pageLength))


#pragma pack(push, 1)

struct SVO_BlockInfo
{
	// length of the whole block
	int blockLength;

	// number of voxel nodes
	int nodeCount;

	// relative offset to block start
	int firstPageOffset;
	// relative offset to block start from this block info
	int nodesOffset;
	// relative offset to material lookup entry table
	int materialLookUpEntryOffset;
	// relative offset to material table
	int materialTableOffset;

	// block cell
	AACell cell;
};

// sizeof() = 4B
struct SVO_Node
{
	// pointer to first child node
	unsigned short childPtr : 15;
	// if set to 1, next int is 32B ptr to first node child
	unsigned short farPtr : 1;
	// specifies position and number of child nodes
	unsigned char validMask;
	// specifies which of child nodes are actualy leaves
	unsigned char leafMask;
};

struct SVO_BlockDescriptor
{
	int* buffer;
	int bufferLength;

	inline const int* GetBlock() const 
	{ 
		return ((int)buffer % SVO_PAGE_SIZE == 0) ? buffer : ALIGN_TO(buffer, SVO_PAGE_SIZE);
	}

	inline const SVO_BlockInfo* GetBlockInfo() const
	{ 
		const int* block = GetBlock();
		return (SVO_BlockInfo*)((int)block + *block);
	}
};

// sizeof() = 4B
struct SVO_MaterialLookUpEntry
{
	// poiter to material
	unsigned int ptrToMaterial : 24;
	// 
	unsigned int validMask : 8;
};


// sizeof() = 8B
struct SVO_Material
{
	// voxel color
	vector4b color;
	// voxel normal, positive or negative space
	unsigned int sign : 1;
	// voxel normal axis
	unsigned int axis : 2;
	// voxel normal u-coordinate on unit cube
	unsigned int u : 15;
	// voxel normal v-coordinate on unit cube
	unsigned int v : 14;
};

#pragma pack(pop)


class SVO : public Singleton<SVO>
{
public:

	SVO();
	~SVO();

	SVO_BlockDescriptor CreateBlock(const VoxelOctree* voxelOctree);
	
	bool CheckBlock(const SVO_BlockDescriptor& blockDescriptor);

	void WriteToFile(const char* directory, const SVO_BlockDescriptor& blockDescriptor);
	SVO_BlockDescriptor ReadFromFile(const char* fileName);

private:

	int ProcessChildNodes(const OctreeNode* octreeNode, const SVO_Node* parentNode, SVO_Node* nextSvoNode);
	void FixPageHeaders(SVO_BlockDescriptor& blockDescriptor);
};
