#pragma once

#include "pch.h"

#include "maths/types/vector3.h"

#include "graphics/renderer.h"

namespace engine
{
	struct core_game_objects
	{
		renderer* r;
		window* w;

		core_game_objects(renderer* rp, window* wp) : r(rp), w(wp) {}
	};

	typedef uint16_t component_index;

	extern int id_counter;
	const int NO_COMPONENTS = 3;
	const int NO_COMPONENT_IS = (NO_COMPONENTS - (NO_COMPONENTS % 64)) / 64 + 1;

	template<class... Ts>
	struct ecs_manager;

	struct base {};

	enum ecs_types
	{
		ecs_entity,
		ecs_component,
		ecs_system,
	};

	class ecs_mask
	{
	public:
		uint64_t mask[NO_COMPONENT_IS];

		ecs_mask()
		{
			std::fill_n(mask, NO_COMPONENT_IS, 0);

			uint64_t* start = &mask[0];

			for (int i = 0; i < NO_COMPONENT_IS; i++)
			{
				*start = 0;
				start++;
			}
		}

		void set(int i)
		{
			int offset = i % 64;
			int v_index = (i - offset) / 64;
			mask[v_index] = mask[v_index] | ((uint64_t)1 << offset);
		}

		void reset(int i)
		{
			int offset = i % 64;
			int v_index = (i - offset) / 64;

			mask[v_index] = mask[v_index] ^ ((uint64_t)1 << offset);
		}

		inline bool operator|(ecs_mask& m)
		{
			for (int i = 0; i < NO_COMPONENT_IS; i++)
			{
				if (!(mask[i] | m.mask[i]))
					return false;
			}
			return true;
		}
		inline bool operator&(ecs_mask& m)
		{
			for (int i = 0; i < NO_COMPONENT_IS; i++)
			{
				if (!(mask[i] & m.mask[i]))
					return false;
			}
			return true;
		}
		inline bool operator[](int i)
		{
			int offset = i % 64;
			int v_index = (i - offset) / 64;

			return mask[v_index] & ((uint64_t)1 << offset);
		}
	};

	struct ecs_data
	{
		static std::map<std::type_index, component_index> component_ids;
	};

	struct entity
	{
		static std::map<std::type_index, void*> component_managers;

		ecs_mask mask;
		std::map<std::type_index, component_index> component_ids;

		template<typename T>
		T& get();

		template<typename T>
		void enable();
		template<typename T>
		void disable();
	};

	struct component{};

	template<class T>
	struct component_manager
	{
		int component_id;

		std::vector<T> components;

		component_manager() : component_id(id_counter) { id_counter++; }
	};

	typedef void (*linked_function)(float dt, entity&, core_game_objects*);

	struct system
	{
		ecs_mask mask;

		linked_function function;
		int order;

		system(int o, linked_function lf) : order(o), function(lf) {}
	};

	template<class... Ts>
	class ecs_manager
	{
	public:
		core_game_objects* cgo;

		std::vector<entity> entities;
		std::tuple<component_manager<Ts>...> components_tuple;
		std::vector<component_manager<component>*> components;
		std::vector<system> systems;

		void update(float dt)
		{
			for (system s : systems) { for (entity e : entities)
			{
				//std::cout << "System: " << s.mask.mask[0] << std::endl << "Entity: " << e.mask.mask[0] << std::endl << std::endl;
				if (s.mask | e.mask)
					s.function(dt, e, cgo);
			}}
		}
		
		ecs_manager() : components_tuple{component_manager<Ts>{}...}
		{
			components.push_back(nullptr);
			constructor_helper<0, Ts...>();
		}

		template<int I, typename t, typename... ts>
		void constructor_helper()
		{
			const int i = I;

			component_manager<t>* cm = &std::get<i>(components_tuple);
			components.push_back((component_manager<component>*)cm);
			entity::component_managers[typeid(t)] = &std::get<i>(components_tuple);
			ecs_data::component_ids[typeid(t)] = i + 1;

			constructor_helper<i + 1, ts...>();
		}

		template<int I>
		void constructor_helper() {}

		template<typename... ts>
		entity& add_entity(ts... data)
		{
			entities.push_back(entity());

			add_entity_helper<0, ts...>(data...);

			return entities[entities.size() - 1];
		}

		template<int I, typename t, typename... ts>
		void add_entity_helper(t c, ts... data)
		{
			t comp = c;
			((component_manager<t>*)components[ecs_data::component_ids[typeid(t)]])->components.push_back(comp);
			entities[entities.size()-1].component_ids[typeid(t)] = (components[ecs_data::component_ids[typeid(t)]]->components.size()) / sizeof(t);
			entities[entities.size()-1].enable<t>();
			add_entity_helper<I, ts...>(data...);
		}

		template<int I>
		void add_entity_helper() {}

		template<typename... ts>
		void add_system(int o, linked_function lf)
		{
			systems.push_back(system(o, lf));
			add_system_helper<0, ts...>(systems.size()-1);
		}

		template<int I, typename t, typename... ts>
		void add_system_helper(int i)
		{
			systems[i].mask.set(ecs_data::component_ids[typeid(t)]);

			add_system_helper<0, ts...>(i);
		}

		template<int I>
		void add_system_helper(int i)
		{
			std::sort(systems.begin(), systems.end(), [](system& s1, system& s2) { return s1.order > s2.order; });
		}
	};

	template<typename T>
	T& entity::get()
	{
		component_manager<T>& cm = *((component_manager<T>*)component_managers[typeid(T)]);
		return ((component_manager<T>*)component_managers[typeid(T)])->components[component_ids[typeid(T)]-1];
	}

	template<typename T>
	void entity::enable()
	{
		if (component_ids.count(typeid(T)))
			mask.set(ecs_data::component_ids[typeid(T)]);
		else
			std::cout << "Error: Failed to attach " << typeid(T).name() << std::endl;
	}
	template<typename T>
	void entity::disable()
	{
		if (!component_ids.count(typeid(T)))
			mask.reset(ecs_data::component_ids[typeid(T)]);
		else
			std::cout << "Error: Failed to detach " << typeid(T).name() << std::endl;
	}
}