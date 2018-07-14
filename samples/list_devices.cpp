#include "smidi/smidi.h"

#include <iostream>

std::ostream& operator<<(std::ostream& stream, const smidi::device_info& device_info)
{
    return stream << "name: " << device_info.name << ", manufacturer: " << device_info.manufacturer << ", product: " << device_info.product
                  << ", driver_version: " << device_info.driver_major_version << "." << device_info.driver_minor_version;
}

std::ostream& operator<<(std::ostream& stream, const std::vector<smidi::device_info>& device_infos)
{
    for (const smidi::device_info& device_info : device_infos)
    {
        stream << device_info << std::endl;
    }
    return stream;
}

int main(int argc, char* argv[])
{
    try
    {
        std::unique_ptr<smidi::system> system = smidi::create_system();
        std::cout << "output devices:" << std::endl << system->output_devices() << std::endl;
        std::cout << "input devices:" << std::endl << system->input_devices() << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << "error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
