#pragma once
//#include "../Gateware.h"
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")
#include "d3dx12.h" // official helper file provided by microsoft

class Level_Data
{
public:
	struct light
	{
		GW::MATH::GVECTORF position;
		GW::MATH::GVECTORF color;
		float size;
	};

	struct SCENE_DATA
	{
		GW::MATH::GVECTORF sunDirection, sunColor, sunAmbient, camPos; //lighting info
		GW::MATH::GMATRIXF viewMatrix, projectionMatrix; //viewing info
		GW::MATH::GVECTORF padding[4];
	};

	struct MESH_DATA
	{
		GW::MATH::GMATRIXF world; //final world space transform
		H2B::ATTRIBUTES material; //Color/Texture of surface
		unsigned padding[28];
	};

	struct LIGHT_DATA
	{
		light lights[16];
		GW::MATH::GMATRIXF padding[3];
	};

	std::vector<std::vector<H2B::MATERIAL>> mats;
	std::vector<std::vector<H2B::VERTEX>> vertices;
	std::vector<std::vector<unsigned>> indices;
	std::vector<std::vector<H2B::MESH>> meshes;
	std::vector<std::vector<H2B::BATCH>> batches;
	std::vector<int> meshCount;
	std::vector<int> vertexCount;
	std::vector<int> indexCount;
	std::vector<light> lights;

	Microsoft::WRL::ComPtr<ID3D12Resource>	allVertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource>	allIndexBuffer;

	Microsoft::WRL::ComPtr<ID3D12Resource>	constantBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource>	lightConstantBuffer;

	// proxy handles

	GW::INPUT::GInput InputProxy;
	GW::INPUT::GController ControllerProxy;

	GW::MATH::GMatrix Matproxy;
	GW::MATH::GMATRIXF world;

	GW::MATH::GMATRIXF view;

	GW::MATH::GMATRIXF perspective;

	GW::MATH::GMATRIXF camera;

	GW::MATH::GVECTORF lightDir;
	GW::MATH::GVECTORF lightColor;

	std::chrono::high_resolution_clock::time_point start;
	std::chrono::high_resolution_clock::time_point end;
	bool running = false;
	float timeChange;

	float aspectRatio;

	std::vector<MESH_DATA> meshData;
	SCENE_DATA sceneData;
	LIGHT_DATA lightData;

	Microsoft::WRL::ComPtr <IDXGISwapChain4> swap;

	int activeFrames;

	int memory;

	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> descriptorHeap;

	UINT8* transferMemoryLocation3;
	UINT8* transferMemoryLocation4;

	D3D12_VERTEX_BUFFER_VIEW					vertexView;
	Microsoft::WRL::ComPtr<ID3D12Resource>		vertexBuffer;
	D3D12_INDEX_BUFFER_VIEW						indexView;
	Microsoft::WRL::ComPtr<ID3D12Resource>		indexBuffer;
	Microsoft::WRL::ComPtr<ID3D12RootSignature>	rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState>	pipeline;

	//Skybox

	Microsoft::WRL::ComPtr<ID3D12Resource>			textureResource;
	Microsoft::WRL::ComPtr<ID3D12Resource>			textureUpload;

	void ParseStuff()
	{
		H2B::Parser parser;
		openFile();
		for (int i = 0; i < names.size(); i++)
		{
			//if (names[i].find('.') == std::string::npos)
			//{
			if (parser.Parse(("../h2bs/" + names[i] + ".h2b").c_str()))
			{
				mats.push_back(parser.materials);
				vertices.push_back(parser.vertices);
				indices.push_back(parser.indices);
				meshes.push_back(parser.meshes);
				meshCount.push_back(parser.meshCount);
				batches.push_back(parser.batches);
				vertexCount.push_back(parser.vertexCount);
				indexCount.push_back(parser.indexCount);
			}
			else
			{
				std::cout << "../h2bs/" + names[i] + ".h2b" << ": Failed to parse data" << std::endl;
			}
			//}
		}
		for (int i = 0; i < lightColors.size(); i++)
		{
			lights.push_back({ lightWorldMatrices[i].row4, lightColors[i], size[i] });
		}
	}

	void LoadLevel(ID3D12Device* creator, GW::SYSTEM::GWindow win, GW::GRAPHICS::GDirectX12Surface d3d)
	{
		ParseStuff();
		transferMemoryLocation3 = 0;
		transferMemoryLocation4 = 0;

		Matproxy.Create();

		//MATRIX STUFF

		world = worldMatrices[0];

		Matproxy.LookAtLHF(GW::MATH::GVECTORF{ world.row4.x - 10,  world.row4.y - 10, world.row4.z - 10, 0 }, GW::MATH::GVECTORF{ world.row4.x,  world.row4.y, world.row4.z, 0 }, GW::MATH::GVECTORF{ 0, 1, 0, 0 }, view);
		sceneData.viewMatrix = view;
		Matproxy.InverseF(view, camera);

		Matproxy.ProjectionDirectXLHF(1.1345f, aspectRatio, 0.1, 500, perspective);
		sceneData.projectionMatrix = perspective;

		//SCENE DATA

		lightDir = GW::MATH::GVECTORF{ -1, -1, 2, 0 };
		GW::MATH::GVector::NormalizeF(lightDir, lightDir);
		lightColor = GW::MATH::GVECTORF{ 0.9f, 0.9f, 1, 1 };

		sceneData.sunColor = lightColor;
		sceneData.sunDirection = lightDir;
		sceneData.camPos = GW::MATH::GVECTORF{ world.row4.x - 10,  world.row4.y - 10, world.row4.z - 10, 0 };
		sceneData.sunAmbient = GW::MATH::GVECTORF{ 0.25f, 0.25f, 0.35f, 1 };

		//MESH DATA

		for (int i = 0; i < vertices.size(); i++)
		{
			MESH_DATA temp;
			if (names[i] == "SkyBox")
			{
				temp.world = worldMatrices[0];
			}
			else
			{
				temp.world = worldMatrices[i];
			}
			for (int j = 0; j < meshCount[i]; j++)
			{
				temp.material = mats[i][j].attrib;
				meshData.push_back(temp);
			}
		}

		//LIGHT DATA

		for (int i = 0; i < lights.size(); i++)
		{
			lightData.lights[i] = lights[i];
			//GW::MATH::GVector::NormalizeF(lightData.lights[i].position, lightData.lights[i].position);
		}


		//CREATE VERTEX BUFFER

		std::vector<H2B::VERTEX> verts;
		for (int i = 0; i < vertices.size(); i++)
		{
			for (int j = 0; j < vertices[i].size(); j++)
			{
				verts.push_back(vertices[i][j]);
			}
		}

		creator->CreateCommittedResource( // using UPLOAD heap for simplicity
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // DEFAULT recommend  
			D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(verts[0]) * verts.size()),
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer));
		// Transfer triangle data to the vertex buffer.
		UINT8* transferMemoryLocation;
		vertexBuffer->Map(0, &CD3DX12_RANGE(0, 0),
			reinterpret_cast<void**>(&transferMemoryLocation));
		memcpy(transferMemoryLocation, verts.data(), sizeof(verts[0]) * verts.size());
		vertexBuffer->Unmap(0, nullptr);

		// Create a vertex View to send to a Draw() call.
		vertexView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vertexView.StrideInBytes = sizeof(float) * 9;
		vertexView.SizeInBytes = sizeof(verts[0]) * verts.size();

		//CREATE INDEX BUFFER

		std::vector<unsigned int> index;
		for (int i = 0; i < indices.size(); i++)
		{
			for (int j = 0; j < indices[i].size(); j++)
			{
				index.push_back(indices[i][j]);
			}
		}

		creator->CreateCommittedResource( // using UPLOAD heap for simplicity
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // DEFAULT recommend  
			D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(index[0]) * index.size()),
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&indexBuffer));
		// Transfer triangle data to the index buffer.
		UINT8* transferMemoryLocation2;
		indexBuffer->Map(0, &CD3DX12_RANGE(0, 0),
			reinterpret_cast<void**>(&transferMemoryLocation2));
		memcpy(transferMemoryLocation2, index.data(), sizeof(index[0]) * index.size());
		indexBuffer->Unmap(0, nullptr);
		//Create an index View to send to a Draw() call.
		indexView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
		indexView.Format = DXGI_FORMAT_R32_UINT;
		indexView.SizeInBytes = sizeof(index[0]) * index.size();

		d3d.GetSwapchain4(&swap);
		DXGI_SWAP_CHAIN_DESC swap2;
		swap->GetDesc(&swap2);
		activeFrames = swap2.BufferCount;
		memory = (sizeof(sceneData) + (sizeof(meshData[0]) * meshData.size())) * activeFrames;

		//Mesh Constant Buffer
		creator->CreateCommittedResource( // using UPLOAD heap for simplicity
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // DEFAULT recommend  
			D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(memory),
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&constantBuffer));

		int memoryOffset = 0;

		for (int i = 0; i < activeFrames; i++)
		{
			constantBuffer->Map(0, &CD3DX12_RANGE(0, 0),
				reinterpret_cast<void**>(&transferMemoryLocation3));
			memcpy(transferMemoryLocation3 + memoryOffset, &sceneData, sizeof(sceneData));
			constantBuffer->Unmap(0, nullptr);
			for (int y = 0; y < meshData.size(); y++)
			{
				memoryOffset += 256;
				constantBuffer->Map(0, &CD3DX12_RANGE(0, 0),
					reinterpret_cast<void**>(&transferMemoryLocation3));
				memcpy(transferMemoryLocation3 + memoryOffset, &meshData[y], sizeof(meshData[y]));
				constantBuffer->Unmap(0, nullptr);

			}
			memoryOffset += 256;
		}


		//Light Constant Buffer
		creator->CreateCommittedResource( // using UPLOAD heap for simplicity
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // DEFAULT recommend  
			D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(lightData) * activeFrames),
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&lightConstantBuffer));

		memoryOffset = 0;

		for (int i = 0; i < activeFrames; i++)
		{
			lightConstantBuffer->Map(0, &CD3DX12_RANGE(0, 0),
				reinterpret_cast<void**>(&transferMemoryLocation4));
			memcpy(transferMemoryLocation4 + memoryOffset, &lightData, sizeof(lightData));
			lightConstantBuffer->Unmap(0, nullptr);
			memoryOffset += sizeof(lightData);
		}

		D3D12_DESCRIPTOR_HEAP_DESC heap = {};
		heap.NumDescriptors = 3;
		heap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		creator->CreateDescriptorHeap(&heap, IID_PPV_ARGS(&descriptorHeap));

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = constantBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = memory / 2;

		CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle0(descriptorHeap->GetCPUDescriptorHandleForHeapStart(), 0, 0);
		creator->CreateConstantBufferView(&cbvDesc, cbvHandle0);

		UINT compilerFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if _DEBUG
		compilerFlags |= D3DCOMPILE_DEBUG;
#endif
	}
};
