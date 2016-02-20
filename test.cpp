#include <vulkan/vulkan.h>
#include "glfw/include/GLFW/glfw3.h"
#include <stdlib.h>
#include <stdio.h>
#include <vector>

// TODO cmake will make glfw linking way easier
// TODO put all these structs and processes into helper functions/classes

const char** pExtensions;
int nExtensions;
VkInstance instance = nullptr;
VkDevice device = nullptr;

VkResult createInstance() {
  // find extensions required by window
  pExtensions = glfwGetRequiredInstanceExtensions(&nExtensions);
  
  // set up vulkan parameters
  VkInstanceCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  info.ppEnabledExtensionNames = pExtensions;
  info.enabledExtensionCount = (uint32_t)nExtensions;

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

  // find first supported device (TODO find best)
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
        return vkCreateDevice(gpu, &deviceInfo, nullptr, &device);
        
        // TODO MemoryProperties, Queue, DepthFormat etc
      } else printf("Queue %u unsupported\n", index);
    }
  }

  // no suitable device was found
  return VkResult::VK_ERROR_DEVICE_LOST;
}

void log(const char* msg) {
  printf("%s\n", msg);
}

void shutdown() {
  // destroy things before instance
  if (device) { log("Destroying device"); vkDestroyDevice(device, nullptr); }
  if (instance) { log("Destroying vulkan instance"); vkDestroyInstance(instance, nullptr); }
}

void check(bool condition, const char* msg) {
  if (condition != true) {
    fprintf(stderr, "Error: %s\n", msg);
    shutdown();
    exit(EXIT_FAILURE);
  ;}
}

void initialize() {
  log("Creating vulkan instance");
  check(!createInstance(), "failed to create vulkan instance");
  log("Creating device");
  check(!createDevice(), "failed to create device");
}

int main(int argc, char* argv[]) {
  initialize();

  shutdown();
}
