#ifndef SMIDI_H
#define SMIDI_H

#if defined(_WIN32) && defined(_SMIDI_DLL_IMPLEMENTATION)
#define SMIDI_API __declspec(dllexport)
#elif defined(_WIN32) && defined(SMIDI_DLL)
#define SMIDI_API __declspec(dllimport)
#elif defined(__GNUC__) && defined(_SMIDI_DLL_IMPLEMENTATION)
#define SMIDI_API __attribute__((visibility("default")))
#else
#define SMIDI_API
#endif

// SMIDI C API
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define SMIDI_MAX_DEVICE_NAME_LENGTH 256

typedef struct smidi_device_info
{
    char name[SMIDI_MAX_DEVICE_NAME_LENGTH];
    unsigned int manufacturer;
    unsigned int product;
    unsigned int driver_major_version;
    unsigned int driver_minor_version;
} smidi_device_info;

typedef struct smidi_input_device smidi_input_device;
typedef struct smidi_output_device smidi_output_device;
typedef struct smidi_system smidi_system;
typedef long long smidi_time_stamp;

SMIDI_API smidi_system *smidi_create_system();
SMIDI_API void smidi_destroy_system(smidi_system* system);

SMIDI_API int smidi_system_get_output_device_count(smidi_system* system);
SMIDI_API int smidi_system_get_output_device_info(smidi_system* system, int device_info_index, smidi_device_info* out_device_info);
SMIDI_API smidi_output_device* smidi_system_create_output_device(smidi_system* system, const char *device_name);
SMIDI_API void smidi_destroy_output_device(smidi_output_device* output_device);
SMIDI_API int smidi_output_device_send_message(smidi_output_device* output_device, const void* buffer, int buffer_size);

SMIDI_API int smidi_system_get_input_device_count(smidi_system* system);
SMIDI_API int smidi_system_get_input_device_info(smidi_system* system, int device_info_index, smidi_device_info* out_device_info);
SMIDI_API smidi_input_device* smidi_system_create_input_device(smidi_system* system, const char *device_name);
SMIDI_API void smidi_destroy_input_device(smidi_input_device* input_device);
SMIDI_API int smidi_input_device_recieve_message(smidi_input_device* input_device, void* buffer, int buffer_size, smidi_time_stamp* time_stamp);

#ifdef __cplusplus
}
#endif // __cplusplus

// SMIDI C++ API
#ifdef __cplusplus

#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace smidi
{
    using time_stamp = smidi_time_stamp;
    using device_info = smidi_device_info;

    class SMIDI_API output_device
    {
      public:
        virtual size_t send(const uint8_t* data, size_t size) = 0;
    };

    class SMIDI_API input_device
    {
      public:
        virtual size_t receive(uint8_t* data, size_t size, time_stamp* time_stamp) = 0;
    };

    class SMIDI_API system
    {
      public:
        virtual const std::vector<device_info>& output_devices() const noexcept = 0;
        virtual std::unique_ptr<output_device> create_output_device(const std::string& name) = 0;

        virtual const std::vector<device_info>& input_devices() const noexcept = 0;
        virtual std::unique_ptr<input_device> create_input_device(const std::string& name) = 0;
    };

    SMIDI_API std::unique_ptr<system> create_system();
} // namespace smidi

#endif // __cplusplus

#endif // SMIDI_H
