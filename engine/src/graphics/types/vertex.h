#pragma once

#include <vulkan/vulkan.h>

#include "maths/types/vector2.h"
#include "maths/types/vector3.h"

namespace engine
{
	struct vertex
	{
		vector3 pos;
		short material_id;

		vertex() {}
		vertex(float x, float y, float z) : pos(vector3(x, y, z)), material_id(0) {}
		vertex(float x, float y, float z, short material_id) : pos(vector3(x, y, z)),material_id(material_id) {}

		static VkVertexInputBindingDescription get_binding_description()
		{
			VkVertexInputBindingDescription binding_d{};
			binding_d.binding = 0;
			binding_d.stride = sizeof(vertex);
			binding_d.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return binding_d;
		}
		static std::array<VkVertexInputAttributeDescription, 2> get_attribute_descriptions()
		{
			std::array<VkVertexInputAttributeDescription, 2> attribute_ds{};
			attribute_ds[0].binding = 0;
			attribute_ds[0].location = 0;
			attribute_ds[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attribute_ds[0].offset = offsetof(vertex, pos);

			attribute_ds[1].binding = 0;
			attribute_ds[1].location = 1;
			attribute_ds[1].format = VK_FORMAT_R32_SINT;
			attribute_ds[1].offset = offsetof(vertex, material_id);

			return attribute_ds;
		}
	};
}