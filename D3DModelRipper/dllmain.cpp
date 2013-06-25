// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <map>
#include <Windows.h>
#include <direct.h>
#include <d3d9.h>
#include <vector>
#include <d3dx9.h>
#include <Detours.h>
#include <thread>
#include <fstream>
#include <Shlwapi.h>

#pragma comment(lib, "Detours.lib")
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "Shlwapi.lib")

using namespace std;

vector<long> texturePointers;
map<WORD, WORD> vmap;
ofstream error_file("Error.txt");
ofstream out;
std::stringstream sstm;
bool dumped;

typedef struct _INIT_STRUCT 
{
	long addresses[14];
} INIT_STRUCT, *PINIT_STRUCT;

struct vertex
{
	float x, y,z;
};

int GetIndex(vertex v);
vertex GetVertex(WORD i, void * verts);
bool VertContains(vertex v);

vector<vertex> vertex_vec;

HMODULE dll;
IDirect3DIndexBuffer9 * bound_indices;
IDirect3DVertexBuffer9 * bound_vertices;
D3DINDEXBUFFER_DESC index_desc;
D3DVERTEXBUFFER_DESC vertex_desc; 
UINT stream_number;
UINT stride;

extern "C" typedef HRESULT (WINAPI *pSetTexture)(LPDIRECT3DDEVICE9 pDevice, DWORD stage, IDirect3DBaseTexture9 * texture); 
extern "C" typedef HRESULT (WINAPI *pDrawIndexedPrimitive)(
		LPDIRECT3DDEVICE9 pDevice,
		D3DPRIMITIVETYPE Type,
		INT BaseVertexIndex,
		UINT MinIndex,
		UINT NumVertices,
		UINT StartIndex,
		UINT PrimitiveCount
);
extern "C" typedef HRESULT (WINAPI *pSetIndices)(LPDIRECT3DDEVICE9 pDevice, IDirect3DIndexBuffer9 *pIndexData);
extern "C" typedef HRESULT (WINAPI *pSetStreamSource)(LPDIRECT3DDEVICE9 pDevice, UINT StreamNumber, 
													  IDirect3DVertexBuffer9 *pStreamData,
													  UINT OffsetInBytes,UINT Stride);
pDrawIndexedPrimitive DrawIndexedPrimitive;
pSetIndices SetIndices;
pSetStreamSource SetStreamSource;

bool DirExists(const std::string& dirName_in)
{
	DWORD ftyp = GetFileAttributesA(dirName_in.c_str());
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;  

	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
		return true;   

	return false;    
}

bool WINAPI Contains(long textureP)
{
	for (vector<long>::iterator iter = texturePointers.begin(); iter != texturePointers.end(); iter++)
	{
		if(*iter == textureP)
			return true;
	}
	return false;
}

string BinaryPath() 
{
	char buffer[MAX_PATH];
	ZeroMemory(buffer, MAX_PATH);
	GetModuleFileName( NULL, buffer, MAX_PATH );
	PathRemoveFileSpec(buffer);
	string filename(buffer);
	return filename;
}

HRESULT WINAPI MySetStreamSource(LPDIRECT3DDEVICE9 pDevice, UINT StreamNumber, IDirect3DVertexBuffer9 *pStreamData, 
								 UINT OffsetInBytes, UINT Stride)
{
	bound_vertices = pStreamData;  
	if(pStreamData != NULL)
		pStreamData->GetDesc(&vertex_desc);
	stream_number = StreamNumber;
	stride = Stride;
	return SetStreamSource(pDevice, StreamNumber, pStreamData, OffsetInBytes, Stride);
}

HRESULT WINAPI PopulateTriangleList(UINT PrimCount, string filename, UINT MinIndex, UINT NumVertices, WORD * indices, void * verts)
{
	int f = 0;

	for(int i = MinIndex; i < MinIndex+PrimCount; i++)
	{
		vertex v = GetVertex(indices[i], verts);
		if(!VertContains(v))
		{
			vertex_vec.push_back(v);
			out << "v " << v.x << " " << v.y << " " << v.z << endl;
		}

		vmap[indices[i]] = GetIndex(v);
	}

	for(int i = MinIndex; i < MinIndex+PrimCount; i++)
	{
		if(f % 3 == 0)
			out << endl << "f ";
		out << vmap[indices[i]] << " ";
		f++;
	}
	return 0;
}

bool VertContains(vertex v)
{
	for(vector<vertex>::iterator iter = vertex_vec.begin(); iter != vertex_vec.end(); iter++)
	{
		if(iter->x == v.x)
			if(iter->y == v.y)
				if(iter->z == v.z)
					return true;
	}
	return false;
}

HRESULT WINAPI PopulateTriangleStrip(UINT PrimCount, string filename, UINT MinIndex, UINT NumVertices, WORD * indices, void * verts)
{
	int f = 0;
	int count =0;

	for(int i = MinIndex; i < MinIndex+PrimCount; i++)
	{
		vertex v = GetVertex(indices[i], verts);
		if(!VertContains(v))
		{
			vertex_vec.push_back(v);
			out << "v " << v.x << " " << v.y << " " << v.z << endl;
		}

		vmap[indices[i]] = GetIndex(v);
	}

	for(int i = MinIndex; i < MinIndex+PrimCount; i++)
	{
		if(f % 3 == 0 && count <= 3)
			out << endl << "f ";

		if(count < 3)
		{
			out << vmap[indices[i]] << " ";
		}
		else
		{
			out << vmap[indices[i-2]] << " ";
			out << vmap[indices[i-1]] << " ";
			out << vmap[indices[i]] << endl << "f ";
		}
		f++;
		count++;
	}
	//out << "Face count= " << count << endl;

	return 0;
}

HRESULT WINAPI PopulateIndices(UINT PrimCount, UINT MinIndex, UINT NumVertices, INT BaseVertexIndex, D3DPRIMITIVETYPE Type, void * verts)
{
		if(bound_indices == NULL)
		{
			return 1;
		}
		void * d;
		bound_indices->Lock(0,PrimCount * 3 * sizeof(WORD), &d,0);
		WORD * indices = (WORD*)d;
		int f = 0;
		if(Type ==  D3DPT_TRIANGLELIST)
			PopulateTriangleList(PrimCount, "", MinIndex, NumVertices, indices, verts);
		else if(Type == D3DPT_TRIANGLESTRIP)
			PopulateTriangleStrip(PrimCount, "", MinIndex, NumVertices, indices, verts);
		bound_indices->Unlock();
		return 0;
}

bool VertexEquals(vertex v, vertex y)
{
	if(v.x == y.x)
		if(v.y == y.y)
			if(v.z == y.z)
				return true;
	return false;
}

int GetIndex(vertex v)
{
	for(int i = 0; i < vertex_vec.size(); i++)
	{
		if(VertexEquals(vertex_vec[i], v))
			return i;
	}
	return -1;
}

vertex GetVertex(WORD i, void * data)
{
	vertex v;
	if(bound_vertices != NULL)
	{
		float x, y, z;
		byte * vertices = (byte*)data;
		memcpy(&x, &vertices[i*stride], sizeof(float));
		memcpy(&y, &vertices[i*stride+sizeof(float)], sizeof(float));
		memcpy(&z, &vertices[i*stride+sizeof(float)*2], sizeof(float));
		v.x = x;
		v.y = y;
		v.z = z;
	}
	return v;
}

HRESULT WINAPI MyDrawIndexedPrimitive(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinIndex,
									  UINT NumVertices, UINT StartIndex, UINT PrimitiveCount)
{
	pDevice->GetIndices(&bound_indices);
	
	if(!dumped && PrimitiveCount == 2136 && NumVertices == 1469)
	//if(!dumped && PrimitiveCount == 6062 && NumVertices == 2517)
	{
		if(bound_vertices != NULL)
		{
			//ofstream out("C:\\Users\\emist\\Desktop\\index.txt");
			//out << "Stride= " << stride << endl;
			//out << "Primitive Type= " << Type << endl;
			void * data;
			bound_vertices->Lock(0,0, &data, 0);
			byte * vertices = (byte*)data;
			//out << "NumVerts= " << NumVertices << endl;
			//out << "MinIndex= " << MinIndex << endl;
			//out << "StartVertexIndex=" << StartIndex << endl;
			//out << "BaseVertexIndex= " << BaseVertexIndex << endl;
			PopulateIndices(PrimitiveCount, MinIndex, NumVertices, BaseVertexIndex, Type, data);
			bound_vertices->Unlock();
		}
		dumped = true;
	}	
	return DrawIndexedPrimitive(pDevice, Type, BaseVertexIndex, MinIndex, NumVertices, StartIndex, PrimitiveCount);
}

extern "C" __declspec(dllexport) void InstallHook(LPVOID message)
{
	PINIT_STRUCT messageStruct = reinterpret_cast<PINIT_STRUCT>(message);	  
	DrawIndexedPrimitive = (pDrawIndexedPrimitive)messageStruct->addresses[0];
	SetStreamSource = (pSetStreamSource)messageStruct->addresses[2];
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)DrawIndexedPrimitive, MyDrawIndexedPrimitive);
	DetourAttach(&(PVOID&)SetStreamSource, MySetStreamSource);
	DetourTransactionCommit();
}

BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call,LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		{
		dll = hModule;
		vertex f;
		f.x = -0.00000001f;
		f.y = -0.000000001f;
		f.z = -0.00000001f;
		vertex_vec.push_back(f);
		string output;
		output.append(BinaryPath());
		output.append("\\model.obj");
		out = ofstream(output);
		}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

