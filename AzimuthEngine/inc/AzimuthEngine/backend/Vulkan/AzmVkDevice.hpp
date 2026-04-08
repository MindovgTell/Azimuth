#pragma once 

#include <cstdint>
#include <vector>

namespace azm::backend
{
    struct PhysicalDevice 
    {
        
    };

    class VkPhysicalDevice 
    {
    private:
        std::vector<PhysicalDevice> _devices;
        int _devIdx = -1;
    public: 
        VkPhysicalDevice() {}
        ~VkPhysicalDevice() {}

        void init();

        std::uint32_t selectDevice();

        const PhysicalDevice& selected() const;

    private:
        void getDeviceAPIVersion(int DeviceIndex);

	    void getExtensions(int DeviceIndex);
    };
}