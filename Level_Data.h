#pragma once
//#include "../Gateware.h"

class Level_Data
{
	//unsigned vertexCount[10];
	//unsigned indexCount[10];
	//unsigned materialCount[10];

public:
	std::vector<std::vector<H2B::MATERIAL>> mats;
	std::vector<std::vector<H2B::VERTEX>> vertices;
	std::vector<unsigned> indices[10];
	std::vector<H2B::MESH> meshes[10];
	std::vector<H2B::BATCH> batches[10];
	std::vector<int> meshCount;

	Microsoft::WRL::ComPtr<ID3D12Resource>	allVertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource>	allIndexBuffer;

	Microsoft::WRL::ComPtr<ID3D12Resource>	constantBuffer;

	void ParseStuff()
	{
		H2B::Parser parser;
		openFile();
		/*for (int i = 0; i < 20; i++)
		{
			std::string filepath = "../h2bs/" + names[i];
			std::cout << std::endl << filepath << " ";
			if (parser.Parse(filepath.c_str()))
			{
				std::cout << "Successful";
			}
			else
			{
				std::cout << "Failed to parse data";
			}
		}*/

		parser.Parse("../h2bs/Bookcase_Full_Cylinder.h2b");
		mats.push_back(parser.materials);
		vertices.push_back(parser.vertices);
		indices[0] = parser.indices;
		meshes[0] = parser.meshes;
		meshCount.push_back(parser.meshCount);
		batches[0] = parser.batches;
	}
};
