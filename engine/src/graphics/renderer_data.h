#pragma once

#include "pch.h"

#include "graphics/types/material.h"
#include "graphics/types/mesh_ubo.h"

namespace engine
{
	struct renderer_data
	{
		static const int MAX_MATERIALS = 255;
		static const int MAX_MESHES = 255;
		static const int MAX_MESH_INSTANCES = 255;

		std::vector<material> materials;
		std::vector<mesh_ubo> mesh_data;
		std::vector<int> instances = std::vector<int>(255);
		
		std::vector<vertex> mesh_verticies;
		std::vector<int> mesh_indicies;
		std::vector<int> mesh_vertex_start;
		std::vector<int> mesh_index_start;

		void add_object(int id)
		{
			mesh_ubo mu;

			int c = 0;
			for (int i = 0; i < instances.size(); i++)
			{
				if (i == id)
				{
					std::vector<mesh_ubo>::iterator it = mesh_data.begin() + c;

					mesh_data.insert(it, mu);
					instances[i]++;
				}
				else
					c += instances[i];
			}
		}

		void add_mesh(std::vector<vertex>& m, std::vector<int>& i)
		{
			for (int j : i)
				j += mesh_verticies.size();

			mesh_verticies.reserve(mesh_verticies.size() + m.size());
			mesh_verticies.insert(mesh_verticies.end(), m.begin(), m.end());
			//mesh_vertex_start[mesh_verticies.size() - 1] = &mesh_verticies[mesh_verticies.size() - 1];
			mesh_vertex_start.push_back(mesh_verticies.size() - m.size());

			mesh_indicies.reserve(mesh_indicies.size() + i.size());
			mesh_indicies.insert(mesh_indicies.end(), i.begin(), i.end());
			//mesh_index_start[mesh_indicies.size() - 1] = &mesh_indicies[mesh_indicies.size() - 1];
			mesh_index_start.push_back(mesh_indicies.size() - i.size());
		}

		renderer_data()
		{
			material m = material(vector3(1, 1, 0));
			materials.push_back(m);

			std::vector<vertex> verticies =
			{
				vertex(-1, -1, 1, 0),
				vertex(0, -1, 1, 0),
				vertex(-1, 0, 1, 0),
				vertex(0, 0, 1, 0),
			};
			std::vector<int> indicies =
			{
				0, 1, 2, 1, 3, 2,
			};
			add_mesh(verticies, indicies);
		}
	};
}