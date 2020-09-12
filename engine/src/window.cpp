#include "pch.h"
#include "window.h"

#include <GLFW/glfw3.h>

namespace engine
{
	void window::e_mouse_button_callback(GLFWwindow* window, int b, int a, int m)
	{
		input::update_mbs(b, a);
	}
	void window::e_mouse_scroll_callback(GLFWwindow* window, double xs, double ys)
	{

	}
	void window::e_mouse_move_callback(GLFWwindow* window, double x, double y)
	{
		input::update_mpos(x, y);
	}

	void window::e_keyboard_key_callback(GLFWwindow* window, int k, int s, int a, int m)
	{
		input::update_keys(k, a);
	}
}