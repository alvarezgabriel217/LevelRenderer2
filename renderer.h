// minimalistic code to draw a single triangle, this is not part of the API.
// required for compiling shaders on the fly, consider pre-compiling instead
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")
#include "d3dx12.h" // official helper file provided by microsoft
#include <chrono>
#include <ctime>


//#define GATEWARE_PROXY_CLASS(GMatrix)

// Simple Vertex Shader
const char* vertexShaderSource = R"(
// an ultra simple hlsl vertex shader
#pragma pack_matrix( row_major )


struct OBJ_ATTRIBUTES
{
	float3 Kd; 
	float d;
	float3 Ks; 
	float Ns;
	float3 Ka; 
	float sharpness;
	float3 Tf; 
	float Ni;
	float3 Ke; 
	uint illum;

};

struct SCENE_DATA
{
	float4 sunDirection;
	float4 sunColor;
	float4 sunAmbient;
	float4 camPos;
	float4x4 viewMatrix;
	float4x4 projectionMatrix;
	float4 padding[4];
};

struct MESH_DATA
{
	float4x4 world;
	OBJ_ATTRIBUTES material;
	uint padding[28];
};

ConstantBuffer<SCENE_DATA> cameraAndLights : register(b0, Space0);
ConstantBuffer<MESH_DATA> meshInfo : register(b1, Space0);

//cbuffer SHADER_VARS
//{
//	float4x4 worldMatrix;
//	float4x4 viewMatrix;
//	float4x4 perspectiveMatrix;
//}

struct OUTPUT_TO_RASTERIZER
{
	float4 posH : SV_POSITION; // Homogenous projection space
	float3 nrW : NORMALS; // Normal in world space (for lighting)
	float3 posW : WORLD; // Position in world space (for lighting)
};

struct Vertex
{
		float3 position : POSITION;
		float3 uvw : UVW;
		float3 normals : NORMALS;
};


OUTPUT_TO_RASTERIZER main(Vertex inputVertex)
{
	OUTPUT_TO_RASTERIZER output;
	float4 newVertex = float4(inputVertex.position, 1);
	newVertex = mul(newVertex, meshInfo.world);
	output.posW = newVertex;
	newVertex = mul(newVertex, cameraAndLights.viewMatrix);
	newVertex = mul(newVertex, cameraAndLights.projectionMatrix);
	output.posH = newVertex;
	output.nrW = mul(inputVertex.normals, (float3x3)meshInfo.world);

	return output;
}
)";


// Simple Pixel Shader
const char* pixelShaderSource = R"(
#pragma pack_matrix( row_major )
// an ultra simple hlsl pixel shader

struct OBJ_ATTRIBUTES
{
	float3 Kd; 
	float d;
	float3 Ks; 
	float Ns;
	float3 Ka; 
	float sharpness;
	float3 Tf; 
	float Ni;
	float3 Ke; 
	uint illum;

};

struct SCENE_DATA
{
	float4 sunDirection;
	float4 sunColor;
	float4 sunAmbient;
	float4 camPos;
	float4x4 viewMatrix;
	float4x4 projectionMatrix;
	float4 padding[4];
};

struct MESH_DATA
{
	float4x4 world;
	OBJ_ATTRIBUTES material;
	uint padding[28];
};

ConstantBuffer<SCENE_DATA> cameraAndLights : register(b0, Space0);
ConstantBuffer<MESH_DATA> meshInfo : register(b1, Space0);

struct OUTPUT_TO_RASTERIZER
{
	float4 posH : SV_POSITION; // Homogenous projection space
	float3 nrW : NORMALS; // Normal in world space (for lighting)
	float3 posW : WORLD; // Position in world space (for lighting)
};

float4 main(OUTPUT_TO_RASTERIZER input) : SV_TARGET 
{	
	input.nrW = normalize(input.nrW);
	float3 sunDic = normalize(cameraAndLights.sunDirection.xyz);
	float lightRatio = saturate(dot(-sunDic, input.nrW));
	lightRatio = saturate(lightRatio + cameraAndLights.sunAmbient);
	float4 result = lightRatio * cameraAndLights.sunColor * float4(meshInfo.material.Kd, 1);

	float3 viewDir = normalize(cameraAndLights.camPos.xyz - input.posW.xyz);
	float3 halfVector = normalize(-sunDic + viewDir);
	float intensity = max(pow(saturate(dot(input.nrW, halfVector)), (meshInfo.material.Ns + 0.00001f)), 0);

	float4 reflectedLight = cameraAndLights.sunColor * float4(meshInfo.material.Ks, 0) * intensity;	

	return result + reflectedLight;
}
)";
// Creation, Rendering & Cleanup
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
class Renderer
{
	Level_Data levelData;

	// proxy handles
	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GDirectX12Surface d3d;

	GW::INPUT::GInput InputProxy;
	GW::INPUT::GController ControllerProxy;

	GW::MATH::GMatrix Matproxy;
	GW::MATH::GMATRIXF world;

	GW::MATH::GMATRIXF world2;
	GW::MATH::GMATRIXF world3;
	GW::MATH::GMATRIXF world4;
	GW::MATH::GMATRIXF world5;
	GW::MATH::GMATRIXF world6;

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

	Microsoft::WRL::ComPtr <IDXGISwapChain4> swap;

	int activeFrames;

	int memory;

	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> descriptorHeap;

	UINT8* transferMemoryLocation3;

	D3D12_VERTEX_BUFFER_VIEW					vertexView;
	Microsoft::WRL::ComPtr<ID3D12Resource>		vertexBuffer;
	D3D12_INDEX_BUFFER_VIEW						indexView;
	Microsoft::WRL::ComPtr<ID3D12Resource>		indexBuffer;
	Microsoft::WRL::ComPtr<ID3D12RootSignature>	rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState>	pipeline;
public:

	/*struct Vertex
	{
		GW::MATH::GVECTORF position;
		GW::MATH::GVECTORF uvw;
		GW::MATH::GVECTORF normals;
	};*/

	/*struct SHADER_VARS
	{
		GW::MATH::GMATRIXF matrix;
		GW::MATH::GMATRIXF matrix2;
		GW::MATH::GMATRIXF matrix3;
	};*/

	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GDirectX12Surface _d3d)
	{
		levelData.ParseStuff();

		win = _win;
		d3d = _d3d;
		ID3D12Device* creator;
		d3d.GetDevice((void**)&creator);

		InputProxy.Create(_win);
		ControllerProxy.Create();

		Matproxy.Create();

		//MATRIX STUFF

		world = worldMatrices[11];
		//world.row4.x = world.row4.x / 10;
		//world.row4.y = world.row4.y / 10;
		//world.row4.z = world.row4.z / 10;
		//world.row4.w = world.row4.w / 10;
		//Matproxy.IdentityF(world);
		//Matproxy.TranslateLocalF(world, GW::MATH::GVECTORF{0, -0.5f, 1}, world);
		//Matproxy.InverseF(world, world);

		//Matproxy.ScaleLocalF(world, GW::MATH::GVECTORF{ 1, 1, 1, 1 }, world);


		meshData[0].world = world;
		meshData[1].world = world;
		meshData[2].world = world;
		meshData[3].world = world;
		meshData[4].world = world;
		meshData[5].world = world;
		meshData[6].world = world;

		Matproxy.LookAtLHF(GW::MATH::GVECTORF{ world.row4.x,  world.row4.y, world.row4.z, 0 }, GW::MATH::GVECTORF{ world.row4.x,  world.row4.y, world.row4.z, 0 }, GW::MATH::GVECTORF{ 0, 1, 0, 0 }, view);
		sceneData.viewMatrix = view;
		Matproxy.InverseF(view, camera);




		_d3d.GetAspectRatio(aspectRatio);

		Matproxy.ProjectionDirectXLHF(1.1345f, aspectRatio, 0.1, 100, perspective);
		sceneData.projectionMatrix = perspective;

		//SCENE DATA

		lightDir = GW::MATH::GVECTORF{ -1, -1, 2, 0 };
		GW::MATH::GVector::NormalizeF(lightDir, lightDir);
		lightColor = GW::MATH::GVECTORF{ 0.9f, 0.9f, 1, 1 };

		sceneData.sunColor = lightColor;
		sceneData.sunDirection = lightDir;
		sceneData.camPos = GW::MATH::GVECTORF{ 0.75f, 0.25f, -1.5f, 0 };
		sceneData.sunAmbient = GW::MATH::GVECTORF{ 0.25f, 0.25f, 0.35f, 1 };

		//MESH DATA

		for (int i = 0; i < levelData.meshCount[0]; i++)
		{
			meshData.push_back(MESH_DATA{ world, levelData.mats[0][i].attrib});
		}

		/*meshData[0].world = world;
		meshData[1].world = world;



		meshData[0].material = levelData.mats[0][0].attrib;
		meshData[1].material = levelData.mats[0][0].attrib;*/
		/*world2 = worldMatrices[11];
		world2.row4.x = world2.row4.x / 10;
		world2.row4.y = world2.row4.y / 10;
		world2.row4.z = world2.row4.z / 10;

		world3 = worldMatrices[12];
		world3.row4.x = world3.row4.x / 10;
		world3.row4.y = world3.row4.y / 10;
		world3.row4.z = world3.row4.z / 10;

		world4 = worldMatrices[13];
		world4.row4.x = world4.row4.x / 10;
		world4.row4.y = world4.row4.y / 10;
		world4.row4.z = world4.row4.z / 10;

		world5 = worldMatrices[14];
		world5.row4.x = world5.row4.x / 10;
		world5.row4.y = world5.row4.y / 10;
		world5.row4.z = world5.row4.z / 10;*/


		//world6 = worldMatrices[15];
		//world6.row4.x = world6.row4.x / 10;
		//world6.row4.y = world6.row4.y / 10;
		//world6.row4.z = world6.row4.z / 10;

		/*Matproxy.IdentityF(world);
		Matproxy.RotateXLocalF(world, 1.5708, world);
		Matproxy.TranslateGlobalF(world, GW::MATH::GVECTORF{ 0, -0.5f, 0, 0 }, world);


		Matproxy.IdentityF(world2);
		Matproxy.RotateXLocalF(world2, 1.5708, world2);
		Matproxy.TranslateGlobalF(world2, GW::MATH::GVECTORF{ 0, 0.5f, 0, 0 }, world2);

		Matproxy.IdentityF(world3);
		Matproxy.RotateYLocalF(world3, 1.5708, world3);
		Matproxy.TranslateGlobalF(world3, GW::MATH::GVECTORF{ 0.5f, 0, 0, 0 }, world3);

		Matproxy.IdentityF(world4);
		Matproxy.RotateYLocalF(world4, 1.5708, world4);
		Matproxy.TranslateGlobalF(world4, GW::MATH::GVECTORF{ -0.5f, 0, 0, 0 }, world4);

		Matproxy.IdentityF(world5);
		Matproxy.TranslateLocalF(world5, GW::MATH::GVECTORF{ 0, 0, 0.5f, 0 }, world5);

		Matproxy.IdentityF(world6);
		Matproxy.TranslateLocalF(world6, GW::MATH::GVECTORF{ 0, 0, -0.5f, 0 }, world6);*/


		//Vertex verts[4568];
		H2B::VERTEX verts[4568];

		/*for (int i = 0; i <= 100; i += 4)
		{
			verts[i] = { -0.5f + (i * 0.01f), 0.5f, 0, 1 };
			verts[i + 1] = { -0.5f + (i * 0.01f), -0.5f, 0, 1 };
			verts[i + 2] = { -0.5f, -0.5f + (i * 0.01f), 0, 1 };
			verts[i + 3] = { 0.5f, -0.5f + (i * 0.01f), 0, 1 };
		}*/

		/*for (int i = 0; i < 4568; i++)
		{
			verts[i].position = GW::MATH::GVECTORF{ levelData.vertices[0][i].pos.x, levelData.vertices[0][i].pos.y, levelData.vertices[0][i].pos.z };
			verts[i].uvw = GW::MATH::GVECTORF{ levelData.vertices[0][i].uvw.x, levelData.vertices[0][i].uvw.y, levelData.vertices[0][i].uvw.z };
			verts[i].normals = GW::MATH::GVECTORF{ levelData.vertices[0][i].nrm.x, levelData.vertices[0][i].nrm.y, levelData.vertices[0][i].nrm.z };
		}*/

		for (int i = 0; i < 4568; i++)
		{
			verts[i] = levelData.vertices[0][i];
		}

		creator->CreateCommittedResource( // using UPLOAD heap for simplicity
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // DEFAULT recommend  
			D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(verts)),
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer));
		// Transfer triangle data to the vertex buffer.
		UINT8* transferMemoryLocation;
		vertexBuffer->Map(0, &CD3DX12_RANGE(0, 0),
			reinterpret_cast<void**>(&transferMemoryLocation));
		memcpy(transferMemoryLocation, verts, sizeof(verts));
		vertexBuffer->Unmap(0, nullptr);

		// Create a vertex View to send to a Draw() call.
		vertexView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vertexView.StrideInBytes = sizeof(float) * 9;
		vertexView.SizeInBytes = sizeof(verts);

		unsigned int index[10080];

		for (int i = 0; i < 10080; i++)
		{
			index[i] = levelData.indices[0][i];
		}

		creator->CreateCommittedResource( // using UPLOAD heap for simplicity
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // DEFAULT recommend  
			D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(index)),
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&indexBuffer));
		// Transfer triangle data to the index buffer.
		UINT8* transferMemoryLocation2;
		indexBuffer->Map(0, &CD3DX12_RANGE(0, 0),
			reinterpret_cast<void**>(&transferMemoryLocation2));
		memcpy(transferMemoryLocation2, index, sizeof(index));
		indexBuffer->Unmap(0, nullptr);
		//Create an index View to send to a Draw() call.
		indexView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
		indexView.Format = DXGI_FORMAT_R32_UINT;
		indexView.SizeInBytes = sizeof(index);

		d3d.GetSwapchain4(&swap);
		DXGI_SWAP_CHAIN_DESC swap2;
		swap->GetDesc(&swap2);
		activeFrames = swap2.BufferCount;
		memory = (sizeof(sceneData) + (sizeof(meshData[0]) * meshData.size())) * activeFrames;

		creator->CreateCommittedResource( // using UPLOAD heap for simplicity
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // DEFAULT recommend  
			D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(memory),
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&levelData.constantBuffer));
		for (int i = 0; i < activeFrames; i++)
		{
			for (int x = 0; x < levelData.meshCount[0]; x++)
			{
				levelData.constantBuffer->Map(0, &CD3DX12_RANGE(0, 0),
					reinterpret_cast<void**>(&transferMemoryLocation3));
				//std::cout << sizeof(meshData);
				memcpy(transferMemoryLocation3 + (memory / 2 * i), &sceneData, sizeof(sceneData));
				levelData.constantBuffer->Unmap(0, nullptr);
				levelData.constantBuffer->Map(0, &CD3DX12_RANGE(0, 0),
					reinterpret_cast<void**>(&transferMemoryLocation3));
				memcpy(transferMemoryLocation3 + (256 * (x + 1) + (memory / 2 * i)), &meshData[x], sizeof(meshData[x]));
				//memcpy(transferMemoryLocation3 + 512, &meshData[1], sizeof(meshData[1]));
				levelData.constantBuffer->Unmap(0, nullptr);
			}
		}
		//constantBuffer->Unmap(0, nullptr);

		// TODO: Part 2e
		D3D12_DESCRIPTOR_HEAP_DESC heap = {};
		heap.NumDescriptors = 2;
		heap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		creator->CreateDescriptorHeap(&heap, IID_PPV_ARGS(&descriptorHeap));

		// TODO: Part 2f
		//meshSize = sizeof(meshData);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = levelData.constantBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = sizeof(sceneData);

		CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle0(descriptorHeap->GetCPUDescriptorHandleForHeapStart(), 0, 0);
		creator->CreateConstantBufferView(&cbvDesc, cbvHandle0);

		// Create Vertex Shader
		UINT compilerFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if _DEBUG
		compilerFlags |= D3DCOMPILE_DEBUG;
#endif
		Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, errors;
		if (FAILED(D3DCompile(vertexShaderSource, strlen(vertexShaderSource),
			nullptr, nullptr, nullptr, "main", "vs_5_1", compilerFlags, 0,
			vsBlob.GetAddressOf(), errors.GetAddressOf())))
		{
			std::cout << (char*)errors->GetBufferPointer() << std::endl;
			abort();
		}
		// Create Pixel Shader
		Microsoft::WRL::ComPtr<ID3DBlob> psBlob; errors.Reset();
		if (FAILED(D3DCompile(pixelShaderSource, strlen(pixelShaderSource),
			nullptr, nullptr, nullptr, "main", "ps_5_1", compilerFlags, 0,
			psBlob.GetAddressOf(), errors.GetAddressOf())))
		{
			std::cout << (char*)errors->GetBufferPointer() << std::endl;
			abort();
		}

		// Create Input Layout
		D3D12_INPUT_ELEMENT_DESC format[] = {
			{
				"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
				D3D12_APPEND_ALIGNED_ELEMENT,
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
			},
			{
				"UVW", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
				D3D12_APPEND_ALIGNED_ELEMENT,
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
			},
			{
				"NORMALS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
				D3D12_APPEND_ALIGNED_ELEMENT,
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
			},
			{
				"WORLD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
				D3D12_APPEND_ALIGNED_ELEMENT,
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
			}
		};

		//Create root parameters
		//rootParam[0].InitAsConstants(48, 0);
		CD3DX12_ROOT_PARAMETER rootParams[2];
		rootParams[0].InitAsConstantBufferView(0, 0);
		rootParams[1].InitAsConstantBufferView(1, 0);

		// create root signature
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(2, rootParams, 0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		Microsoft::WRL::ComPtr<ID3DBlob> signature;
		D3D12SerializeRootSignature(&rootSignatureDesc,
			D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errors);
		creator->CreateRootSignature(0, signature->GetBufferPointer(),
			signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
		// create pipeline state
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psDesc;
		ZeroMemory(&psDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		psDesc.InputLayout = { format, ARRAYSIZE(format) };
		psDesc.pRootSignature = rootSignature.Get();
		psDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
		psDesc.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());
		psDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psDesc.SampleMask = UINT_MAX;
		psDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psDesc.NumRenderTargets = 1;
		psDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psDesc.SampleDesc.Count = 1;
		creator->CreateGraphicsPipelineState(&psDesc, IID_PPV_ARGS(&pipeline));
		// free temporary handle
		creator->Release();
	}

	void Render()
	{
		// grab the context & render target

		sceneData.viewMatrix = view;
		levelData.constantBuffer->Map(0, &CD3DX12_RANGE(0, 0),
			reinterpret_cast<void**>(&transferMemoryLocation3));
		memcpy(transferMemoryLocation3, &sceneData, sizeof(SCENE_DATA));
		levelData.constantBuffer->Unmap(0, nullptr);

		ID3D12GraphicsCommandList* cmd;
		D3D12_CPU_DESCRIPTOR_HANDLE rtv;
		D3D12_CPU_DESCRIPTOR_HANDLE dsv;
		d3d.GetCommandList((void**)&cmd);
		d3d.GetCurrentRenderTargetView((void**)&rtv);
		d3d.GetDepthStencilView((void**)&dsv);
		// setup the pipeline
		cmd->SetGraphicsRootSignature(rootSignature.Get());
		ID3D12DescriptorHeap* ppHeaps[] = { descriptorHeap.Get() };
		cmd->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
		cmd->SetGraphicsRootConstantBufferView(0, levelData.constantBuffer->GetGPUVirtualAddress());
		cmd->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
		cmd->SetPipelineState(pipeline.Get());
		// now we can draw
		cmd->IASetVertexBuffers(0, 1, &vertexView);
		cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmd->IASetIndexBuffer(&indexView);

		for (int i = 0; i < activeFrames; i++)
		{
			for (int x = 0; x < levelData.meshCount[0]; x++)
			{
				cmd->SetGraphicsRootConstantBufferView(1, levelData.constantBuffer->GetGPUVirtualAddress() + (256 * (x + 1) + (memory / 2 * i)));
				cmd->DrawIndexedInstanced(levelData.meshes[0][x].drawInfo.indexCount, 1, levelData.meshes[0][x].drawInfo.indexOffset, 0, 0);

			}
		}
		// release temp handles
		cmd->Release();
	}

	//void Render()
	//{
	//	// grab the context & render target
	//	ID3D12GraphicsCommandList* cmd;
	//	D3D12_CPU_DESCRIPTOR_HANDLE rtv;
	//	D3D12_CPU_DESCRIPTOR_HANDLE dsv;
	//	d3d.GetCommandList((void**)&cmd);
	//	d3d.GetCurrentRenderTargetView((void**)&rtv);
	//	d3d.GetDepthStencilView((void**)&dsv);
	//	// setup the pipeline
	//	cmd->SetGraphicsRootSignature(rootSignature.Get());

	//	ID3D12DescriptorHeap* ppHeaps[] = { descriptorHeap.Get()};
	//	cmd->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	//	cmd->SetGraphicsRootConstantBufferView(1, levelData.constantBuffer->GetGPUVirtualAddress());
	//	cmd->SetGraphicsRootConstantBufferView(2, levelData.constantBuffer->GetGPUVirtualAddress() + sizeof(SCENE_DATA));

	//	cmd->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
	//	cmd->SetPipelineState(pipeline.Get());


	//	/*SHADER_VARS vars;
	//	vars.matrix = world;
	//	vars.matrix2 = view;
	//	vars.matrix3 = perspective;*/

	//	//cmd->SetGraphicsRoot32BitConstants(0, 48, &vars, 0);
	//	// now we can draw
	//	cmd->IASetVertexBuffers(0, 1, &vertexView);
	//	cmd->IASetIndexBuffer(&indexView);

	//	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//	/*cmd->DrawInstanced(104, 1, 0, 0);
	//	vars.matrix = world2;
	//	cmd->SetGraphicsRoot32BitConstants(0, 48, &vars, 0);
	//	cmd->DrawInstanced(104, 1, 0, 0);

	//	vars.matrix = world3;
	//	cmd->SetGraphicsRoot32BitConstants(0, 48, &vars, 0);
	//	cmd->DrawInstanced(104, 1, 0, 0);

	//	vars.matrix = world4;
	//	cmd->SetGraphicsRoot32BitConstants(0, 48, &vars, 0);
	//	cmd->DrawInstanced(104, 1, 0, 0);

	//	vars.matrix = world5;
	//	cmd->SetGraphicsRoot32BitConstants(0, 48, &vars, 0);
	//	cmd->DrawInstanced(104, 1, 0, 0);

	//	vars.matrix = world6;
	//	cmd->SetGraphicsRoot32BitConstants(0, 48, &vars, 0);
	//	cmd->DrawInstanced(104, 1, 0, 0);*/

	//	//cmd->DrawIndexedInstanced(10080, 1, 0, 0, 0);

	//	//cmd->SetGraphicsRootConstantBufferView(1, levelData.constantBuffer->GetGPUVirtualAddress());

	//	/*for (int i = 0; i < activeFrames; i++)
	//	{
	//		for (int x = 0; x < activeFrames; x++)
	//		{

	//			cmd->SetGraphicsRootConstantBufferView(1, levelData.constantBuffer->GetGPUVirtualAddress() + (256 * (x + 1) + (memory / 2 * i)));
	//		}
	//	}
	//		*/	cmd->DrawIndexedInstanced(10080, 1, 0, 0, 0);

	//	//vars.matrix = world2;
	//	//cmd->SetGraphicsRoot32BitConstants(0, 48, &vars, 0);
	//	//cmd->DrawIndexedInstanced(10080, 1, 0, 0, 0);

	//	////vars.matrix = world3;
	//	//cmd->SetGraphicsRoot32BitConstants(0, 48, &vars, 0);
	//	//cmd->DrawIndexedInstanced(10080, 1, 0, 0, 0);

	//	////vars.matrix = world4;
	//	//cmd->SetGraphicsRoot32BitConstants(0, 48, &vars, 0);
	//	//cmd->DrawIndexedInstanced(10080, 1, 0, 0, 0);

	//	////vars.matrix = world5;
	//	//cmd->SetGraphicsRoot32BitConstants(0, 48, &vars, 0);
	//	//cmd->DrawIndexedInstanced(10080, 1, 0, 0, 0);

	//	//vars.matrix = world6;
	//	//cmd->SetGraphicsRoot32BitConstants(0, 48, &vars, 0);
	//	//cmd->DrawIndexedInstanced(10080, 1, 0, 0, 0);

	//	// release temp handles
	//	cmd->Release();
	//}

	void UpdateCamera()
	{
		if (running)
		{
			end = std::chrono::high_resolution_clock::now();
			running = false;
			timeChange = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
		}
		else
		{
			timeChange = 0;
		}
		if (!running)
		{
			start = std::chrono::high_resolution_clock::now();
			running = true;
		}

		const float Camera_speed = 0.3f;

		float Total_Y_Change = 0;
		float Total_X_Change = 0;
		float Total_Z_Change = 0;
		float PerFrameSpeed = Camera_speed * timeChange / 100;

		unsigned int height;
		unsigned int width;

		float space = 0;
		float wKey = 0;
		float sKey = 0;
		float aKey = 0;
		float dKey = 0;
		float leftShift = 0;
		float mouseY = 0;
		float mouseX = 0;
		float leftStickY = 0;
		float leftStickX = 0;
		float rightStickY = 0;
		float rightStickX = 0;
		float rightTrigger = 0;
		float leftTrigger = 0;

		InputProxy.GetState(23, space);
		InputProxy.GetState(14, leftShift);
		InputProxy.GetState(60, wKey);
		InputProxy.GetState(56, sKey);
		InputProxy.GetState(41, dKey);
		InputProxy.GetState(38, aKey);

		GW::GReturn result = InputProxy.GetMouseDelta(mouseX, mouseY);

		ControllerProxy.GetState(0, 7, rightTrigger);
		ControllerProxy.GetState(0, 6, leftTrigger);
		ControllerProxy.GetState(0, 17, leftStickY);
		ControllerProxy.GetState(0, 16, leftStickX);
		ControllerProxy.GetState(0, 19, rightStickY);
		ControllerProxy.GetState(0, 18, rightStickX);

		win.GetHeight(height);
		win.GetWidth(width);

		Total_Y_Change = space - leftShift + rightTrigger - leftTrigger;
		Total_Z_Change = wKey - sKey + leftStickY;
		Total_X_Change = dKey - aKey + leftStickX;

		float Thumb_Speed = 3.1415927 * timeChange / 60;

		float Total_Pitch = 1.1345f * mouseY / height + rightStickY * -Thumb_Speed;
		float Total_Yaw = 1.1345f * aspectRatio * mouseX / width + rightStickX * Thumb_Speed;

		Matproxy.TranslateGlobalF(camera, GW::MATH::GVECTORF{ 0, Total_Y_Change * PerFrameSpeed, 0, 0 }, camera);
		Matproxy.TranslateLocalF(camera, GW::MATH::GVECTORF{ Total_X_Change * PerFrameSpeed, 0, Total_Z_Change * PerFrameSpeed, 0 }, camera);
		if (result != GW::GReturn::REDUNDANT)
		{
			Matproxy.RotateXLocalF(camera, Total_Pitch, camera);
			Matproxy.RotateYGlobalF(camera, Total_Yaw, camera);
		}

		Matproxy.InverseF(camera, view);
	}
	~Renderer()
	{
		// ComPtr will auto release so nothing to do here 
	}
};
