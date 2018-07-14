#include "smidi/smidi.h"

#include <algorithm>
#include <assert.h>
#include <iostream>

// C API
#define SMIDI_LOG_ERROR(msg) std::cerr << "SMIDI ERROR: " << __func__ << ": " << msg << std::endl

smidi_system* smidi_create_system()
{
    try
    {
        std::unique_ptr<smidi::system> system = smidi::create_system();
        return reinterpret_cast<smidi_system*>(system.release());
    }
    catch (const std::exception& e)
    {
        SMIDI_LOG_ERROR(e.what());
        return nullptr;
    }
}

void smidi_destroy_system(smidi_system* system)
{
    if (system == nullptr)
    {
        SMIDI_LOG_ERROR("NULL system.");
        return;
    }

    smidi::system* sys = reinterpret_cast<smidi::system*>(system);
    try
    {
        delete sys;
    }
    catch (const std::exception& e)
    {
        SMIDI_LOG_ERROR(e.what());
    }
}

int smidi_system_get_output_device_count(smidi_system* system)
{
    if (system == nullptr)
    {
        return 0;
    }

    smidi::system* sys = reinterpret_cast<smidi::system*>(system);
    try
    {
        size_t output_device_count = sys->output_devices().size();
        assert(output_device_count < std::numeric_limits<int>::max());
        return static_cast<int>(output_device_count);
    }
    catch (const std::exception& e)
    {
        SMIDI_LOG_ERROR(e.what());
        return 0;
    }
}

int smidi_system_get_output_device_info(smidi_system* system, int device_info_index, smidi_device_info* out_device_info)
{
    if (system == nullptr)
    {
        SMIDI_LOG_ERROR("NULL system.");
        return 0;
    }

    smidi::system* sys = reinterpret_cast<smidi::system*>(system);

    try
    {
        if (device_info_index < 0 || static_cast<size_t>(device_info_index) >= sys->output_devices().size())
        {
            SMIDI_LOG_ERROR("Invalid output device index.");
            return 0;
        }

        memcpy(out_device_info, &sys->output_devices()[device_info_index], sizeof(smidi_device_info));
        return 1;
    }
    catch (const std::exception& e)
    {
        SMIDI_LOG_ERROR(e.what());
        return 0;
    }
}

smidi_output_device* smidi_system_create_output_device(smidi_system* system, const char* device_name)
{
    if (system == nullptr)
    {
        SMIDI_LOG_ERROR("NULL system.");
        return nullptr;
    }

    smidi::system* sys = reinterpret_cast<smidi::system*>(system);

    try
    {
        std::unique_ptr<smidi::output_device> output_device = sys->create_output_device(device_name);
        return reinterpret_cast<smidi_output_device*>(output_device.release());
    }
    catch (const std::exception& e)
    {
        SMIDI_LOG_ERROR(e.what());
        return nullptr;
    }
}

void smidi_destroy_output_device(smidi_output_device* output_device)
{
    if (output_device == nullptr)
    {
        SMIDI_LOG_ERROR("NULL output device.");
        return;
    }

    smidi::output_device* dev = reinterpret_cast<smidi::output_device*>(output_device);

    try
    {
        delete dev;
    }
    catch (const std::exception& e)
    {
        SMIDI_LOG_ERROR(e.what());
    }
}

int smidi_output_device_send_message(smidi_output_device* output_device, const void* buffer, int buffer_size)
{
    if (output_device == nullptr)
    {
        SMIDI_LOG_ERROR("NULL output device.");
        return 0;
    }

    if (buffer_size < 0)
    {
        SMIDI_LOG_ERROR("Invalid buffer size.");
        return 0;
    }

    smidi::output_device* dev = reinterpret_cast<smidi::output_device*>(output_device);

    try
    {
        size_t result = dev->send(static_cast<const uint8_t*>(buffer), static_cast<size_t>(buffer_size));
        assert(result < std::numeric_limits<int>::max());
        return static_cast<int>(result);
    }
    catch (const std::exception& e)
    {
        SMIDI_LOG_ERROR(e.what());
        return 0;
    }
}

int smidi_system_get_input_device_count(smidi_system* system)
{
    if (system == nullptr)
    {
        SMIDI_LOG_ERROR("NULL system.");
        return 0;
    }

    smidi::system* sys = reinterpret_cast<smidi::system*>(system);

    try
    {
        size_t input_device_count = sys->input_devices().size();
        assert(input_device_count < std::numeric_limits<int>::max());
        return static_cast<int>(input_device_count);
    }
    catch (const std::exception& e)
    {
        SMIDI_LOG_ERROR(e.what());
        return 0;
    }
}

int smidi_system_get_input_device_info(smidi_system* system, int device_info_index, smidi_device_info* out_device_info)
{
    if (system == nullptr)
    {
        SMIDI_LOG_ERROR("NULL system.");
        return 0;
    }

    smidi::system* sys = reinterpret_cast<smidi::system*>(system);

    try
    {
        if (device_info_index < 0 || static_cast<size_t>(device_info_index) >= sys->input_devices().size())
        {
            SMIDI_LOG_ERROR("Invalid input device index.");
            return 0;
        }

        memcpy(out_device_info, &sys->input_devices()[device_info_index], sizeof(smidi_device_info));
        return 1;
    }
    catch (const std::exception& e)
    {
        SMIDI_LOG_ERROR(e.what());
        return 0;
    }
}

smidi_input_device* smidi_system_create_input_device(smidi_system* system, const char* device_name)
{
    if (system == nullptr)
    {
        SMIDI_LOG_ERROR("NULL system.");
        return nullptr;
    }

    smidi::system* sys = reinterpret_cast<smidi::system*>(system);

    try
    {
        std::unique_ptr<smidi::input_device> input_device = sys->create_input_device(device_name);
        return reinterpret_cast<smidi_input_device*>(input_device.release());
    }
    catch (const std::exception& e)
    {
        SMIDI_LOG_ERROR(e.what());
        return nullptr;
    }
}

void smidi_destroy_input_device(smidi_input_device* input_device)
{
    if (input_device == nullptr)
    {
        SMIDI_LOG_ERROR("NULL input device.");
        return;
    }

    try
    {
        smidi::input_device* dev = reinterpret_cast<smidi::input_device*>(input_device);
        delete dev;
    }
    catch (const std::exception& e)
    {
        SMIDI_LOG_ERROR(e.what());
    }
}

int smidi_input_device_recieve_message(smidi_input_device* input_device, void* buffer, int buffer_size, smidi_time_stamp* time_stamp)
{
    if (input_device == nullptr)
    {
        SMIDI_LOG_ERROR("NULL input device.");
        return 0;
    }

    if (buffer_size < 0)
    {
        SMIDI_LOG_ERROR("Invalid buffer size.");
        return 0;
    }

    smidi::input_device* dev = reinterpret_cast<smidi::input_device*>(input_device);
    try
    {
        size_t result = dev->receive(static_cast<uint8_t*>(buffer), static_cast<size_t>(buffer_size), time_stamp);
        assert(result < std::numeric_limits<int>::max());
        return static_cast<int>(result);
    }
    catch (const std::exception& e)
    {
        SMIDI_LOG_ERROR(e.what());
        return 0;
    }
}
