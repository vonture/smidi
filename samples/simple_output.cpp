#include "smidi/smidi.h"

#include <array>
#include <chrono>
#include <iostream>
#include <thread>

struct control_change_channel_message
{
    control_change_channel_message() = default;
    control_change_channel_message(const control_change_channel_message& other) = default;
    control_change_channel_message& operator=(const control_change_channel_message& other) = default;
    control_change_channel_message(uint8_t channel, uint8_t controller_number, uint8_t controller_value)
        : header((0xB << 4) | (channel & 0xF))
        , controller_number(controller_number)
        , controller_value(controller_value)
    {
    }

    uint8_t header = 0;
    uint8_t controller_number = 0;
    uint8_t controller_value = 0;
};
static_assert(sizeof(control_change_channel_message) == 3, "unexpected struct size.");

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
            control_change_channel_message message(channel, controller, strobe_value);
            device->send(reinterpret_cast<const uint8_t*>(&message), sizeof(message));

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(500ms);
        }

        constexpr uint8_t clear_value = 0;
        std::array<control_change_channel_message, num_controllers> clear_messages;
        for (uint8_t controller = 0; controller < num_controllers; controller++)
        {
            clear_messages[controller] = control_change_channel_message(channel, controller, clear_value);
        }
        device->send(reinterpret_cast<const uint8_t*>(clear_messages.data()), sizeof(clear_messages));
    }
    catch (const std::exception& e)
    {
        std::cout << "error: " << e.what();
    }
}
