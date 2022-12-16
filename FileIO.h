#pragma once
#include <iostream>
#include <fstream>
#include <vector>

std::ifstream input;

std::vector<GW::MATH::GMATRIXF> worldMatrices;
std::vector<GW::MATH::GMATRIXF> lightWorldMatrices;
std::vector<GW::MATH::GMATRIXF> cameras;
std::vector<std::string> names;
std::vector<GW::MATH::GVECTORF> lightColors;
std::vector<float> size;

std::string filename = "../Levels/GameLevel.txt";
std::string skyboxFilename = "../Skyboxes/";

bool canRender = true;

void openFile()
{
	input.open(filename);

	if (input.is_open())
	{
		while (!input.eof())
		{
			std::string buffer2;
			std::getline(input, buffer2);
			if (std::strcmp("MESH", buffer2.c_str()) == 0)
			{
				std::getline(input, buffer2);

				if (buffer2 == "SkyBox")
				{
					names.push_back(buffer2);
					std::getline(input, buffer2);
					skyboxFilename += buffer2;

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
				else
				{
					if (buffer2.find('.'))
					{
						names.push_back(buffer2.substr(0, buffer2.find('.')));
					}
					else
					{
						names.push_back(buffer2);
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
			else if (std::strcmp("LIGHT", buffer2.c_str()) == 0)
			{

				std::getline(input, buffer2);

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

				lightWorldMatrices.push_back(GW::MATH::GMATRIXF{ rows[0], rows[1], rows[2], rows[3] });

				std::getline(input, buffer2);

				GW::MATH::GVECTORF temp;

				buffer2 = buffer2.substr(buffer2.find('=') + 1);
				temp.x = atof(buffer2.c_str());
				buffer2 = buffer2.substr(buffer2.find('=') + 1);
				temp.y = atof(buffer2.c_str());
				buffer2 = buffer2.substr(buffer2.find('=') + 1);
				temp.z = atof(buffer2.c_str());
				temp.w = 0;

				lightColors.push_back(temp);

				std::getline(input, buffer2);

				buffer2 = buffer2.substr(buffer2.find(':') + 1);

				size.push_back(atof(buffer2.c_str()));
			}
			else if (std::strcmp("CAMERA", buffer2.c_str()) == 0)
			{
				GW::MATH::GVECTORF rows[4];
				std::getline(input, buffer2);

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
				cameras.push_back(GW::MATH::GMATRIXF{ rows[0], rows[1], rows[2], rows[3] });
			}
		}

		/*for (int i = 0; i < worldMatrices.size(); i++)
		{
			std::cout << names[i] << std::endl;
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
			std::cout << worldMatrices[i].row4.w << std::endl << std::endl;
		}*/

		/*for (int i = 0; i < lightWorldMatrices.size(); i++)
		{
			std::cout <<  std::endl;
			std::cout << lightWorldMatrices[i].row1.x << ", ";
			std::cout << lightWorldMatrices[i].row1.y << ", ";
			std::cout << lightWorldMatrices[i].row1.z << ", ";
			std::cout << lightWorldMatrices[i].row1.w << std::endl;
			std::cout << lightWorldMatrices[i].row2.x << ", ";
			std::cout << lightWorldMatrices[i].row2.y << ", ";
			std::cout << lightWorldMatrices[i].row2.z << ", ";
			std::cout << lightWorldMatrices[i].row2.w << std::endl;
			std::cout << lightWorldMatrices[i].row3.x << ", ";
			std::cout << lightWorldMatrices[i].row3.y << ", ";
			std::cout << lightWorldMatrices[i].row3.z << ", ";
			std::cout << lightWorldMatrices[i].row3.w << std::endl;
			std::cout << lightWorldMatrices[i].row4.x << ", ";
			std::cout << lightWorldMatrices[i].row4.y << ", ";
			std::cout << lightWorldMatrices[i].row4.z << ", ";
			std::cout << lightWorldMatrices[i].row4.w << std::endl;
			std::cout << lightColors[i].x << " " << lightColors[i].y << " " << lightColors[i].z << " " << lightColors[i].z << std::endl;
			std::cout << "Size: " << size[i] << std::endl << std::endl;
		}*/

		input.close();
	}
	else
	{
		std::cout << "Filename not found";
	}
}
