#pragma once
//#include "../Gateware.h"

class Level_Data
{
	//unsigned vertexCount[10];
	//unsigned indexCount[10];
	//unsigned materialCount[10];

public:
	struct light
	{
		GW::MATH::GVECTORF position;
		GW::MATH::GVECTORF color;
		float size;
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


	void ParseStuff()
	{
		H2B::Parser parser;
		openFile();
		for (int i = 0; i < names.size(); i++)
		{
			if (parser.Parse(names[i].c_str()))
			{
				mats.push_back(parser.materials);
				vertices.push_back(parser.vertices);
				indices.push_back(parser.indices);
				meshes.push_back(parser.meshes);
				meshCount.push_back(parser.meshCount);
				batches.push_back(parser.batches);
				vertexCount.push_back(parser.vertexCount);
				indexCount.push_back(parser.indexCount);
				//std::cout << "Successful";
			}
			else
			{
				std::cout << names[i] << ": Failed to parse data" << std::endl;
			}
		}
		for (int i = 0; i < lightColors.size(); i++)
		{
			lights.push_back({lightWorldMatrices[i].row4, lightColors[i], size[i] });
		}

		/*parser.Parse("../h2bs/Bookcase_Full_Cylinder.h2b");
		mats.push_back(parser.materials);
		vertices.push_back(parser.vertices);
		indices[0] = parser.indices;
		meshes[0] = parser.meshes;
		meshCount.push_back(parser.meshCount);
		batches[0] = parser.batches;*/
	}
};
