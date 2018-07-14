#include "smidi/smidi.h"
#include "smidi_ext/smidi_messages.h"

#include <array>
#include <assert.h>
#include <chrono>
#include <iostream>
#include <thread>

int main(int argc, char* argv[])
{
    try
    {
        std::unique_ptr<smidi::system> system = smidi::create_system();

        const std::vector<smidi::device_info>& devices = system->output_devices();
        if (devices.empty())
        {
            std::cout << "no devices available." << std::endl;
            return 0;
        }

        std::unique_ptr<smidi::output_device> device = system->create_output_device(devices.back().name);

        constexpr uint8_t channel = 5;
        constexpr uint8_t num_controllers = 16;
        constexpr uint8_t strobe_value = 127;
        for (uint8_t controller = 0; controller < num_controllers; controller++)
        {
            smidi::control_change_message message(channel, controller, strobe_value);
            size_t result = device->send(message_data(message), message_size(message));
            assert(result == message_size(message));

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(500ms);
        }

        constexpr uint8_t clear_value = 0;
        for (uint8_t controller = 0; controller < num_controllers; controller++)
        {
            smidi::control_change_message message(channel, controller, clear_value);
            size_t result = device->send(message_data(message), message_size(message));
            assert(result == message_size(message));
        }
    }
    catch (const std::exception& e)
    {
        std::cout << "error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
