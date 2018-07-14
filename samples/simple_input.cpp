#include "smidi/smidi.h"
#include "smidi_ext/smidi_messages.h"

#include <array>
#include <assert.h>
#include <iomanip>
#include <iostream>

struct message_printer
{
    void operator()(const smidi::empty_message& message) const {}

    void operator()(const smidi::system_exclusive_message& message) const
    {
        for (uint8_t byte : message.message())
        {
            std::cout << " " << std::setfill('0') << std::setw(2) << std::hex << static_cast<size_t>(byte);
        }
    }

    void operator()(const smidi::control_change_message& message) const
    {
        std::cout << " channel: " << static_cast<size_t>(message.channel());
        std::cout << " controller: " << static_cast<size_t>(message.controller());
        std::cout << " value: " << static_cast<size_t>(message.value());
    }
};

int main(int argc, char* argv[])
{
    try
    {
        std::unique_ptr<smidi::system> system = smidi::create_system();

        const std::vector<smidi::device_info>& devices = system->input_devices();
        if (devices.empty())
        {
            std::cout << "no devices available." << std::endl;
            return 0;
        }

        std::unique_ptr<smidi::input_device> device = system->create_input_device(devices.back().name);

        while (true)
        {
            size_t message_size = device->receive(nullptr, 0, nullptr);

            std::vector<uint8_t> buffer(message_size, 0);
            smidi::time_stamp time_stamp;
            size_t result = device->receive(buffer.data(), buffer.size(), &time_stamp);
            assert(result == message_size);

            smidi::message_variant message = smidi::message_from_data(buffer.data(), buffer.size());

            std::cout << "received message: ";
            std::cout << "time: " << time_stamp;
            std::visit(message_printer(), message);
            std::cout << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cout << "error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
