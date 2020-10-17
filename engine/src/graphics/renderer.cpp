#include "pch.h"
#include "renderer.h"

namespace engine
{
	const int MAX_FRAMES_IN_FLIGHT = 2;

	const std::vector<const char*> validation_layers = { "VK_LAYER_KHRONOS_validation" };
	const std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkResult create_debug_utils_messenger_ext(VkInstance i, const VkDebugUtilsMessengerCreateInfoEXT* ci, const VkAllocationCallbacks* a, VkDebugUtilsMessengerEXT* dm)
	{
		auto f = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(i, "vkCreateDebugUtilsMessengerEXT");
		if (f != nullptr)
			return f(i, ci, a, dm);
		else
			return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
	void destroy_debug_utils_messenger_ext(VkInstance i, VkDebugUtilsMessengerEXT dm, const VkAllocationCallbacks* a)
	{
		auto f = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(i, "vkDestroyDebugUtilsMessengerEXT");
		if (f != nullptr)
			f(i, dm, a);
	}

	void renderer::draw()
	{
		if (cmd_buffer_changed)
		{
			recreate_swap_chain();
			cmd_buffer_changed = false;
		}

		vkWaitForFences(device, 1, &in_flight_fs[current_frame], VK_TRUE, UINT64_MAX);

		uint32_t image_index;
		VkResult result = vkAcquireNextImageKHR(device, swap_chain, UINT64_MAX, image_available_ss[current_frame], VK_NULL_HANDLE, &image_index);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			recreate_swap_chain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
			throw std::runtime_error("Swap chain image not acquired");

		if (images_in_flight[image_index] != VK_NULL_HANDLE)
			vkWaitForFences(device, 1, &images_in_flight[image_index], VK_TRUE, UINT64_MAX);

		images_in_flight[image_index] = in_flight_fs[current_frame];

		update_uniform_buffer(image_index);

		VkSubmitInfo submit_i{};
		submit_i.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore wait_ss[] = { image_available_ss[current_frame] };
		VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		submit_i.waitSemaphoreCount = 1;
		submit_i.pWaitSemaphores = wait_ss;
		submit_i.pWaitDstStageMask = wait_stages;

		submit_i.commandBufferCount = 1;
		submit_i.pCommandBuffers = &command_buffers[image_index];

		VkSemaphore signal_ss[] = { render_finished_ss[current_frame] };
		submit_i.signalSemaphoreCount = 1;
		submit_i.pSignalSemaphores = signal_ss;

		vkResetFences(device, 1, &in_flight_fs[current_frame]);
		if (vkQueueSubmit(q_graphics, 1, &submit_i, in_flight_fs[current_frame]) != VK_SUCCESS)
			throw std::runtime_error("Draw command buffer not submitted");

		VkPresentInfoKHR present_i{};
		present_i.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_i.waitSemaphoreCount = 1;
		present_i.pWaitSemaphores = signal_ss;

		VkSwapchainKHR swap_chains[] = { swap_chain };

		present_i.swapchainCount = 1;
		present_i.pSwapchains = swap_chains;
		present_i.pImageIndices = &image_index;

		result = vkQueuePresentKHR(q_present, &present_i);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized)
		{
			framebuffer_resized = false;
			recreate_swap_chain();
		}
		else if (result != VK_SUCCESS)
			throw std::runtime_error("Swap chain image not presented");

		current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	int renderer::add_object(int mesh_i)
	{
		cmd_buffer_changed = true;

		int i = renderer_objects.size();
		for (int r = 0; r < renderer_objects.size(); r++)
		{
			if (renderer_objects[r].mesh_id == -1) // ID for invalid mesh
			{
				i = r;
			}
		}
		if (i == rd.mesh_data.size())
		{
			rd.add_object(mesh_i);
			renderer_objects.push_back(renderer_object(i, &rd.mesh_data[i]));
		}
		else
		{
			rd.mesh_data[i] = mesh_ubo();
			renderer_objects[i] = renderer_object(i, &rd.mesh_data[i]);
		}

		mesh_instances[mesh_i]++;

		return i;
	}
	mesh_ubo& renderer::get_object(int mesh_i)
	{
		return *renderer_objects[mesh_i].ubo_data;
	}

	void renderer::add_material_data(material m)
	{
		rd.materials.push_back(m);
	}
	void renderer::add_mesh_data(std::vector<vertex>& d, std::vector<int>& i)
	{
		rd.add_mesh(d, i);
	}
	renderer::renderer(window* w) : glfw_window(w)
	{
#ifdef NDEBUG
		debug = false;
#else
		debug = true;
#endif

		glfwSetWindowUserPointer(glfw_window->get_window(), this);
		glfwSetFramebufferSizeCallback(glfw_window->get_window(), renderer::framebuffer_callback);

		init();
	}

	renderer::~renderer()
	{
		cleanup_swap_chain();

		vkDestroyDescriptorSetLayout(device, descriptor_set_layouts[0], nullptr);
		vkDestroyDescriptorSetLayout(device, descriptor_set_layouts[1], nullptr);

		vkDestroyBuffer(device, vertex_buffer, nullptr);
		vkFreeMemory(device, vertex_buffer_memory, nullptr);
		vkDestroyBuffer(device, index_buffer, nullptr);
		vkFreeMemory(device, index_buffer_memory, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(device, image_available_ss[i], nullptr);
			vkDestroySemaphore(device, render_finished_ss[i], nullptr);
			vkDestroyFence(device, in_flight_fs[i], nullptr);
		}

		vkDestroyCommandPool(device, command_pool, nullptr);

		vkDestroyDevice(device, nullptr);

		if (debug)
			destroy_debug_utils_messenger_ext(vk_instance, debug_messenger, nullptr);

		vkDestroySurfaceKHR(vk_instance, surface, nullptr);
		vkDestroyInstance(vk_instance, nullptr);
	}

	void renderer::init()
	{
		create_instance();
		setup_debug_messenger();
		create_surface();
		select_physical_device();
		create_logical_device();
		create_swap_chain();
		create_image_views();
		create_render_pass();
		create_descriptor_set_layout();
		create_graphics_pipeline();
		create_framebuffers();
		create_command_pool();
		create_vertex_buffer();
		create_index_buffer();
		create_uniform_buffers();
		create_descriptor_pool();
		create_descriptor_sets();
		create_command_buffers();
		create_sync_objects();
	}

	void renderer::create_instance()
	{
		VkApplicationInfo app_info{};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName = "Engine";
		app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.pEngineName = "Engine";
		app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		create_info.pApplicationInfo = &app_info;

		auto extensions = get_required_extensions();
		create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		create_info.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debug_create_info;
		if (debug)
		{
			create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
			create_info.ppEnabledLayerNames = validation_layers.data();

			debug_create_info = {};
			debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			debug_create_info.pfnUserCallback = debug_callback;

			create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
		}
		else
		{
			create_info.enabledLayerCount = 0;
		}

		if (vkCreateInstance(&create_info, nullptr, &vk_instance) != VK_SUCCESS)
			std::cout << "Vulkan instance not created" << std::endl;
	}
	void renderer::setup_debug_messenger()
	{
		if (debug)
		{
			uint32_t layer_count;
			vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
			std::vector<VkLayerProperties> available_layers(layer_count);
			vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

			bool found = false;
			bool all_found = true;

			for (const char* name : validation_layers)
			{
				for (const auto& properties : available_layers)
				{
					if (!strcmp(name, properties.layerName))
						found = true;
				}

				if (!found)
					all_found = false;
			}

			if (!all_found)
				throw std::runtime_error("Validation layers not available");

			VkDebugUtilsMessengerCreateInfoEXT create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			create_info.pfnUserCallback = debug_callback;

			if (create_debug_utils_messenger_ext(vk_instance, &create_info, nullptr, &debug_messenger) != VK_SUCCESS)
				throw std::runtime_error("Debug member not initialised");
		}
	}
	void renderer::create_surface()
	{
		if (glfwCreateWindowSurface(vk_instance, glfw_window->get_window(), nullptr, &surface) != VK_SUCCESS)
			throw std::runtime_error("Window surface not created");
	}
	void renderer::select_physical_device()
	{

		uint32_t device_count = 0;
		vkEnumeratePhysicalDevices(vk_instance, &device_count, nullptr);

		if (device_count == 0)
			throw std::runtime_error("No physical devices found");

		std::vector<int> device_scores;
		std::vector<VkPhysicalDevice> devices(device_count);
		vkEnumeratePhysicalDevices(vk_instance, &device_count, devices.data());

		// Needs work

		for (const auto& d : devices)
			device_scores.push_back(suitability(d));

		std::sort(device_scores.begin(), device_scores.end());
		physical_device = devices[0];

		//

		if (physical_device == VK_NULL_HANDLE)
			throw std::runtime_error("No suitable physical devices found");
	}
	void renderer::create_logical_device()
	{
		queue_families qfs = get_queue_families(physical_device);

		std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
		std::set<uint32_t> unique_queue_families =
		{
			qfs.graphics.value(),
			qfs.present.value()
		};
		float priority = 1;

		for (uint32_t qf : unique_queue_families)
		{
			VkDeviceQueueCreateInfo queue_create_info{};
			queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_info.queueFamilyIndex = qf;
			queue_create_info.queueCount = 1;
			queue_create_info.pQueuePriorities = &priority;
			queue_create_infos.push_back(queue_create_info);
		}

		VkPhysicalDeviceFeatures features{};
		VkDeviceCreateInfo device_create_info{};
		device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
		device_create_info.pQueueCreateInfos = queue_create_infos.data();
		device_create_info.pEnabledFeatures = &features;

		device_create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
		device_create_info.ppEnabledExtensionNames = device_extensions.data();
		device_create_info.enabledLayerCount = 0;

		if (debug)
		{
			device_create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
			device_create_info.ppEnabledLayerNames = validation_layers.data();
		}

		if (vkCreateDevice(physical_device, &device_create_info, nullptr, &device) != VK_SUCCESS)
			throw std::runtime_error("Logical device not created!");

		vkGetDeviceQueue(device, qfs.graphics.value(), 0, &q_graphics);
		vkGetDeviceQueue(device, qfs.present.value(), 0, &q_present);
	}
	void renderer::create_swap_chain()
	{
		swap_chain_features scf = get_swap_chain_features(physical_device);
		VkSurfaceFormatKHR surface_format = format_swap_chain(scf.formats);
		VkPresentModeKHR present_mode = choose_swap_present_mode(scf.present_modes);
		VkExtent2D extent = choose_swap_extent(scf.capabilities);

		uint32_t image_count = scf.capabilities.minImageCount + 1;
		if (scf.capabilities.maxImageCount > 0 && image_count > scf.capabilities.maxImageCount)
			image_count = scf.capabilities.maxImageCount;

		VkSwapchainCreateInfoKHR sc_create_info{};
		sc_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		sc_create_info.surface = surface;
		sc_create_info.minImageCount = image_count;
		sc_create_info.imageFormat = surface_format.format;
		sc_create_info.imageColorSpace = surface_format.colorSpace;
		sc_create_info.imageExtent = extent;
		sc_create_info.imageArrayLayers = 1;
		sc_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		queue_families qfs = get_queue_families(physical_device);
		uint32_t qfis[] = { qfs.graphics.value(), qfs.present.value() };
		if (qfs.graphics != qfs.present)
		{
			sc_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			sc_create_info.queueFamilyIndexCount = 2;
			sc_create_info.pQueueFamilyIndices = qfis;
		}
		else
		{
			sc_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			sc_create_info.queueFamilyIndexCount = 0;
			sc_create_info.pQueueFamilyIndices = nullptr;
		}

		sc_create_info.preTransform = scf.capabilities.currentTransform;
		sc_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		sc_create_info.presentMode = present_mode;
		sc_create_info.clipped = VK_TRUE;
		sc_create_info.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(device, &sc_create_info, nullptr, &swap_chain) != VK_SUCCESS)
			throw std::runtime_error("Swap chain not created");

		vkGetSwapchainImagesKHR(device, swap_chain, &image_count, nullptr);
		swap_chain_images.resize(image_count);
		vkGetSwapchainImagesKHR(device, swap_chain, &image_count, swap_chain_images.data());

		swap_chain_format = surface_format.format;
		swap_chain_extent = extent;
	}
	void renderer::create_image_views()
	{
		swap_chain_image_views.resize(swap_chain_images.size());
		for (size_t i = 0; i < swap_chain_images.size(); i++)
		{
			VkImageViewCreateInfo iv_create_info{};
			iv_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			iv_create_info.image = swap_chain_images[i];
			iv_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			iv_create_info.format = swap_chain_format;

			iv_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			iv_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			iv_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			iv_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			iv_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			iv_create_info.subresourceRange.baseMipLevel = 0;
			iv_create_info.subresourceRange.levelCount = 1;
			iv_create_info.subresourceRange.baseArrayLayer = 0;
			iv_create_info.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device, &iv_create_info, nullptr, &swap_chain_image_views[i]) != VK_SUCCESS)
				throw std::runtime_error("Image views not created");
		}
	}
	void renderer::create_render_pass()
	{
		VkAttachmentDescription color_attachment{};
		color_attachment.format = swap_chain_format;
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference color_attachment_reference{};
		color_attachment_reference.attachment = 0;
		color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment_reference;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo render_pass_create_info{};
		render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_create_info.attachmentCount = 1;
		render_pass_create_info.pAttachments = &color_attachment;
		render_pass_create_info.subpassCount = 1;
		render_pass_create_info.pSubpasses = &subpass;

		render_pass_create_info.dependencyCount = 1;
		render_pass_create_info.pDependencies = &dependency;

		if (vkCreateRenderPass(device, &render_pass_create_info, nullptr, &render_pass) != VK_SUCCESS)
			throw std::runtime_error("Render pass not created");
	}
	void renderer::create_descriptor_set_layout()
	{
		descriptor_set_layouts.push_back(VkDescriptorSetLayout());
		descriptor_set_layouts.push_back(VkDescriptorSetLayout());
		descriptor_sets.push_back(descriptor_set_pair());
		descriptor_sets.push_back(descriptor_set_pair());

		VkDescriptorSetLayoutBinding mat_ubo_layout_binding{};
		mat_ubo_layout_binding.binding = 0;
		mat_ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		mat_ubo_layout_binding.descriptorCount = 1;
		mat_ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutCreateInfo mat_layout_ci{};
		mat_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		mat_layout_ci.bindingCount = 1;
		mat_layout_ci.pBindings = &mat_ubo_layout_binding;

		if (vkCreateDescriptorSetLayout(device, &mat_layout_ci, nullptr, &descriptor_set_layouts[0]) != VK_SUCCESS)
			throw std::runtime_error("Descriptor set layout not created");

		VkDescriptorSetLayoutBinding mesh_ubo_layout_binding{};
		mesh_ubo_layout_binding.binding = 1;
		mesh_ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		mesh_ubo_layout_binding.descriptorCount = 1;
		mesh_ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutCreateInfo mesh_layout_ci{};
		mesh_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		mesh_layout_ci.bindingCount = 1;
		mesh_layout_ci.pBindings = &mesh_ubo_layout_binding;

		if (vkCreateDescriptorSetLayout(device, &mesh_layout_ci, nullptr, &descriptor_set_layouts[1]) != VK_SUCCESS)
			throw std::runtime_error("Descriptor set layout not created");
	}
	void renderer::create_graphics_pipeline()
	{
		auto d_vert_code = read_shader("engine/shaders/vert.spv");
		auto d_frag_code = read_shader("engine/shaders/frag.spv");

		VkShaderModule vert_sm = create_shader_module(d_vert_code);
		VkShaderModule frag_sm = create_shader_module(d_frag_code);

		VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
		vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vert_shader_stage_info.module = vert_sm;
		vert_shader_stage_info.pName = "main";

		VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
		frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		frag_shader_stage_info.module = frag_sm;
		frag_shader_stage_info.pName = "main";

		VkPipelineShaderStageCreateInfo shader_stages[] =
		{
			vert_shader_stage_info,
			frag_shader_stage_info
		};

		VkPipelineVertexInputStateCreateInfo vertex_input_i{};
		vertex_input_i.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		auto binding_d = vertex::get_binding_description();
		auto attribute_ds = vertex::get_attribute_descriptions();

		vertex_input_i.vertexBindingDescriptionCount = 1;
		vertex_input_i.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_ds.size());
		vertex_input_i.pVertexBindingDescriptions = &binding_d;
		vertex_input_i.pVertexAttributeDescriptions = attribute_ds.data();

		VkPipelineInputAssemblyStateCreateInfo input_assembly{};
		input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		input_assembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = swap_chain_extent.width;
		viewport.height = swap_chain_extent.height;
		viewport.minDepth = 0;
		viewport.maxDepth = 1;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swap_chain_extent;

		VkPipelineViewportStateCreateInfo viewport_state{};
		viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_state.viewportCount = 1;
		viewport_state.pViewports = &viewport;
		viewport_state.scissorCount = 1;
		viewport_state.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState color_blend_attachment{};
		color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		color_blend_attachment.blendEnable = VK_TRUE;
		color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo color_blending{};
		color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blending.logicOpEnable = VK_FALSE;
		color_blending.attachmentCount = 1;
		color_blending.pAttachments = &color_blend_attachment;

		VkDynamicState dynamic_states[] =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH
		};
		VkPipelineDynamicStateCreateInfo dynamic_state{};
		dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state.dynamicStateCount = 2;
		dynamic_state.pDynamicStates = dynamic_states;

		VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
		pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_create_info.setLayoutCount = 2;
		pipeline_layout_create_info.pSetLayouts = (VkDescriptorSetLayout*)&descriptor_set_layouts[0];

		if (vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &pipeline_layout) != VK_SUCCESS)
			throw std::runtime_error("Pipeline layout not created");

		VkGraphicsPipelineCreateInfo pipeline_create_info{};
		pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_create_info.stageCount = 2;
		pipeline_create_info.pStages = shader_stages;

		pipeline_create_info.pVertexInputState = &vertex_input_i;
		pipeline_create_info.pInputAssemblyState = &input_assembly;
		pipeline_create_info.pViewportState = &viewport_state;
		pipeline_create_info.pRasterizationState = &rasterizer;
		pipeline_create_info.pMultisampleState = &multisampling;
		pipeline_create_info.pDepthStencilState = nullptr;
		pipeline_create_info.pColorBlendState = &color_blending;
		pipeline_create_info.pDynamicState = nullptr;

		pipeline_create_info.layout = pipeline_layout;
		pipeline_create_info.renderPass = render_pass;
		pipeline_create_info.subpass = 0;

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &graphics_pipeline) != VK_SUCCESS)
			throw std::runtime_error("Graphics pipeline not created");

		vkDestroyShaderModule(device, vert_sm, nullptr);
		vkDestroyShaderModule(device, frag_sm, nullptr);
	}
	void renderer::create_framebuffers()
	{
		swap_chain_framebuffers.resize(swap_chain_image_views.size());

		for (size_t i = 0; i < swap_chain_image_views.size(); i++)
		{
			VkImageView attachments[] = { swap_chain_image_views[i] };

			VkFramebufferCreateInfo framebuffer_ci{};
			framebuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebuffer_ci.renderPass = render_pass;
			framebuffer_ci.attachmentCount = 1;
			framebuffer_ci.pAttachments = attachments;
			framebuffer_ci.width = swap_chain_extent.width;
			framebuffer_ci.height = swap_chain_extent.height;
			framebuffer_ci.layers = 1;

			if (vkCreateFramebuffer(device, &framebuffer_ci, nullptr, &swap_chain_framebuffers[i]) != VK_SUCCESS)
				throw std::runtime_error("Framebuffer not created");
		}
	}
	void renderer::create_command_pool()
	{
		queue_families qfs = get_queue_families(physical_device);

		VkCommandPoolCreateInfo pool_ci{};
		pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_ci.queueFamilyIndex = qfs.graphics.value();
		pool_ci.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

		if (vkCreateCommandPool(device, &pool_ci, nullptr, &command_pool) != VK_SUCCESS)
			throw std::runtime_error("Command pool not created");
	}
	void renderer::create_vertex_buffer()
	{
		VkDeviceSize buffer_size = sizeof(rd.mesh_verticies[0]) * rd.mesh_verticies.size();

		VkBuffer staging_buffer;
		VkDeviceMemory staging_buffer_memory;

		create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

		void* data;
		vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data);
		memcpy(data, rd.mesh_verticies.data(), (size_t)buffer_size);
		vkUnmapMemory(device, staging_buffer_memory);

		create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, vertex_buffer, vertex_buffer_memory);
		copy_buffer(staging_buffer, vertex_buffer, buffer_size);

		vkDestroyBuffer(device, staging_buffer, nullptr);
		vkFreeMemory(device, staging_buffer_memory, nullptr);
	}
	void renderer::create_index_buffer()
	{
		VkDeviceSize buffer_size = sizeof(rd.mesh_indicies[0]) * rd.mesh_indicies.size();

		VkBuffer staging_buffer;
		VkDeviceMemory staging_buffer_memory;

		create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

		void* data;
		vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data);
		memcpy(data, rd.mesh_indicies.data(), (size_t)buffer_size);
		vkUnmapMemory(device, staging_buffer_memory);

		create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, index_buffer, index_buffer_memory);
		copy_buffer(staging_buffer, index_buffer, buffer_size);

		vkDestroyBuffer(device, staging_buffer, nullptr);
		vkFreeMemory(device, staging_buffer_memory, nullptr);
	}
	void renderer::create_uniform_buffers()
	{
		VkDeviceSize buffer_size = sizeof(ubo);

		mat_uniform_buffers.resize(swap_chain_images.size());
		mat_uniform_buffers_memory.resize(swap_chain_images.size());

		mesh_uniform_buffers.resize(swap_chain_images.size());
		mesh_uniform_buffers_memory.resize(swap_chain_images.size());

		for (size_t i = 0; i < swap_chain_images.size(); i++)
		{
			create_buffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, mat_uniform_buffers[i], mat_uniform_buffers_memory[i]);
			create_buffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, mesh_uniform_buffers[i], mesh_uniform_buffers_memory[i]);
		}
	}
	void renderer::create_descriptor_pool()
	{
		VkDescriptorPoolSize pool_size{};
		pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		pool_size.descriptorCount = static_cast<uint32_t>(swap_chain_images.size() * descriptor_set_count);

		VkDescriptorPoolCreateInfo pool_ci{};
		pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_ci.poolSizeCount = 1;
		pool_ci.pPoolSizes = &pool_size;
		pool_ci.maxSets = static_cast<uint32_t>(swap_chain_images.size() * descriptor_set_count);

		if (vkCreateDescriptorPool(device, &pool_ci, nullptr, &descriptor_pool) != VK_SUCCESS)
			throw std::runtime_error("Desriptor pool not created");
	}
	void renderer::create_descriptor_sets()
	{
		std::vector<VkDescriptorSetLayout> mat_layouts(swap_chain_images.size(), descriptor_set_layouts[0]);
		VkDescriptorSetAllocateInfo mat_alloc_i{};
		mat_alloc_i.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		mat_alloc_i.descriptorPool = descriptor_pool;
		mat_alloc_i.descriptorSetCount = static_cast<uint32_t>(swap_chain_images.size());
		mat_alloc_i.pSetLayouts = mat_layouts.data();

		std::vector<VkDescriptorSetLayout> mesh_layouts(swap_chain_images.size(), descriptor_set_layouts[1]);
		VkDescriptorSetAllocateInfo mesh_alloc_i{};
		mesh_alloc_i.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		mesh_alloc_i.descriptorPool = descriptor_pool;
		mesh_alloc_i.descriptorSetCount = static_cast<uint32_t>(swap_chain_images.size());
		mesh_alloc_i.pSetLayouts = mesh_layouts.data();

		descriptor_sets.resize(swap_chain_images.size());

		std::vector<VkDescriptorSet> temp_ma_ds;
		for (size_t i = 0; i < swap_chain_images.size(); i++)
			temp_ma_ds.push_back(descriptor_sets[i].mat_descriptor_set);

		std::vector<VkDescriptorSet> temp_me_ds;
		for (size_t i = 0; i < swap_chain_images.size(); i++)
			temp_me_ds.push_back(descriptor_sets[i].mesh_descriptor_set);

		if (vkAllocateDescriptorSets(device, &mat_alloc_i, temp_ma_ds.data()) != VK_SUCCESS)
			throw std::runtime_error("Material descriptor sets not allocated");
		if (vkAllocateDescriptorSets(device, &mesh_alloc_i, temp_me_ds.data()) != VK_SUCCESS)
			throw std::runtime_error("Mesh descriptor sets not allocated");

		for (size_t i = 0; i < swap_chain_images.size(); i++)
		{
			descriptor_sets[i].mat_descriptor_set = temp_ma_ds[i];
			descriptor_sets[i].mesh_descriptor_set = temp_me_ds[i];

			VkDescriptorBufferInfo mat_buffer_i{};
			mat_buffer_i.buffer = mat_uniform_buffers[i];
			mat_buffer_i.offset = 0;
			mat_buffer_i.range = sizeof(material);

			VkWriteDescriptorSet mat_descriptor_write{};
			mat_descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			mat_descriptor_write.dstSet = descriptor_sets[i].mat_descriptor_set;
			mat_descriptor_write.dstBinding = 0;
			mat_descriptor_write.dstArrayElement = 0;

			mat_descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			mat_descriptor_write.descriptorCount = 1;

			mat_descriptor_write.pBufferInfo = &mat_buffer_i;

			VkDescriptorBufferInfo mesh_buffer_i{};
			mesh_buffer_i.buffer = mesh_uniform_buffers[i];
			mesh_buffer_i.offset = 0;
			mesh_buffer_i.range = sizeof(mesh_ubo);

			VkWriteDescriptorSet mesh_descriptor_write{};
			mesh_descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			mesh_descriptor_write.dstSet = descriptor_sets[i].mesh_descriptor_set;
			mesh_descriptor_write.dstBinding = 1;
			mesh_descriptor_write.dstArrayElement = 0;

			mesh_descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			mesh_descriptor_write.descriptorCount = 1;

			mesh_descriptor_write.pBufferInfo = &mesh_buffer_i;

			vkUpdateDescriptorSets(device, 1, &mat_descriptor_write, 0, nullptr);
			vkUpdateDescriptorSets(device, 1, &mesh_descriptor_write, 0, nullptr);
		}
	}
	void renderer::create_command_buffers()
	{
		command_buffers.resize(swap_chain_framebuffers.size());

		VkCommandBufferAllocateInfo allocate_i{};
		allocate_i.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocate_i.commandPool = command_pool;
		allocate_i.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocate_i.commandBufferCount = (uint32_t)command_buffers.size();

		if (vkAllocateCommandBuffers(device, &allocate_i, command_buffers.data()) != VK_SUCCESS)
			throw std::runtime_error("Command buffers not allocated");

		for (size_t i = 0; i < command_buffers.size(); i++)
		{
			VkCommandBufferBeginInfo begin_i{};
			begin_i.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			begin_i.flags = 0;
			begin_i.pInheritanceInfo = nullptr;

			if (vkBeginCommandBuffer(command_buffers[i], &begin_i) != VK_SUCCESS)
				throw std::runtime_error("Command buffer recording not started");

			VkRenderPassBeginInfo render_pass_i{};
			render_pass_i.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			render_pass_i.renderPass = render_pass;
			render_pass_i.framebuffer = swap_chain_framebuffers[i];
			render_pass_i.renderArea.offset = { 0, 0 };
			render_pass_i.renderArea.extent = swap_chain_extent;

			VkClearValue clear_colour = { 0.01, 0.01, 0.01, 1 };
			render_pass_i.clearValueCount = 1;
			render_pass_i.pClearValues = &clear_colour;

			vkCmdBeginRenderPass(command_buffers[i], &render_pass_i, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

			VkBuffer vertex_buffers[] = { vertex_buffer };
			VkDeviceSize offsets[] = { 0 };

			vkCmdBindVertexBuffers(command_buffers[i], 0, 1, vertex_buffers, offsets);
			vkCmdBindIndexBuffer(command_buffers[i], index_buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 2, (VkDescriptorSet*)&descriptor_sets[i], 0, nullptr);

			for (size_t j = 0; j < rd.mesh_index_start.size(); j++)
			{
				for (size_t k = 0; k < mesh_instances[j]; k++)
				{
					uint32_t ic = 0;
					if (j == rd.mesh_index_start.size() - 1)
						ic = (uint32_t)((rd.mesh_indicies.size()) - rd.mesh_index_start[j]);
					else
						ic = (uint32_t)(rd.mesh_index_start[j + 1] - rd.mesh_index_start[j]);

					uint32_t is = mesh_instances[j];
					uint32_t fi = (uint32_t)(rd.mesh_index_start[j] - rd.mesh_index_start[0]);
					uint32_t vo = (uint32_t)(rd.mesh_vertex_start[j] - rd.mesh_vertex_start[0]);
					uint32_t fis = j;

					//vkCmdDrawIndexed(command_buffers[i], ic, is, fi, vo, fis);
					vkCmdDrawIndexed(command_buffers[i], ic, is, fi, vo, fis);
				}
			}

			vkCmdEndRenderPass(command_buffers[i]);

			if (vkEndCommandBuffer(command_buffers[i]) != VK_SUCCESS)
				throw std::runtime_error("Command buffers not recorded");
		}
	}
	void renderer::create_sync_objects()
	{
		image_available_ss.resize(MAX_FRAMES_IN_FLIGHT);
		render_finished_ss.resize(MAX_FRAMES_IN_FLIGHT);
		in_flight_fs.resize(MAX_FRAMES_IN_FLIGHT);
		images_in_flight.resize(swap_chain_images.size(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphore_ci{};
		semaphore_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		VkFenceCreateInfo fence_ci{};
		fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			if (vkCreateSemaphore(device, &semaphore_ci, nullptr, &image_available_ss[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphore_ci, nullptr, &render_finished_ss[i]) != VK_SUCCESS ||
				vkCreateFence(device, &fence_ci, nullptr, &in_flight_fs[i]) != VK_SUCCESS)
				throw std::runtime_error("Synchronisation objects not created");
		}
	}

	void renderer::recreate_swap_chain()
	{
		cleanup_swap_chain();

		int w = 0, h = 0;
		glfwGetFramebufferSize(glfw_window->get_window(), &w, &h);
		while (w == 0 || h == 0)
		{
			glfwGetFramebufferSize(glfw_window->get_window(), &w, &h);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(device);

		create_swap_chain();
		create_image_views();
		create_render_pass();
		create_graphics_pipeline();
		create_framebuffers();
		create_vertex_buffer();
		create_index_buffer();
		create_uniform_buffers();
		create_descriptor_pool();
		create_descriptor_sets();
		create_command_buffers();
	}
	void renderer::cleanup_swap_chain()
	{
		for (size_t i = 0; i < swap_chain_framebuffers.size(); i++)
			vkDestroyFramebuffer(device, swap_chain_framebuffers[i], nullptr);

		vkFreeCommandBuffers(device, command_pool, static_cast<uint32_t>(command_buffers.size()), command_buffers.data());

		vkDestroyPipeline(device, graphics_pipeline, nullptr);
		vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
		vkDestroyRenderPass(device, render_pass, nullptr);

		for (size_t i = 0; i < swap_chain_image_views.size(); i++)
			vkDestroyImageView(device, swap_chain_image_views[i], nullptr);

		vkDestroySwapchainKHR(device, swap_chain, nullptr);

		for (size_t i = 0; i < swap_chain_images.size(); i++)
		{
			vkDestroyBuffer(device, mat_uniform_buffers[i], nullptr);
			vkFreeMemory(device, mat_uniform_buffers_memory[i], nullptr);

			vkDestroyBuffer(device, mesh_uniform_buffers[i], nullptr);
			vkFreeMemory(device, mesh_uniform_buffers_memory[i], nullptr);
		}

		vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
	}

	void renderer::update_uniform_buffer(uint32_t ci)
	{
		void* mat_data;
		vkMapMemory(device, mat_uniform_buffers_memory[ci], 0, sizeof(material), 0, &mat_data);
		memcpy(mat_data, &rd.materials[0], sizeof(material));
		vkUnmapMemory(device, mat_uniform_buffers_memory[ci]);

		void* mesh_data;
		vkMapMemory(device, mesh_uniform_buffers_memory[ci], 0, sizeof(mesh_ubo), 0, &mesh_data);
		memcpy(mesh_data, &rd.mesh_data[0], sizeof(mesh_ubo));
		vkUnmapMemory(device, mesh_uniform_buffers_memory[ci]);
	}

	std::vector<const char*> renderer::get_required_extensions()
	{
		uint32_t glfw_extension_count = 0;
		const char** glfw_extensions;
		glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
		std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

		if (debug)
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		return extensions;
	}

	int renderer::suitability(VkPhysicalDevice d)
	{
		VkPhysicalDeviceProperties ps;
		vkGetPhysicalDeviceProperties(d, &ps);

		VkPhysicalDeviceFeatures fs;
		vkGetPhysicalDeviceFeatures(d, &fs);

		int s = 1;

		if (ps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			s += 1;

		queue_families qfis = get_queue_families(d);

		bool swap_chain_adequate = false;
		if (supports_device_extensions(d))
		{
			swap_chain_features scf = get_swap_chain_features(d);
			swap_chain_adequate = !scf.formats.empty() && !scf.present_modes.empty();
		}

		if (
			qfis.graphics.has_value() ||
			!supports_device_extensions(d) ||
			!swap_chain_adequate
			)
			return 0;

		return s;
	}

	queue_families renderer::get_queue_families(VkPhysicalDevice d)
	{
		uint32_t qfc = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(d, &qfc, nullptr);
		std::vector<VkQueueFamilyProperties> qfs(qfc);
		vkGetPhysicalDeviceQueueFamilyProperties(d, &qfc, qfs.data());

		queue_families qfis;
		for (int i = 0; i < qfc; i++)
		{
			if (qfs[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				qfis.graphics = i;

			VkBool32 supports_present = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(d, i, surface, &supports_present);
			if (supports_present)
				qfis.present = i;
		}

		return qfis;
	}

	swap_chain_features renderer::get_swap_chain_features(VkPhysicalDevice d)
	{
		swap_chain_features scf;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(d, surface, &scf.capabilities);
		uint32_t format_count;
		vkGetPhysicalDeviceSurfaceFormatsKHR(d, surface, &format_count, nullptr);
		if (format_count != 0)
		{
			scf.formats.resize(format_count);
			vkGetPhysicalDeviceSurfaceFormatsKHR(d, surface, &format_count, scf.formats.data());
		}

		return scf;
	}

	bool renderer::supports_device_extensions(VkPhysicalDevice d)
	{
		uint32_t extension_count;
		vkEnumerateDeviceExtensionProperties(d, nullptr, &extension_count, nullptr);
		std::vector<VkExtensionProperties> available_extensions(extension_count);
		vkEnumerateDeviceExtensionProperties(d, nullptr, &extension_count, available_extensions.data());
		std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

		for (const auto& e : available_extensions)
		{
			required_extensions.erase(e.extensionName);
		}

		return required_extensions.empty();
	}

	VkSurfaceFormatKHR renderer::format_swap_chain(const std::vector<VkSurfaceFormatKHR>& afs)
	{
		for (const auto& af : afs)
		{
			if (af.format == VK_FORMAT_B8G8R8A8_SRGB && af.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return af;
		}
		return afs[0];
	}
	VkPresentModeKHR renderer::choose_swap_present_mode(const std::vector<VkPresentModeKHR>& apms)
	{
		for (const auto& apm : apms)
		{
			if (apm == VK_PRESENT_MODE_MAILBOX_KHR)
				return VK_PRESENT_MODE_MAILBOX_KHR;
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}
	VkExtent2D renderer::choose_swap_extent(const VkSurfaceCapabilitiesKHR& c)
	{
		if (c.currentExtent.width != UINT32_MAX)
			return c.currentExtent;
		else
		{
			int w, h;
			glfwGetFramebufferSize(glfw_window->get_window(), &w, &h);

			VkExtent2D ae = { static_cast<uint32_t>(w), static_cast<uint32_t>(h) };

			ae.width = (std::max)(c.minImageExtent.width, (std::min)(c.maxImageExtent.width, ae.width));
			ae.height = (std::max)(c.minImageExtent.height, (std::min)(c.maxImageExtent.height, ae.height));
		}
	}

	VkShaderModule renderer::create_shader_module(const std::vector<char>& c)
	{
		VkShaderModuleCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		ci.codeSize = c.size();
		ci.pCode = reinterpret_cast<const uint32_t*>(c.data());

		VkShaderModule sm;

		if (vkCreateShaderModule(device, &ci, nullptr, &sm) != VK_SUCCESS)
			throw std::runtime_error("Shader module not created");

		return sm;
	}

	uint32_t renderer::find_memory_type(uint32_t tf, VkMemoryPropertyFlags ps)
	{
		VkPhysicalDeviceMemoryProperties mps;
		vkGetPhysicalDeviceMemoryProperties(physical_device, &mps);

		for (uint32_t i = 0; i < mps.memoryTypeCount; i++)
		{
			if ((tf & (1 << i)) && (mps.memoryTypes[i].propertyFlags & ps) == ps)
				return i;
		}

		throw std::runtime_error("Suitable memory type not found");
	}

	void renderer::create_buffer(VkDeviceSize s, VkBufferUsageFlags u, VkMemoryPropertyFlags ps, VkBuffer& b, VkDeviceMemory& bm)
	{
		VkBufferCreateInfo buffer_ci{};
		buffer_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_ci.size = s;
		buffer_ci.usage = u;
		buffer_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &buffer_ci, nullptr, &b) != VK_SUCCESS)
			throw std::runtime_error("Buffer not created");

		VkMemoryRequirements memory_requirements;
		vkGetBufferMemoryRequirements(device, b, &memory_requirements);

		VkMemoryAllocateInfo allocate_i{};
		allocate_i.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocate_i.allocationSize = memory_requirements.size;
		allocate_i.memoryTypeIndex = find_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		if (vkAllocateMemory(device, &allocate_i, nullptr, &bm) != VK_SUCCESS)
			throw std::runtime_error("Buffer memory not allocated");

		vkBindBufferMemory(device, b, bm, 0);
	}
	void renderer::copy_buffer(VkBuffer sb, VkBuffer db, VkDeviceSize s)
	{
		VkCommandBufferAllocateInfo alloc_i{};
		alloc_i.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_i.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_i.commandPool = command_pool;
		alloc_i.commandBufferCount = 1;

		VkCommandBuffer command_buffer;
		vkAllocateCommandBuffers(device, &alloc_i, &command_buffer);

		VkCommandBufferBeginInfo begin_i{};
		begin_i.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_i.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(command_buffer, &begin_i);

		VkBufferCopy copy_region{};
		copy_region.size = s;
		vkCmdCopyBuffer(command_buffer, sb, db, 1, &copy_region);

		vkEndCommandBuffer(command_buffer);

		VkSubmitInfo submit_i{};
		submit_i.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_i.commandBufferCount = 1;
		submit_i.pCommandBuffers = &command_buffer;

		vkQueueSubmit(q_graphics, 1, &submit_i, VK_NULL_HANDLE);
		vkQueueWaitIdle(q_graphics);

		vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
	}
}