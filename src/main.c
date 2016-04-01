
#include <stdio.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

int main()
{
	if(glfwVulkanSupported())
	{
		printf("wow\n");
	}

	return 0;
}