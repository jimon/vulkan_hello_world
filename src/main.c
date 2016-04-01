
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

// such MSC
#ifdef _MSC_VER
#define alloca _alloca
#endif

// most of it based on https://gist.github.com/graphitemaster/e162a24e57379af840d4

void error_callback(int error, const char * description)
{
	printf("glfw error : (%i) %s\n", error, description);
	exit(1);
}

VkInstance create_instance() // create vulkan api instance
{
	VkApplicationInfo application_info;
	application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	application_info.pNext = NULL;
	application_info.pApplicationName = "wut app";	// is it even used ?
	application_info.pEngineName = NULL;			// oh lol
	application_info.engineVersion = 1;
	application_info.apiVersion = VK_API_VERSION;

	VkInstanceCreateInfo instance_info;
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pNext = NULL;
	instance_info.flags = 0;
	instance_info.pApplicationInfo = &application_info;
	instance_info.enabledLayerCount = 0;
	instance_info.ppEnabledLayerNames = NULL;
	instance_info.ppEnabledExtensionNames = glfwGetRequiredInstanceExtensions(&instance_info.enabledExtensionCount); // important for inter-op with glfw

	PFN_vkCreateInstance pfnCreateInstance = (PFN_vkCreateInstance)glfwGetInstanceProcAddress(NULL, "vkCreateInstance");
	VkInstance instance = NULL;
	VkResult result = pfnCreateInstance(&instance_info, NULL, &instance);
	if(result != VK_SUCCESS)
	{
		printf("failed to create instance: %i\n", result);
		exit(1);
	}
	return instance;
}

VkDevice create_device(VkInstance instance, VkPhysicalDevice * out_physical_device)
{
	PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)glfwGetInstanceProcAddress(instance, "vkEnumeratePhysicalDevices");
	uint32_t device_count = 0;
	VkResult result = vkEnumeratePhysicalDevices(instance, &device_count, NULL);
	if(result != VK_SUCCESS)
	{
		printf("failed to query the number of physical devices present: %i\n", result);
		exit(1);
	}

	if(device_count == 0)
	{
		printf("couldn't detect any device present with vulkan support: %i\n", result);
		exit(1);
	}

	VkPhysicalDevice * physical_devices = alloca(device_count * sizeof(VkPhysicalDevice));
	result = vkEnumeratePhysicalDevices(instance, &device_count, physical_devices);
	if(result != VK_SUCCESS)
	{
		printf("faied to enumerate physical devices present: %i\n", result);
		exit(1);
	}

	printf("found %i vulkan devices\n", device_count);

	PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)glfwGetInstanceProcAddress(instance, "vkGetPhysicalDeviceProperties");
	for(uint32_t i = 0; i < device_count; i++)
	{
		VkPhysicalDeviceProperties device_properties;
		memset(&device_properties, 0, sizeof(device_properties));
		vkGetPhysicalDeviceProperties(physical_devices[i], &device_properties);
		printf("driver version: %i\n", device_properties.driverVersion);
		printf("device name:    %s\n", device_properties.deviceName);
		printf("device type:    %i\n", device_properties.deviceType);
		printf("api version:    %i.%i.%i\n", VK_VERSION_MAJOR(device_properties.apiVersion), VK_VERSION_MINOR(device_properties.apiVersion), VK_VERSION_PATCH(device_properties.apiVersion));
	}

	PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)glfwGetInstanceProcAddress(instance, "vkGetPhysicalDeviceQueueFamilyProperties");
	for(uint32_t i = 0; i < device_count; ++i)
	{
		uint32_t queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_count, NULL);
		VkQueueFamilyProperties * family_properties = alloca(queue_family_count * sizeof(VkQueueFamilyProperties));
		vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_count, family_properties);

		for(uint32_t j = 0; j < queue_family_count; ++j)
		{
			printf("count of queues: %d\n", family_properties[j].queueCount);
			printf("supported operationg on this queue:%s%s%s%s\n",
				(family_properties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) ? " graphics" : "",
				(family_properties[j].queueFlags & VK_QUEUE_COMPUTE_BIT) ? ", compute" : "",
				(family_properties[j].queueFlags & VK_QUEUE_TRANSFER_BIT) ? ", transfer" : "",
				(family_properties[j].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) ? ", sparse binding" : "");

			if(glfwGetPhysicalDevicePresentationSupport(instance, physical_devices[i], j))
				printf("and it should work just fine with glfw here\n");
		}
	}

	VkPhysicalDevice physical_device = physical_devices[0];
	if(out_physical_device)
		*out_physical_device = physical_device;

	float queue_priorities[] = {1.0f};
	VkDeviceQueueCreateInfo device_queue_info;
	device_queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	device_queue_info.pNext = NULL;
	device_queue_info.flags = 0;
	device_queue_info.queueFamilyIndex = 0;
	device_queue_info.queueCount = 1;
	device_queue_info.pQueuePriorities = queue_priorities;

	VkDeviceCreateInfo device_info;
	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.pNext = NULL;
	device_info.flags = 0;
	device_info.enabledLayerCount = 0;
	device_info.ppEnabledLayerNames = NULL;
	device_info.enabledExtensionCount = 0;
	device_info.ppEnabledExtensionNames = NULL;
	device_info.pEnabledFeatures = NULL;
	device_info.queueCreateInfoCount = 1;
	device_info.pQueueCreateInfos = &device_queue_info;

	PFN_vkCreateDevice vkCreateDevice = (PFN_vkCreateDevice)glfwGetInstanceProcAddress(instance, "vkCreateDevice");
	VkDevice device;
	result = vkCreateDevice(physical_device, &device_info, NULL, &device); // choose wisely which device to use xD
	if(result != VK_SUCCESS)
	{
		printf("failed creating logical device: %i\n", result);
		exit(1);
	}

	return device;
}

static void key_callback(GLFWwindow * window, int key, int scancode, int action, int mods)
{
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

#define SWAPCHAIN_IMAGES_MAX 32

typedef struct
{
	uint32_t graphics_queue_index;
	uint32_t present_queue_index;

	VkFormat color_format;
	VkColorSpaceKHR color_space;

	uint32_t width, height;

	VkSwapchainKHR swapchainKHR;

	VkImage images[SWAPCHAIN_IMAGES_MAX];
	uint32_t images_count;

	VkImageView image_views[SWAPCHAIN_IMAGES_MAX];
	uint32_t image_views_count;
} swapchain_t;

swapchain_t figure_out_color_format(VkInstance instance, VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
	PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)glfwGetInstanceProcAddress(instance, "vkGetPhysicalDeviceQueueFamilyProperties");
	PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)glfwGetInstanceProcAddress(instance, "vkGetPhysicalDeviceSurfaceSupportKHR");

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, NULL);
	VkQueueFamilyProperties * family_properties = alloca(queue_family_count * sizeof(VkQueueFamilyProperties));
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, family_properties);
	VkBool32 * supports_present = alloca(queue_family_count * sizeof(VkBool32));

	for(uint32_t i = 0; i < queue_family_count; ++i)
		vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &supports_present[i]);

	swapchain_t swapchain = {0};

	swapchain.graphics_queue_index = -1;
	swapchain.present_queue_index = -1;
	for(uint32_t i = 0; i < queue_family_count; ++i)
	{
		if(family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			if(swapchain.graphics_queue_index == -1)
				swapchain.graphics_queue_index = i;

			if(supports_present[i] == VK_TRUE)
			{
				swapchain.graphics_queue_index = i;
				swapchain.present_queue_index = i;
				break;
			}
		}
	}

	if(swapchain.present_queue_index == -1)
	{
		for (uint32_t i = 0; i < queue_family_count; ++i)
		{
			if(supports_present[i] != VK_TRUE)
				continue;
			swapchain.present_queue_index = i;
			break;
		}
	}

	if(swapchain.graphics_queue_index == -1 || swapchain.present_queue_index == -1)
	{
		printf("no graphics or present queue found\n");
		exit(1);
	}

	PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)glfwGetInstanceProcAddress(NULL, "vkGetPhysicalDeviceSurfaceFormatsKHR");

	uint32_t surface_formats_count = 0;
	VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_formats_count, NULL);
	if(result != VK_SUCCESS)
	{
		printf("failed to get physical device surface formats: %i\n", result);
		exit(1);
	}

	VkSurfaceFormatKHR * surface_formats = alloca(surface_formats_count * sizeof(VkSurfaceFormatKHR));
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_formats_count, surface_formats);
	if(result != VK_SUCCESS)
	{
		printf("failed to get physical device surface formats: %i\n", result);
		exit(1);
	}

	printf("total surface format count %i\n", surface_formats_count);
	for(uint32_t i = 0; i < surface_formats_count; ++i)
	{
		printf("surface format %i : format %i color space %i\n", i, surface_formats[i].format, surface_formats[i].colorSpace);
	}

	if(!surface_formats_count)
	{
		printf("no surface formats available\n");
		exit(1);
	}

	swapchain.color_format = (surface_formats_count == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED) ? VK_FORMAT_B8G8R8A8_UNORM : surface_formats[0].format;
	swapchain.color_space = surface_formats[0].colorSpace;

	return swapchain;
}

void create_swapchain(swapchain_t * swapchain, VkInstance instance, VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface)
{
	PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)glfwGetInstanceProcAddress(NULL, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
	VkSurfaceCapabilitiesKHR surface_capabilities;
	VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);
	if(result != VK_SUCCESS)
	{
		printf("failed to get physical device surface capabilities: %i\n", result);
		exit(1);
	}

	PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)glfwGetInstanceProcAddress(NULL, "vkGetPhysicalDeviceSurfacePresentModesKHR");
	uint32_t present_modes_count = 0;
	result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_modes_count, NULL);
	if(result != VK_SUCCESS)
	{
		printf("failed to get physical device surface present modes: %i\n", result);
		exit(1);
	}
	VkPresentModeKHR * present_modes = alloca(present_modes_count * sizeof(VkPresentModeKHR));
	result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_modes_count, present_modes);
	if(result != VK_SUCCESS)
	{
		printf("failed to get physical device surface present modes: %i\n", result);
		exit(1);
	}

	VkExtent2D swap_chain_extent;
	if(surface_capabilities.currentExtent.width == -1 && surface_capabilities.currentExtent.height == -1) // does surface enforce size ?
	{
		swap_chain_extent.width = swapchain->width;
		swap_chain_extent.height = swapchain->height;
	}
	else
	{
		swap_chain_extent = surface_capabilities.currentExtent;
		swapchain->width = swap_chain_extent.width;
		swapchain->height = swap_chain_extent.height;
	}

	VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
	for(uint32_t i = 0; i < present_modes_count; i++)
		if(present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}

	uint32_t desired_images = surface_capabilities.minImageCount + 1;
	if(surface_capabilities.maxImageCount > 0 && desired_images > surface_capabilities.maxImageCount)
		desired_images = surface_capabilities.maxImageCount;

	VkSurfaceTransformFlagBitsKHR pre_transform = surface_capabilities.currentTransform;
	if(surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
		pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

	VkSwapchainCreateInfoKHR swapchain_create_info = {0};
	swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_create_info.pNext = NULL;
	swapchain_create_info.surface = surface;
	swapchain_create_info.minImageCount = desired_images;
	swapchain_create_info.imageFormat = swapchain->color_format;
	swapchain_create_info.imageColorSpace = swapchain->color_space;
	swapchain_create_info.imageExtent = swap_chain_extent;
	swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_create_info.preTransform = pre_transform;
	swapchain_create_info.imageArrayLayers = 1;
	swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_create_info.queueFamilyIndexCount = 0; // ?
	swapchain_create_info.pQueueFamilyIndices = NULL;
	swapchain_create_info.presentMode = present_mode;
	swapchain_create_info.clipped = true;
	swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	VkSwapchainKHR old_swapchain = swapchain->swapchainKHR;

	PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)glfwGetInstanceProcAddress(NULL, "vkCreateSwapchainKHR");
	result = vkCreateSwapchainKHR(device, &swapchain_create_info, NULL, &swapchain->swapchainKHR);
	if(result != VK_SUCCESS)
	{
		printf("failed to create swapchain: %i\n", result);
		exit(1);
	}

	if(old_swapchain != VK_NULL_HANDLE)
	{
		PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)glfwGetInstanceProcAddress(NULL, "vkDestroySwapchainKHR");
		vkDestroySwapchainKHR(device, old_swapchain, NULL);
	}

	PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)glfwGetInstanceProcAddress(NULL, "vkGetSwapchainImagesKHR");
	result = vkGetSwapchainImagesKHR(device, swapchain->swapchainKHR, &swapchain->images_count, NULL);
	if(result != VK_SUCCESS)
	{
		printf("failed to get swapchain images: %i\n", result); // not sure if this is critical failure or not
		exit(1);
	}

	if(swapchain->images_count > SWAPCHAIN_IMAGES_MAX)
	{
		printf("too many swapchain images : %i > %i\n", swapchain->images_count, SWAPCHAIN_IMAGES_MAX);
		exit(1);
	}

	result = vkGetSwapchainImagesKHR(device, swapchain->swapchainKHR, &swapchain->images_count, swapchain->images);
	if(result != VK_SUCCESS)
	{
		printf("failed to get swapchain images: %i\n", result);
		exit(1);
	}
}

int main()
{
	// glfw init
	glfwSetErrorCallback(error_callback);
	glfwInit();

	if(!glfwVulkanSupported())
	{
		printf("not wow, no vulkan support\n");
		exit(1);
	}
	else
		printf("so wow, so vulkan\n");

	// starting up
	VkInstance instance = create_instance();
	VkPhysicalDevice physical_device;
	VkDevice device = create_device(instance, &physical_device);

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow * window = glfwCreateWindow(640, 480, "sup vulkan", NULL, NULL);
	glfwSetKeyCallback(window, key_callback);

	VkSurfaceKHR surface;
	VkResult result = glfwCreateWindowSurface(instance, window, NULL, &surface);
	if(result != VK_SUCCESS)
	{
		printf("failed creating window surface: %i\n", result);
		exit(1);
	}

	// setting up swap chain

	swapchain_t swapchain = figure_out_color_format(instance, physical_device, surface);

	swapchain.width = 640; // TODO
	swapchain.height = 480;

	create_swapchain(&swapchain, instance, physical_device, device, surface);

	VkSemaphore present_complete;
	VkSemaphore render_complete;

	VkSemaphoreCreateInfo semaphoreCreateInfo = {0};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = NULL;
	PFN_vkCreateSemaphore vkCreateSemaphore = (PFN_vkCreateSemaphore)glfwGetInstanceProcAddress(NULL, "vkCreateSemaphore");
	result = vkCreateSemaphore(device, &semaphoreCreateInfo, NULL, &present_complete);
	if(result != VK_SUCCESS)
	{
		printf("failed creating semaphore: %i\n", result);
		exit(1);
	}
	result = vkCreateSemaphore(device, &semaphoreCreateInfo, NULL, &render_complete);
	if(result != VK_SUCCESS)
	{
		printf("failed creating semaphore: %i\n", result);
		exit(1);
	}

	VkCommandPoolCreateInfo cmdPoolCreateInfo = {0};
	cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	VkCommandPool commandPool;
	PFN_vkCreateCommandPool vkCreateCommandPool = (PFN_vkCreateCommandPool)glfwGetInstanceProcAddress(NULL, "vkCreateCommandPool");
	result = vkCreateCommandPool(device, &cmdPoolCreateInfo, NULL, &commandPool);
	if(result != VK_SUCCESS)
	{
		printf("failed creating command pool: %i\n", result);
		exit(1);
	}

	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {0};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = commandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = 1;

	PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers = (PFN_vkAllocateCommandBuffers)glfwGetInstanceProcAddress(NULL, "vkAllocateCommandBuffers");
	VkCommandBuffer cmd;
	result = vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &cmd);
	if(result != VK_SUCCESS)
	{
		printf("failed creating command buffer: %i\n", result);
		exit(1);
	}

	for(uint32_t i = 0; i < swapchain.images_count; ++i)
	{
		VkImageViewCreateInfo colorAttachmentView = {0};
		colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		colorAttachmentView.pNext = NULL;
		colorAttachmentView.format = swapchain.color_format;
		colorAttachmentView.components.r = VK_COMPONENT_SWIZZLE_R;
		colorAttachmentView.components.g = VK_COMPONENT_SWIZZLE_G;
		colorAttachmentView.components.b = VK_COMPONENT_SWIZZLE_B;
		colorAttachmentView.components.a = VK_COMPONENT_SWIZZLE_A;
		colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		colorAttachmentView.subresourceRange.baseMipLevel = 0;
		colorAttachmentView.subresourceRange.levelCount = 1;
		colorAttachmentView.subresourceRange.baseArrayLayer = 0;
		colorAttachmentView.subresourceRange.layerCount = 1;
		colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		colorAttachmentView.flags = 0; // mandatory

//		setImageLayout(commandBuffer,
//					   swapChain->buffers[i].image,
//					   VK_IMAGE_ASPECT_COLOR_BIT,
//					   VK_IMAGE_LAYOUT_UNDEFINED,
//					   VK_IMAGE_LAYOUT_PRESET_SRC_KHR);
//		colorAttachmentView.image = swapChain->buffers[i].image;
//		// Create the view
//		if (vkCreateImageView(swapChain->device,
//							  &colorAttachmentView,
//							  NULL,
//							  &swapChain->buffers[i].view) != VK_SUCCESS)

	}


	// run loop
	while(!glfwWindowShouldClose(window))
	{

		glfwPollEvents();
	}

	// cleaning up
	PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR = (PFN_vkDestroySurfaceKHR)glfwGetInstanceProcAddress(NULL, "vkDestroySurfaceKHR");
	vkDestroySurfaceKHR(instance, surface, NULL);

	glfwDestroyWindow(window);

	PFN_vkDestroyDevice vkDestroyDevice = (PFN_vkDestroyDevice)glfwGetInstanceProcAddress(NULL, "vkDestroyDevice");
	vkDestroyDevice(device, NULL);

	PFN_vkDestroyInstance vkDestroyInstance = (PFN_vkDestroyInstance)glfwGetInstanceProcAddress(NULL, "vkDestroyInstance");
	vkDestroyInstance(instance, NULL);

	return 0;
}
