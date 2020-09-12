#include "pch.h"
#include "ecs.h"

namespace engine
{
	int id_counter = 1;

	std::map<std::type_index, void*> entity::component_managers = std::map<std::type_index, void*>();
	std::map<std::type_index, component_index> ecs_data::component_ids = std::map<std::type_index, component_index>();
}