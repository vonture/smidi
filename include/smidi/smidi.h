#ifndef SMIDI_H
#define SMIDI_H

#include <string>
#include <vector>
#include <memory>

namespace smidi
{
    class output_device
    {
    public:
        virtual void send(const uint8_t* data, size_t size) = 0;
    };

    class input_device
    {
    public:
        virtual size_t recieve(uint8_t* data, size_t size) = 0;
    };

    struct device_info
    {
        std::string name;
        uint32_t manufacturer;
        uint32_t product;
        uint32_t driver_major_version;
        uint32_t driver_minor_version;
    };

    class system
    {
    public:
        virtual const std::vector<device_info>& output_devices() const noexcept = 0;
        virtual std::unique_ptr<output_device> create_output_device(const std::string& name) = 0;

        virtual const std::vector<device_info>& input_devices() const noexcept = 0;
        virtual std::unique_ptr<input_device> create_input_device(const std::string& name) = 0;
    };

    std::unique_ptr<system> create_system();
}

#endif // SMIDI_H
