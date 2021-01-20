#include "stdafx.h"
#include "rgfx/renderer.h"

const uint32_t kMemMgrSize = 256 * 1024 * 1024;
const uint32_t kMemMgrAlign = 1024 * 1024;

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

static void size_callback(GLFWwindow* window, int width, int height)
{
	if (width == 0 || height == 0) return;

//	Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
//	if (app)
//	{
//		Application::GetApplication()->resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
//	}
}

int main(int argc, char* argv[])
{
//	Application* app = Application::GetApplication();

	if (!glfwInit())
	{
		rsys_print("glfwInit failed.\n");
		exit(EXIT_FAILURE);
	}

	if (!glfwVulkanSupported())
	{
		rsys_print("glfw could not fine Vulkan Loader or ICD.\n");
		exit(1);
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	const GLFWvidmode* primaryScreenMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	uint32_t startUpScreenWidth = (uint32_t)((float)primaryScreenMode->width * 0.6f);
	uint32_t startUpScreenHeight = (uint32_t)((float)primaryScreenMode->height * 0.6f);
	GLFWwindow* window = glfwCreateWindow(startUpScreenWidth, startUpScreenHeight, "VulkanL", NULL /*glfwGetPrimaryMonitor()*/, NULL);

//	uint8_t* memoryBlock = static_cast<uint8_t*>(_aligned_malloc(kMemMgrSize, kMemMgrAlign));
//	LinearAllocator allocator(memoryBlock, kMemMgrSize);
	{
//		ScopeStack systemScope(allocator, "System");

//		RenderDevice* pRenderDevice = systemScope.newObject<RenderDevice>(systemScope, static_cast<uint32_t>(primaryScreenMode->width), static_cast<uint32_t>(primaryScreenMode->height), window);
		rgfx_initialize(&(rgfx_initParams) {
			.width = startUpScreenWidth,
			.height = startUpScreenHeight,
			.eyeWidth = 0,
			.eyeHeight = 0,
            .glfwWindow = window,
		});

#if defined(WIN32)
//		app->setGLFWwindow(window);
#endif
//		ScopeStack initScope(systemScope, "AppInit");

//		app->initialize(initScope, *pRenderDevice);

//		app->resize(startUpScreenWidth, startUpScreenHeight);	// Force the app to resize.  This will recreate the swapchain which initially gets created at Monitor resolution.

//		glfwSetWindowUserPointer(window, app);
		glfwSetKeyCallback(window, key_callback);
		glfwSetWindowSizeCallback(window, size_callback);

		uint64_t frameNum = 0;
		while (!glfwWindowShouldClose(window))
		{
//			char frameName[16];
//			sprintf_s(frameName, sizeof(frameName), "Frame %lld", frameNum++);
//			ScopeStack frameScope(initScope, frameName, false);

            rgfx_update();
            rgfx_render();
//			app->update(frameScope, *pRenderDevice);
//			app->render(frameScope, *pRenderDevice);

			glfwPollEvents();
//			if (app->getWasResized())
			{
//				pRenderDevice->recreateSwapChain(frameScope);
//				app->clearWasResized();
			}
		}
	}
	glfwDestroyWindow(window);
}
