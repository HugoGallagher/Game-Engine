#pragma once

#include <vulkan/vulkan.h>

#include "maths/types/vector2.h"
#include "maths/types/vector3.h"

namespace engine
{
	struct vertex
	{
		vector2 pos;
		vector3 col;

		vertex() {}
		vertex(float x, float y) : pos(vector2(x, y)) {}
		vertex(float x, float y, float r, float g, float b) : pos(vector2(x, y)), col(vector3(r, g, b)) {}

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
			attribute_ds[0].format = VK_FORMAT_R32G32_SFLOAT;
			attribute_ds[0].offset = offsetof(vertex, pos);

			attribute_ds[1].binding = 0;
			attribute_ds[1].location = 1;
			attribute_ds[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attribute_ds[1].offset = offsetof(vertex, col);

			return attribute_ds;
		}
	};
}