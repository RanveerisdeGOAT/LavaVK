#include "../include/LavaVK/Queue.hpp"

#include "LavaVK/Device.hpp"
#include "LavaVK/Sync.hpp"

namespace LavaVK {
    Queue::Queue(Device &device, uint32_t family) : m_family(family) {
        vkGetDeviceQueue(device.native(), m_family, 0, &m_queue);
    }


    void Queue::submit(const SubmitInfo &submitInfo, Fence &fence) const {
        vkQueueSubmit(
            m_queue,
            1,
            &submitInfo.native(),
            fence.native()
        );
    }


    void Queue::waitIdle() const {
        vkQueueWaitIdle(m_queue);
    }
}
