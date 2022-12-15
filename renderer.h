// minimalistic code to draw a single triangle, this is not part of the API.
// required for compiling shaders on the fly, consider pre-compiling instead
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")
#include "d3dx12.h" // official helper file provided by microsoft
#include <chrono>
#include <ctime>
#include <commdlg.h>


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

struct light
{
	float4 position;
	float4 color;
	float size;
};

struct light2
{
	float4x4 lightData;
};

struct LIGHT_DATA
{
	light lights[7];
	//float4x4 padding[3];
};

struct LIGHT_DATA2
{
	light2 lights[16];
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
ConstantBuffer<LIGHT_DATA2> lightInfo : register(b2, Space0);

struct OUTPUT_TO_RASTERIZER
{
	float4 posH : SV_POSITION; // Homogenous projection space
	float3 nrW : NORMALS; // Normal in world space (for lighting)
	float3 posW : WORLD; // Position in world space (for lighting)
};

float4 main(OUTPUT_TO_RASTERIZER input) : SV_TARGET 
{	
	//DIRECTIONAL LIGHTING
	input.nrW = normalize(input.nrW);
	float3 sunDic = normalize(cameraAndLights.sunDirection.xyz);
	float lightRatio = saturate(dot(-sunDic, input.nrW));
	lightRatio = saturate(lightRatio + cameraAndLights.sunAmbient);
	float4 directional = lightRatio * cameraAndLights.sunColor;

	//SPECULAR REFLECTION
	float3 viewDir = normalize(cameraAndLights.camPos.xyz - input.posW.xyz);
	float3 halfVector = normalize(-sunDic + viewDir);
	float intensity = max(pow(saturate(dot(input.nrW, halfVector)), (meshInfo.material.Ns + 0.00001f)), 0);

	float4 specular = cameraAndLights.sunColor * float4(meshInfo.material.Ks, 0) * intensity;	

	//POINT LIGHTING
	float4 pointSum = (0, 0, 0, 0);
	for(int i = 0; i < 16; i++)
	{
		if(lightInfo.lights[i].lightData[2].x > 0)
		{

			float3 lightDirection = normalize(lightInfo.lights[i].lightData[3].xyz - input.posW.xyz);
			float lightRatio2 = saturate(dot(lightDirection, input.nrW));
			float attenuation = 1.0f - saturate(length(lightInfo.lights[i].lightData[3].xyz - input.posW.xyz) / (lightInfo.lights[i].lightData[2].x * 10));
			lightRatio2 *= pow(attenuation, 2);
			pointSum += lightRatio2 * lightInfo.lights[i].lightData[0];
		}
	}

	return (directional + pointSum) * float4(meshInfo.material.Kd, 1) + specular;
}
)";
// Creation, Rendering & Cleanup
struct LIGHT_DATA
{
	Level_Data::light lights[16];
	//GW::MATH::GMATRIXF padding[3];
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
class Renderer
{
	Level_Data levelData;

	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GDirectX12Surface d3d;

	//Proxies
	GW::INPUT::GInput InputProxy;
	GW::INPUT::GController ControllerProxy;
	GW::MATH::GMatrix Matproxy;

	ID3D12Device* creator;
public:
	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GDirectX12Surface _d3d)
	{
		//levelData.ParseStuff();

		win = _win;
		d3d = _d3d;

		d3d.GetDevice((void**)&creator);

		InputProxy.Create(_win);
		ControllerProxy.Create();

		_d3d.GetAspectRatio(levelData.aspectRatio);

		levelData.LoadLevel(creator, win, d3d);

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

		CD3DX12_ROOT_PARAMETER rootParams[3];
		rootParams[0].InitAsConstantBufferView(0, 0);
		rootParams[1].InitAsConstantBufferView(1, 0);
		rootParams[2].InitAsConstantBufferView(2, 0);

		// create root signature
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(3, rootParams, 0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		Microsoft::WRL::ComPtr<ID3DBlob> signature;
		D3D12SerializeRootSignature(&rootSignatureDesc,
			D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errors);
		creator->CreateRootSignature(0, signature->GetBufferPointer(),
			signature->GetBufferSize(), IID_PPV_ARGS(&levelData.rootSignature));
		// create pipeline state
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psDesc;
		ZeroMemory(&psDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		psDesc.InputLayout = { format, ARRAYSIZE(format) };
		psDesc.pRootSignature = levelData.rootSignature.Get();
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

		creator->CreateGraphicsPipelineState(&psDesc, IID_PPV_ARGS(&levelData.pipeline));
		// free temporary handle
		creator->Release();
	}

	void Render()
	{
		// grab the context & render target

		levelData.sceneData.viewMatrix = levelData.view;
		levelData.constantBuffer->Map(0, &CD3DX12_RANGE(0, 0),
			reinterpret_cast<void**>(&levelData.transferMemoryLocation3));
		memcpy(levelData.transferMemoryLocation3, &levelData.sceneData, sizeof(SCENE_DATA));
		levelData.constantBuffer->Unmap(0, nullptr);

		ID3D12GraphicsCommandList* cmd;
		D3D12_CPU_DESCRIPTOR_HANDLE rtv;
		D3D12_CPU_DESCRIPTOR_HANDLE dsv;
		d3d.GetCommandList((void**)&cmd);
		d3d.GetCurrentRenderTargetView((void**)&rtv);
		d3d.GetDepthStencilView((void**)&dsv);
		// setup the pipeline
		cmd->SetGraphicsRootSignature(levelData.rootSignature.Get());
		ID3D12DescriptorHeap* ppHeaps[] = { levelData.descriptorHeap.Get() };
		cmd->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
		cmd->SetGraphicsRootConstantBufferView(0, levelData.constantBuffer->GetGPUVirtualAddress());
		cmd->SetGraphicsRootConstantBufferView(2, levelData.lightConstantBuffer->GetGPUVirtualAddress());
		cmd->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
		cmd->SetPipelineState(levelData.pipeline.Get());
		// now we can draw
		cmd->IASetVertexBuffers(0, 1, &levelData.vertexView);
		cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmd->IASetIndexBuffer(&levelData.indexView);

		int vertOffset = 0;
		int indexOffset = 0;
		int memoryOffset = 0;
		for (int i = 0; i < levelData.activeFrames; i++)
		{
			for (int y = 0; y < levelData.vertices.size(); y++)
			{
				for (int x = 0; x < levelData.meshCount[y]; x++)
				{
					memoryOffset += 256;
					if (y != levelData.skyboxIndex)
					{
						cmd->SetGraphicsRootConstantBufferView(1, levelData.constantBuffer->GetGPUVirtualAddress() + memoryOffset);
						cmd->DrawIndexedInstanced(levelData.meshes[y][x].drawInfo.indexCount, 1, levelData.meshes[y][x].drawInfo.indexOffset + indexOffset, vertOffset, 0);
					}

				}
				indexOffset += levelData.indexCount[y];
				vertOffset += levelData.vertexCount[y];
			}
			memoryOffset += 256;
		}
		// release temp handles
		cmd->Release();
	}

	void UpdateCamera()
	{
		if (levelData.running)
		{
			levelData.end = std::chrono::high_resolution_clock::now();
			levelData.running = false;
			levelData.timeChange = std::chrono::duration_cast<std::chrono::milliseconds>(levelData.end - levelData.start).count();
		}
		else
		{
			levelData.timeChange = 0;
		}
		if (!levelData.running)
		{
			levelData.start = std::chrono::high_resolution_clock::now();
			levelData.running = true;
		}

		const float Camera_speed = 0.3f;

		float Total_Y_Change = 0;
		float Total_X_Change = 0;
		float Total_Z_Change = 0;
		float PerFrameSpeed = Camera_speed * levelData.timeChange / 10;

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

		float Thumb_Speed = 3.1415927 * levelData.timeChange / 60;

		float Total_Pitch = 1.1345f * mouseY / height + rightStickY * -Thumb_Speed;
		float Total_Yaw = 1.1345f * levelData.aspectRatio * mouseX / width + rightStickX * Thumb_Speed;

		Matproxy.TranslateGlobalF(levelData.camera, GW::MATH::GVECTORF{ 0, Total_Y_Change * PerFrameSpeed, 0, 0 }, levelData.camera);
		Matproxy.TranslateLocalF(levelData.camera, GW::MATH::GVECTORF{ Total_X_Change * PerFrameSpeed, 0, Total_Z_Change * PerFrameSpeed, 0 }, levelData.camera);
		if (result != GW::GReturn::REDUNDANT)
		{
			Matproxy.RotateXLocalF(levelData.camera, Total_Pitch, levelData.camera);
			Matproxy.RotateYGlobalF(levelData.camera, Total_Yaw, levelData.camera);
		}

		Matproxy.InverseF(levelData.camera, levelData.view);
	}

	void swapLevel()
	{
		float f1State = 0;
		InputProxy.GetState(74, f1State);
		if (f1State == 1)
		{
			OPENFILENAMEA fileBox = { 0 };
			char Buffer[300];
			std::fill(Buffer, Buffer + 300, '\0');
			fileBox.lStructSize = sizeof(OPENFILENAMEA);
			fileBox.lpstrFile = Buffer;
			fileBox.nMaxFile = 300;
			fileBox.Flags = OFN_EXPLORER;
			fileBox.lpstrFilter = NULL;
			fileBox.lpstrCustomFilter = NULL;
			fileBox.nFilterIndex = 0;
			fileBox.lpstrFileTitle = NULL;
			fileBox.lpstrInitialDir = NULL;
			fileBox.lpstrTitle = NULL;
			if (GetOpenFileNameA(&fileBox))
			{
				filename = fileBox.lpstrFile;

				//CLEAR VECTORS
				names = {};
				worldMatrices = {};
				lightWorldMatrices = {};
				lightColors = {};
				levelData.indices = {};
				levelData.vertices = {};
				levelData.mats = {};
				levelData.meshes = {};
				levelData.batches = {};
				levelData.meshCount = {};
				levelData.vertexCount = {};
				levelData.indexCount = {};
				levelData.lights = {};
				levelData.lights2 = {};
				levelData.meshData = {};

				//LOAD THE NEW LEVEL
				levelData.LoadLevel(creator, win, d3d);
			}
		}
	}

	~Renderer()
	{
		// ComPtr will auto release so nothing to do here 
	}
};
