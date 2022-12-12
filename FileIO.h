#pragma once
#include <iostream>
#include <fstream>
#include <vector>

std::ifstream input;

std::vector<GW::MATH::GMATRIXF> worldMatrices;
std::vector<std::string> names;
std::string filename = "../GameLevel.txt";

void openFile()
{
	input.open(filename);

	if (input.is_open())
	{
		while (!input.eof())
		{
			//char buffer[256];
			std::string buffer2;
			std::getline(input, buffer2);
			//input.getline(buffer, 256);
			if (std::strcmp("MESH", buffer2.c_str()) == 0)
			{
				//input.getline(buffer, 256);
				//input.getline(buffer, 256);

				std::getline(input, buffer2);

				if (buffer2.find('.'))
				{
					names.push_back("../h2bs/" + buffer2.substr(0, buffer2.find('.') + 1) + "h2b");
				}
				else
				{
					names.push_back("../h2bs/" + buffer2 + "h2b");
				}

				GW::MATH::GVECTORF rows[4];

				for (int i = 0; i < 4; i++)
				{
					std::getline(input, buffer2);
					buffer2 = buffer2.substr(buffer2.find('(') + 1, buffer2.find(')') - buffer2.find('(') - 1);

					rows[i].x = atof(buffer2.c_str());

					buffer2 = buffer2.substr(buffer2.find(',') + 1);

					rows[i].y = atof(buffer2.c_str());

					buffer2 = buffer2.substr(buffer2.find(',') + 1);

					rows[i].z = atof(buffer2.c_str());

					buffer2 = buffer2.substr(buffer2.find(',') + 1);

					rows[i].w = atof(buffer2.c_str());
				}

				worldMatrices.push_back(GW::MATH::GMATRIXF{ rows[0], rows[1], rows[2], rows[3] });
			}
		}

		for (int i = 0; i < worldMatrices.size(); i++)
		{
			/*std::cout << names[i] << std::endl;
			std::cout << worldMatrices[i].row1.x << ", ";
			std::cout << worldMatrices[i].row1.y << ", ";
			std::cout << worldMatrices[i].row1.z << ", ";
			std::cout << worldMatrices[i].row1.w << std::endl;
			std::cout << worldMatrices[i].row2.x << ", ";
			std::cout << worldMatrices[i].row2.y << ", ";
			std::cout << worldMatrices[i].row2.z << ", ";
			std::cout << worldMatrices[i].row2.w << std::endl;
			std::cout << worldMatrices[i].row3.x << ", ";
			std::cout << worldMatrices[i].row3.y << ", ";
			std::cout << worldMatrices[i].row3.z << ", ";
			std::cout << worldMatrices[i].row3.w << std::endl;
			std::cout << worldMatrices[i].row4.x << ", ";
			std::cout << worldMatrices[i].row4.y << ", ";
			std::cout << worldMatrices[i].row4.z << ", ";
			std::cout << worldMatrices[i].row4.w << std::endl << std::endl;*/
		}

		input.close();
	}
	else
	{
		std::cout << "Filename not found";
	}
}
