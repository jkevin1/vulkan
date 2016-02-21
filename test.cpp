#include <vulkan/vulkan.h>
#include "glfw/include/GLFW/glfw3.h"
#include <stdlib.h>
#include <stdio.h>
#include <vector>

// TODO cmake will make glfw linking way easier
// TODO put all these structs and processes into helper functions/classes

VkInstance instance = nullptr;
VkDevice device = nullptr;
VkPhysicalDeviceMemoryProperties memory;
VkQueue queue;
VkFormat depthFormat;
GLFWwindow* window = nullptr;
VkSurfaceKHR surface;

VkResult createInstance() {
  // find extensions required by window
  const char* extensions[] = { "VK_KHR_xcb_surface" };
  // glfw helper function doesnt work...?

  // AppInfo is ignored for simplicity, not needed

  // set up vulkan parameters
  VkInstanceCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  info.ppEnabledExtensionNames = extensions;
  info.enabledExtensionCount = 1;

  // create vulkan instance
  return vkCreateInstance(&info, nullptr, &instance);
}

VkResult createDevice() {
  // find number of available gpus
  uint32_t nGPUs;
  VkResult error = vkEnumeratePhysicalDevices(instance, &nGPUs, nullptr);
  if (error) return error;

  // get all available gpus
  std::vector<VkPhysicalDevice> gpus(nGPUs);
  error = vkEnumeratePhysicalDevices(instance, &nGPUs, &gpus[0]);
  if (error) return error;

  // find first supported device (TODO find best, or better yet, use multiple)
  for (VkPhysicalDevice& gpu : gpus) {
    // find number of queues on this gpu
    uint32_t nQueues;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &nQueues, nullptr);
    if (nQueues < 1) continue;
    
    printf("Found %u queues on device %p\n", nQueues, gpu);

    // get all available queues for this gpu
    std::vector<VkQueueFamilyProperties> queues(nQueues);
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &nQueues, &queues[0]);
    
    // find first supported queue (TODO find best)
    for (uint32_t index; index < nQueues; index++) {

      // TODO this always fails on intel linux drivers as far as I can tell      
//    if (glfwGetPhysicalDevicePresentationSupport(instance, gpu, index)) {
      // hopefully this works with glfw instead
      if (queues[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        printf("Queue %u supported!\n", index);
        // set up queue parameters
        float priority = 0.0f;
        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.pQueuePriorities = &priority;
        queueInfo.queueFamilyIndex = index;
        queueInfo.queueCount = 1;

        // device extensions
        const char* extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

        // set up device parameters
        VkDeviceCreateInfo deviceInfo{};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.queueCreateInfoCount = 1;
        deviceInfo.pQueueCreateInfos = &queueInfo;
        deviceInfo.enabledExtensionCount = 1;
        deviceInfo.ppEnabledExtensionNames = extensions;

        // create the device
        error = vkCreateDevice(gpu, &deviceInfo, nullptr, &device);
        if (error) return error;

        // find the memory properties for this gpu
        vkGetPhysicalDeviceMemoryProperties(gpu, &memory);
        
        // get the queue requested previously
        vkGetDeviceQueue(device, index, 0, &queue);
        
        // make sure D24S8 is supported, TODO find best available format
        VkFormatProperties formatProps;
        vkGetPhysicalDeviceFormatProperties(gpu, VK_FORMAT_D24_UNORM_S8_UINT, &formatProps);
        if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
          return VkResult::VK_ERROR_FORMAT_NOT_SUPPORTED;
        // device is ready to use
        return VkResult::VK_SUCCESS;
      } else printf("Queue %u unsupported\n", index);
    }
  }

  // no suitable device was found, this error is descriptive enough
  return VkResult::VK_ERROR_DEVICE_LOST;
}

// eventually this will do something other than append a newline
void log(const char* msg) {
  printf("%s\n", msg);
}

void glfwError(int id, const char* msg) {
  fprintf(stderr, "GLFW error %d: %s\n", id, msg);
}

VkResult createWindow(const char* title, int width, int height) {
  // initialize GLFW
  glfwSetErrorCallback(&glfwError);
  if (!glfwInit()) return VkResult::VK_ERROR_INCOMPATIBLE_DISPLAY_KHR;

  // create window for vulkan
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (!window) return VkResult::VK_ERROR_INCOMPATIBLE_DISPLAY_KHR;

  // get vulkan surface
  return glfwCreateWindowSurface(instance, window, nullptr, &surface);
}

void shutdown() {
  // todo destroy swapchain/surface
  glfwTerminate(); // also destroys window
  // destroy things before instance
  if (device) { log("Destroying device"); vkDestroyDevice(device, nullptr); }
  if (instance) { log("Destroying vulkan instance"); vkDestroyInstance(instance, nullptr); }
}

// basically assert that does cleanup and prints message
void check(bool condition, const char* msg) {
  if (condition != true) {
    fprintf(stderr, "Error: %s\n", msg);
    shutdown();
    exit(EXIT_FAILURE);
  }
}

void initialize() {
  log("Creating vulkan instance");
  check(!createInstance(), "failed to create vulkan instance");
  log("Creating device");
  check(!createDevice(), "failed to create device");
  log("Creating window");
  check(!createWindow("test", 640, 480), "failed to create window");
}

int main(int argc, char* argv[]) {
  initialize();

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
//  todo swapchain
  }

  shutdown();
}
