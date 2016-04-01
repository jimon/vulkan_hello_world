
#include <stdio.h>
#include <stdint.h>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

// such MSC
#ifdef _MSC_VER
#define alloca _alloca
#endif

// most of it based on https://gist.github.com/graphitemaster/e162a24e57379af840d4

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

VkDevice create_device(VkInstance instance)
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
	for(uint32_t i = 0; i < device_count; i++)
	{
		uint32_t queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_count, NULL);
		VkQueueFamilyProperties * family_properties = alloca(queue_family_count * sizeof(VkQueueFamilyProperties));
		vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_count, family_properties);

		for(uint32_t j = 0; j < queue_family_count; j++)
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

int main()
{
	glfwInit();

	if(!glfwVulkanSupported())
	{
		printf("not wow, no vulkan support\n");
		exit(1);
	}
	else
		printf("so wow, so vulkan\n");

	VkInstance instance = create_instance();
	VkDevice device = create_device(instance);

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow * window = glfwCreateWindow(640, 480, "sup vulkan", NULL, NULL);

	VkSurfaceKHR surface;
	VkResult result = glfwCreateWindowSurface(instance, window, NULL, &surface);
	if(result != VK_SUCCESS)
	{
		printf("failed creating window surface: %i\n", result);
		exit(1);
	}

	PFN_vkDestroyDevice vkDestroyDevice = (PFN_vkDestroyDevice)glfwGetInstanceProcAddress(NULL, "vkDestroyDevice");
	vkDestroyDevice(device, NULL);

	PFN_vkDestroyInstance vkDestroyInstance = (PFN_vkDestroyInstance)glfwGetInstanceProcAddress(NULL, "vkDestroyInstance");
	vkDestroyInstance(instance, NULL);

	return 0;
}
