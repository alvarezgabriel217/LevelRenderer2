#pragma once
//#include "../Gateware.h"

class Level_Data
{
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
};
