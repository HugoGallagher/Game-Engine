#pragma once

#include "pch.h"

#include "graphics/types/vertex.h"
#include "graphics/types/ubo.h"
#include "graphics/types/material.h"
#include "graphics/types/mesh_ubo.h"
#include "graphics/renderer_data.h"

#include "window.h"

namespace engine
{
	struct queue_families
	{
		std::optional<uint32_t> graphics;
		std::optional<uint32_t> present;

		bool complete() { return graphics.has_value() && present.has_value(); }
	};

	struct swap_chain_features
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> present_modes;
	};

	struct descriptor_set_pair
	{
		VkDescriptorSet mat_descriptor_set;
		VkDescriptorSet mesh_descriptor_set;
	};

	struct renderer_object
	{
		int mesh_id;
		mesh_ubo* ubo_data;

		renderer_object(int i, mesh_ubo* mup) : mesh_id(i), ubo_data(mup) {}
	};

	class renderer
	{
	public:
		//static const std::vector<const char*> validation_layers;

		bool debug;

		renderer_data rd;
		std::vector<renderer_object> renderer_objects;
		std::vector<int> mesh_instances = std::vector<int>(255);

		VkInstance vk_instance;
		VkDebugUtilsMessengerEXT debug_messenger;
		VkPhysicalDevice physical_device = VK_NULL_HANDLE;
		VkDevice device;

		VkSwapchainKHR swap_chain;
		std::vector<VkImage> swap_chain_images;
		VkFormat swap_chain_format;
		VkExtent2D swap_chain_extent;
		std::vector<VkImageView> swap_chain_image_views;

		VkQueue q_graphics;
		VkQueue q_present;

		window* glfw_window;
		VkSurfaceKHR surface;

		VkRenderPass render_pass;
		VkPipelineLayout pipeline_layout;
		VkPipeline graphics_pipeline;

		VkCommandPool command_pool;
		std::vector<VkCommandBuffer> command_buffers;
		std::vector<VkSemaphore> image_available_ss;
		std::vector<VkSemaphore> render_finished_ss;
		std::vector<VkFence> in_flight_fs;
		std::vector<VkFence> images_in_flight;
		size_t current_frame = 0;

		VkBuffer vertex_buffer;
		VkDeviceMemory vertex_buffer_memory;
		VkBuffer index_buffer;
		VkDeviceMemory index_buffer_memory;

		VkDescriptorPool descriptor_pool;
		std::vector<descriptor_set_pair> descriptor_sets;
		std::vector<VkDescriptorSetLayout> descriptor_set_layouts; // 0 = mat, 1 = mesh

		std::vector<VkBuffer> mat_uniform_buffers;
		std::vector<VkDeviceMemory> mat_uniform_buffers_memory;
		std::vector<VkBuffer> mesh_uniform_buffers;
		std::vector<VkDeviceMemory> mesh_uniform_buffers_memory;

		bool framebuffer_resized = false;
		std::vector<VkFramebuffer> swap_chain_framebuffers;

		//std::vector<vertex> verticies;
		//std::vector<uint16_t> indicies;

		int descriptor_set_count = 2;

		renderer(window* w);
		~renderer();

		void init();

		static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data)
		{
			std::cerr << "Layer: " << callback_data->pMessage << std::endl;

			return VK_FALSE;
		}

		void draw();

		void finish_rendering()
		{
			vkDeviceWaitIdle(device);
		}

		int add_object(int mesh_i);
		mesh_ubo& get_object(int mesh_i);

		void add_mesh_data(std::vector<vertex>& d, std::vector<int>& i);
		void add_material_data(material m);

	private:
		void create_instance();
		void setup_debug_messenger();
		void create_surface();
		void select_physical_device();
		void create_logical_device();
		void create_swap_chain();
		void create_image_views();
		void create_render_pass();
		void create_descriptor_set_layout();
		void create_graphics_pipeline();
		void create_framebuffers();
		void create_command_pool();
		void create_command_buffers();
		void create_vertex_buffer();
		void create_index_buffer();
		void create_uniform_buffers();
		void create_descriptor_pool();
		void create_descriptor_sets();
		void create_sync_objects();

		void recreate_swap_chain();
		void cleanup_swap_chain();

		void update_uniform_buffer(uint32_t ci);

		std::vector<const char*> get_required_extensions();
		int suitability(VkPhysicalDevice d);

		queue_families get_queue_families(VkPhysicalDevice d);
		swap_chain_features get_swap_chain_features(VkPhysicalDevice d);

		bool supports_device_extensions(VkPhysicalDevice d);

		VkSurfaceFormatKHR format_swap_chain(const std::vector<VkSurfaceFormatKHR>& afs);
		VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& apms);
		VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& c);

		uint32_t find_memory_type(uint32_t tf, VkMemoryPropertyFlags ps);

		static std::vector<char> read_shader(const std::string& n)
		{
			std::ifstream f(n, std::ios::ate | std::ios::binary);
			if (!f.is_open())
				throw std::runtime_error("File not found");

			size_t s = (size_t)f.tellg();
			std::vector<char> b(s);

			f.seekg(0);
			f.read(b.data(), s);

			f.close();
			return b;
		}

		VkShaderModule create_shader_module(const std::vector<char>& c);

		static void framebuffer_callback(GLFWwindow* window, int w, int h)
		{
			renderer* r = (renderer*)glfwGetWindowUserPointer(window);
			r->framebuffer_resized = true;

			//window = glfwCreateWindow(w)
		}

		void create_buffer(VkDeviceSize s, VkBufferUsageFlags u, VkMemoryPropertyFlags ps, VkBuffer& b, VkDeviceMemory& bm);
		void copy_buffer(VkBuffer sb, VkBuffer db, VkDeviceSize s);

		bool cmd_buffer_changed;
	};
}