#include "List.h"
#include "Log.h"
#include "Mesh.h"
#include <stdio.h>
#include <string.h>
#include "StringHelpers.h"
#include "Vectors.h"


#define MAX_LINE_SIZE			1024
#define MAX_ELEMENT_TAG_SIZE	100
#define MAX_FILENAME_LENGTH		256


//int GetMaterialIdByName(const char* materialName)
//{
//	if (!materialName)
//		return 0;
//
//	// skip first reserved material
//	for (auto i = 1; i < numOfMaterialsUsed; i++)
//	if (materialNames[i] && strcmp(materialName, materialNames[i]) == 0)
//		return i;
//
//	LOG_TL(LogLevel::Warning, "Material '%s' not found!", materialName);
//	return 0;
//}

// TODO prepisat nacitavanie z pamate, nie zo suboru
bool LoadWavefrontObjectFromFile(const char* filename, MemoryManager* memoryManagerInstance, Mesh& mesh)
{
	if (!filename)
		return false;

	list_of<v3f> vertices(memoryManagerInstance, "v3f");
	list_of<Triangle> triangles(memoryManagerInstance, "Triangle");
	list_of<v3f> vertexNormals(memoryManagerInstance, "v3f");
	list_of<v2f> textureCoords(memoryManagerInstance, "v2f");
	list_of<Material> materials(memoryManagerInstance, "Material");

	//if (materials == null)
	//{
	//	materials = new Material[MAX_NUM_OF_MATERIALS];
	//	LOG_TL(LogLevel::Info, "MeshManager::materials[%d] using %s", MAX_NUM_OF_MATERIALS, GetMemSizeString(tmpBuffer, sizeof(Material)* MAX_NUM_OF_MATERIALS));

	//	materialNames = new char*[MAX_NUM_OF_MATERIALS];
	//	for (int i = 0; i < MAX_NUM_OF_MATERIALS; i++)
	//		materialNames[i] = null;

	//	// first material is reserved (triangles without material will use this one)
	//	materials[0].Clear();
	//	materialNames[0] = new char[strlen("__no_material") + 1];
	//	strcpy(materialNames[0], "__no_material");
	//	numOfMaterialsUsed = 1;
	//}

	char lineBuffer[MAX_LINE_SIZE];
	char elementTag[MAX_ELEMENT_TAG_SIZE];

	// open .obj file
	auto file = fopen(filename, "rt");
	if (!file)
	{
		LOG_TL(LogLevel::Error, "Unable to open mesh file: '%s'", filename);
		return false;
	}

	// read one line at a time
	while (fgets(lineBuffer, MAX_LINE_SIZE, file))
	{
		// get element tag (first part of line)
		sscanf(lineBuffer, "%s", elementTag);

		// read vertex
		if (!strcmp(elementTag, "v"))
		{
			vector3<real32> tmp;
			sscanf(lineBuffer + strlen(elementTag) + 1, "%f %f %f", &tmp.x, &tmp.y, &tmp.z);
			
			vertices.Add(v3f((real)tmp.x, (real)tmp.y, (real)tmp.z));
		}
		
		// read vertex texture coordinate
		else if (!strcmp(elementTag, "vt"))
		{
			vector2<real32> tmp;
			sscanf(lineBuffer + strlen(elementTag) + 1, "%f %f", &tmp.x, &tmp.y);

			textureCoords.Add(v2f((real)tmp.x, (real)tmp.y));
		}

		// read vertex normal
		else if (!strcmp(elementTag, "vn"))
		{
			vector3<real32> tmp;
			sscanf(lineBuffer + strlen(elementTag) + 1, "%f %f %f", &tmp.x, &tmp.y, &tmp.z);

			vertexNormals.Add(v3f((real)tmp.x, (real)tmp.y, (real)tmp.z));
		}

		// read triangle
		else if (!strcmp(elementTag, "f"))
		{
			if (Common::Strings::IndexOfChar(lineBuffer, '-') >= 0)
			{
				LOG_TL(LogLevel::Warning, "LoadWavefrontObjectFromFile: Negative indices are not supported!");
			}
			else
			{
				if (lineBuffer[strlen(lineBuffer) - 1] == '\n')
					lineBuffer[strlen(lineBuffer) - 1] = 0;

				const char* face = lineBuffer + strlen(elementTag) + 1;
				const int numOfVerticesInFace = Common::Strings::CountOccurence(face, ' ') + 1;
				const int numOfVertexParts = Common::Strings::CountOccurence(face, '/') / 3 + 1;

				Triangle triangle;

				if (numOfVerticesInFace == 3)
				{
					switch (numOfVertexParts)
					{
						// f v1 v2 v3
						case 1: 
							sscanf(lineBuffer + strlen(elementTag) + 1, "%d %d %d", 
								&triangle.v.x, &triangle.v.y, &triangle.v.z); 
							break;

						// f v1/tc1 v2/tc2 v3/tc3
						case 2: 
							sscanf(lineBuffer + strlen(elementTag) + 1, "%d/%d %d/%d %d/%d", 
								&triangle.v.x, &triangle.tc.x, &triangle.v.y, 
								&triangle.tc.y, &triangle.v.z, &triangle.tc.z); 
							break;

						case 3:
						{
							int i = 0;
							while (face[i] != '/')
								i++;

							if (face[i + 1] == '/')
								// f v1//vn1 v2//vn2 v3//vn3
								sscanf(lineBuffer + strlen(elementTag) + 1, "%d//%d %d//%d %d//%d", 
									&triangle.v.x, &triangle.vn.x,
									&triangle.v.y, &triangle.vn.y,
									&triangle.v.z, &triangle.vn.z);

							else
								// f v1/tc1/vn1 v2/tc2/vn2 v3/tc3/vn3
								sscanf(lineBuffer + strlen(elementTag) + 1, "%d/%d/%d %d/%d/%d %d/%d/%d", 
									&triangle.v.x, &triangle.tc.x, &triangle.vn.x,
									&triangle.v.y, &triangle.tc.y, &triangle.vn.y,
									&triangle.v.z, &triangle.tc.z, &triangle.vn.z);
						}
						break;
					};

					// indices in .obj are indexed from 1
					triangle.v -= 1;
					triangle.vn -= 1;
					triangle.tc -= 1;

					//triangle.materialIndex = activeMaterial;
					triangles.Add(triangle);
				}
				else if (numOfVerticesInFace == 4)
				{
					v4i v, tc, vn;

					switch (numOfVertexParts)
					{
						// f v1 v2 v3 v4
						case 1: 
							sscanf(lineBuffer + strlen(elementTag) + 1, "%d %d %d %d", 
								&v.x, &v.y, &v.z, &v.w); 
							break;

						// f v1/tc1 v2/tc2 v3/tc3 v4/tc4
						case 2: 
							sscanf(lineBuffer + strlen(elementTag) + 1, "%d/%d %d/%d %d/%d %d/%d", 
								&v.x, &tc.x, &v.y, &tc.y, &v.z, &tc.z, &v.w, &tc.w); 
							break;

						case 3:
						{
							int i = 0;
							while (face[i] != '/')
								i++;

							if (face[i + 1] == '/')
								// f v1//vn1 v2//vn2 v3//vn3 v4//vn4
								sscanf(lineBuffer + strlen(elementTag) + 1, "%d//%d %d//%d %d//%d %d//%d", 
									&v.x, &vn.x, &v.y, &vn.y, &v.z, &vn.z, &v.w, &vn.w);

							else
								// f v1/tc1/vn1 v2/tc2/vn2 v3/tc3/vn3 v4/tc4/vn4
								sscanf(lineBuffer + strlen(elementTag) + 1, "%d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d", 
									&v.x, &tc.x, &vn.x, &v.y, &tc.y, &vn.y, &v.z, &tc.z, &vn.z, &v.w, &tc.w, &vn.w);
						}
						break;
					};

					v -= 1;
					vn -= 1;
					tc -= 1;

					triangle.v.x = v.x; triangle.v.y = v.y; triangle.v.z = v.z;
					triangle.vn.x = vn.x; triangle.vn.y = vn.y; triangle.vn.z = vn.z;
					triangle.tc.x = tc.x; triangle.tc.y = tc.y; triangle.tc.z = tc.z;
					//triangle.materialIndex = activeMaterial;
					triangles.Add(triangle);

					triangle.v.x = v.x; triangle.v.y = v.z; triangle.v.z = v.w;
					triangle.vn.x = vn.x; triangle.vn.y = vn.z; triangle.vn.z = vn.w;
					triangle.tc.x = tc.x; triangle.tc.y = tc.z; triangle.tc.z = tc.w;
					//triangle.materialIndex = activeMaterial;
					triangles.Add(triangle);
				}
				else
				{
					LOG_TL(LogLevel::Warning, "Unsupported polygon type! [size: %d line: '%s']", 
						numOfVerticesInFace, lineBuffer);
				}
			}
		}

		// read material name
		else if (!strcmp(elementTag, "mtllib"))
		{
			// replace obj filename with mtl filename (i assume that material is in the same directory as obj)
			char tmpBuffer[MAX_FILENAME_LENGTH] = {};
			memset(tmpBuffer, 0, MAX_FILENAME_LENGTH);

			strcpy(tmpBuffer, filename);
			size_t filenameStart = strlen(tmpBuffer);

			for (filenameStart; filenameStart > 0; filenameStart--)
				if (tmpBuffer[filenameStart] == '\\')
					break;

			tmpBuffer[filenameStart + 1] = '\0';
			strcat(tmpBuffer, lineBuffer + strlen(elementTag) + 1);

			// remove new line char
			tmpBuffer[strlen(tmpBuffer) - 1] = '\0';
			const char* materialFilename = tmpBuffer;

			auto materialFile = fopen(materialFilename, "rt");
			if (!materialFile)
			{
				LOG_TL(LogLevel::Warning, "LoadWavefrontObjectFromFile:: Unable to open material file: '%s'", 
					materialFilename);
			}
			else
			{
				//materials[numOfMaterials].Clear();
				//while (fgets(lineBuffer, MAX_LINE_SIZE, materialFile))
				//{
				//	// get element tag (first part of line)
				//	sscanf(lineBuffer, "%s", elementTag);

				//	// read material name
				//	if (!strcmp(elementTag, "newmtl"))
				//	{
				//		// remove new line char
				//		lineBuffer[strlen(lineBuffer) - 1] = '\0';
				//		const char* materialName = _strupr(lineBuffer + strlen(elementTag) + 1);

				//		materialNames[numOfMaterials] = new char[strlen(materialName) + 1];
				//		strcpy(materialNames[numOfMaterials], materialName);
				//		numOfMaterials++;
				//	}

				//	// TODO read other material values
				//}

				fclose(materialFile);
			}
		}

		else if (!strcmp(elementTag, "usemtl"))
		{	
			// remove new line char
			//lineBuffer[strlen(lineBuffer) - 1] = '\0';
			//const char* materialName = _strupr(lineBuffer + strlen(elementTag) + 1);

			//activeMaterial = GetMaterialIdByName(materialName);
		}

		memset(lineBuffer, 0, MAX_LINE_SIZE);
		memset(elementTag, 0, MAX_ELEMENT_TAG_SIZE);
	}

	// close .obj file
	fclose(file);

	// triangle post-processing
	for (uint i = 0; i < triangles.currentCount; i++)
	{
		auto triangle = triangles[i];

		// add mesh index offset
		//triangle.v += (int)vertices.currentCount;
		//triangle.vn += (int)vertexNormals.currentCount;
		//triangle.tc += (int)textureCoords.currentCount;

		// compute face normal
		v3f v1 = vertices[triangle.v.y] - vertices[triangle.v.x];
		v3f v2 = vertices[triangle.v.z] - vertices[triangle.v.x];
		
		triangle.normal = vectors::Cross(v1, v2);
		vectors::Normalize(triangle.normal);

		if (!vertexNormals.currentCount)
		{
			// TODO compute vertex normals
		}
	}

	//LOG_TL(LogLevel::Info, "\tvertices: %d", numOfVertices);
	//LOG_TL(LogLevel::Info, "\ttriangles: %d", numOfTriangles);
	//LOG_TL(LogLevel::Info, "\tvertexNormals: %d", numOfVertexNormals);
	//LOG_TL(LogLevel::Info, "\ttextureCoords: %d", numOfTextureCoords);
	//LOG_TL(LogLevel::Info, "\tmaterials: %d", numOfMaterials);
	
	mesh.vertices = vertices.CopyToArray();
	mesh.triangles = triangles.CopyToArray();
	mesh.vertexNormals = vertexNormals.CopyToArray();
	mesh.textureCoords = textureCoords.CopyToArray();
	mesh.materials = materials.CopyToArray();

	vertices.Destroy();
	triangles.Destroy();
	vertexNormals.Destroy();
	textureCoords.Destroy();
	materials.Destroy();

	return true;
}
