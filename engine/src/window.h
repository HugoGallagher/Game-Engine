#pragma once

#include "pch.h"

#include "input.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace engine
{
	class window
	{
		GLFWwindow* _window;

		unsigned int _width, _height;
		bool _vsync = false;
		const char* _title;
	public:
		//typedef void(window::*e_callback_function)(event&);
		//e_callback_function e_callback;

		//std::function<void(event&)> e_callback;

		GLFWwindow* get_window() { return _window;  }

		window(unsigned int w, unsigned int h, bool v, const char* t) : _width(w), _height(h), _vsync(v), _title(t)
		{
			glfwInit();

			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

			_window = glfwCreateWindow(_width, _height, _title, NULL, NULL);
			glfwMakeContextCurrent(_window);
			//glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

			glfwSetMouseButtonCallback(_window, &engine::window::e_mouse_button_callback);
			glfwSetScrollCallback(_window, engine::window::e_mouse_scroll_callback);
			glfwSetCursorPosCallback(_window, engine::window::e_mouse_move_callback);

			glfwSetKeyCallback(_window, engine::window::e_keyboard_key_callback);
		}
		~window() { glfwDestroyWindow(_window); glfwTerminate(); }

		void update()
		{
			glfwSwapBuffers(_window);
			glfwPollEvents();
		}

		unsigned int get_width() const { return _width; }
		unsigned int get_height() const { return _height; }
		bool vsync_enabled() const { return _vsync; }

		static void e_mouse_button_callback(GLFWwindow* window, int b, int a, int m);
		static void e_mouse_scroll_callback(GLFWwindow* window, double xs, double ys);
		static void e_mouse_move_callback(GLFWwindow* window, double x, double y);

		static void e_keyboard_key_callback(GLFWwindow* window, int k, int s, int a, int m);
	};
}