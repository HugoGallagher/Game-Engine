#include "ecs/ecs.h"

#include "maths/types/vector3.h"

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
	void controller(float dt, engine::entity& e, std::vector<engine::entity>& es)
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
	void move(float dt, engine::entity& e, std::vector<engine::entity>& es)
	{
		engine::vector3 vel = e.get<motion>().velocity * dt;
		e.get<transform>().position = e.get<transform>().position + vel;
		e.get<motion>().velocity = engine::vector3();
	}

	//transform
	void print_coords(float dt, engine::entity& e, std::vector<engine::entity>& es)
	{
		//std::cout << e.get<transform>().position << std::endl;

		engine::vector3 pos = e.get<transform>().position;
		//std::cout << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
	}
}

class game
{
	float ct, pt;

public:
	float dt;

	engine::ecs_manager<transform, motion, mesh, input> ecs;

	engine::window window = engine::window(1280, 720, false, "Engine");
	engine::renderer renderer;

	game();

	void init();
	void exit();

	void update();
	void draw();
};

game::game() : renderer(engine::renderer(&window))
{
	ecs.add_system<transform, motion, input>(0, ecs_systems::controller);
	ecs.add_system<transform, motion>(1, ecs_systems::move);
	ecs.add_system<transform>(2, ecs_systems::print_coords);

	init();
}

void game::init()
{
	engine::entity e1 = ecs.add_entity<transform, motion>(transform(), motion(3));
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