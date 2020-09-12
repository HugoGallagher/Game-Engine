#include "game_config.h"

void main()
{
	game g;

	std::cin;

	bool running = true;
	while (running)
	{
		g.update();
		g.draw();

		if (glfwWindowShouldClose(g.window.get_window()))
		{
			running = false;
			g.exit();
		}
	}
}