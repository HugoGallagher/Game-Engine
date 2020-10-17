#include "ecs/ecs.h"

#include "maths/types/vector3.h"

#include "graphics/types/material.h"
#include "graphics/types/vertex.h"

#include "include/engine.h"

struct transform
{
	engine::vector3 position;
	engine::vector3 rotation;
	engine::vector3 scale;

	transform() {}
	transform(engine::vector3 pos, engine::vector3 rot, engine::vector3 scl) : position(pos), rotation(rot), scale(scl) {}
};

struct motion
{
	engine::vector3 velocity;
	float speed;

	motion() {}
	motion(float spd) : speed(spd) {}
};

struct mesh
{
	int id;

	mesh() {}
	mesh(int i) : id(i) {}
};

struct input {};

namespace ecs_systems
{
	// transform, motion, input
	void controller(float dt, engine::entity& e, engine::core_game_objects* cgo)
	{
		if (engine::input::key_down('W'))
			e.get<motion>().velocity.y -= e.get<motion>().speed;
		if (engine::input::key_down('S'))
			e.get<motion>().velocity.y += e.get<motion>().speed;
		if (engine::input::key_down('A'))
			e.get<motion>().velocity.x -= e.get<motion>().speed;
		if (engine::input::key_down('D'))
			e.get<motion>().velocity.x += e.get<motion>().speed;
	}

	// transform, motion
	void move(float dt, engine::entity& e, engine::core_game_objects* cgo)
	{
		engine::vector3 vel = e.get<motion>().velocity * dt;
		e.get<transform>().position = e.get<transform>().position + vel;
		e.get<motion>().velocity = engine::vector3();
	}

	//transform
	void print_coords(float dt, engine::entity& e, engine::core_game_objects* cgo)
	{
		//std::cout << e.get<transform>().position << std::endl;

		engine::vector3 pos = e.get<transform>().position;
		std::cout << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
	}

	//mesh
	void set_mesh(float dt, engine::entity& e, engine::core_game_objects* cgo)
	{
		if (engine::input::key_down('1'))
			std::cout << "asdf";
			//e.get<motion>().velocity.y -= e.get<motion>().speed;
	}

	//transform, mesh
	void update_mesh_ubo(float dt, engine::entity& e, engine::core_game_objects* cgo)
	{
		if (e.get<transform>().position.x != 0)
			std::cout;
		engine::mesh_ubo& mu = cgo->r->get_object(e.get<mesh>().id);
		mu.pos.x += e.get<transform>().position.x;
		mu.pos.y += e.get<transform>().position.y;
	}
}

class game
{
	float ct, pt;

public:
	float dt;

	engine::window window = engine::window(1280, 720, false, "Engine");
	engine::renderer renderer;
	engine::core_game_objects cgo = engine::core_game_objects(&renderer, &window);

	engine::ecs_manager<transform, motion, mesh, input> ecs;

	game();

	void init();
	void exit();

	void update();
	void draw();
};

game::game() : renderer(engine::renderer(&window))
{
	init();
}

void game::init()
{
	ecs.cgo = &cgo;
	ecs.add_system<transform, motion, input>(0, ecs_systems::controller);
	ecs.add_system<transform, motion>(1, ecs_systems::move);
	//ecs.add_system<transform>(2, ecs_systems::print_coords);
	ecs.add_system<transform, mesh>(2, ecs_systems::update_mesh_ubo);
	ecs.add_system<mesh>(2, ecs_systems::set_mesh);

	engine::entity e1 = ecs.add_entity<transform, motion, mesh>(transform(), motion(3), mesh(1));
	//engine::entity e2 = ecs.add_entity<transform, motion, mesh>(transform(), motion(3), mesh(1));

	renderer.add_object(0);
	renderer.add_object(1);
}

void game::exit()
{
	renderer.finish_rendering();
}

void game::update()
{
	dt = ct - pt;
	pt = glfwGetTime();

	window.update();
	ecs.update(dt);
	engine::input::update();

	ct = glfwGetTime();
}
void game::draw()
{
	renderer.draw();
}