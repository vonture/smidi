#include "smidi/smidi.h"

#include <assert.h>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <list>
#include <mutex>
#include <system_error>
#include <thread>

#include <Mmsystem.h>
#include <windows.h>

namespace smidi
{
    namespace winmm
    {
        void check_midi_return_value(MMRESULT value)
        {
            if (value != MMSYSERR_NOERROR)
            {
                throw std::system_error(std::error_code(value, std::system_category()));
            }
        }

        class output_device final : public smidi::output_device
        {
          public:
            output_device(UINT index)
            {
                check_midi_return_value(
                    midiOutOpen(&_midi_out, index, static_cast<DWORD_PTR>(NULL), static_cast<DWORD_PTR>(NULL), CALLBACK_NULL));

                _buffer_cleanup_thread = std::thread(std::bind(&output_device::cleanup_buffers, this));
            }

            virtual ~output_device()
            {
                _destroy_cleanup_thread = true;
                _pending_cleanup_messages_condition_variable.notify_one();
                _buffer_cleanup_thread.join();

                midiOutReset(_midi_out);
                midiOutClose(_midi_out);
            }

            void send(const uint8_t* data, size_t size) override
            {
                if (size == 0)
                {
                    return;
                }

                if (size > 3 || data[0] == 0xF0)
                {
                    send_buffered_message(data, size);
                }
                else
                {
                    send_single_message(data, size);
                }
            }

          private:
            void cleanup_buffers()
            {
                using namespace std::chrono_literals;

                while (true)
                {
                    std::unique_lock<decltype(_pending_cleanup_messages_mutex)> unique_lock(_pending_cleanup_messages_mutex);
                    _pending_cleanup_messages_condition_variable.wait_for(unique_lock, 10ms);

                    if (_destroy_cleanup_thread)
                    {
                        return;
                    }

                    while (!_pending_cleanup_messages.empty())
                    {
                        MIDIHDR& header = _pending_cleanup_messages.front()->header;
                        MMRESULT result = midiOutUnprepareHeader(_midi_out, &header, sizeof(header));
                        if (result == MIDIERR_STILLPLAYING)
                        {
                            continue;
                        }
                        check_midi_return_value(result);

                        _pending_cleanup_messages.pop_front();
                    }
                }
            }

            void send_buffered_message(const uint8_t* data, size_t size)
            {
                std::unique_ptr<buffered_message> message = std::make_unique<buffered_message>();

                message->data.assign(data, data + size);

                message->header.lpData = reinterpret_cast<LPSTR>(message->data.data());
                message->header.dwBufferLength = size;
                check_midi_return_value(midiOutPrepareHeader(_midi_out, &message->header, sizeof(message->header)));

                check_midi_return_value(midiOutLongMsg(_midi_out, &message->header, sizeof(message->header)));

                {
                    std::lock_guard<decltype(_pending_cleanup_messages_mutex)> lock(_pending_cleanup_messages_mutex);
                    _pending_cleanup_messages.push_back(std::move(message));
                }
                _pending_cleanup_messages_condition_variable.notify_one();
            }

            void send_single_message(const uint8_t* data, size_t size)
            {
                assert(size > 0 && size <= 3);
                assert(sizeof(DWORD) == 4);

                DWORD message = 0;
                memcpy(&message, data, size);

                check_midi_return_value(midiOutShortMsg(_midi_out, message));
            }

            HMIDIOUT _midi_out = nullptr;

            struct buffered_message
            {
                MIDIHDR header = {0};
                std::vector<uint8_t> data;
            };

            std::thread _buffer_cleanup_thread;
            bool _destroy_cleanup_thread = false;
            std::list<std::unique_ptr<buffered_message>> _pending_cleanup_messages;
            std::mutex _pending_cleanup_messages_mutex;
            std::condition_variable _pending_cleanup_messages_condition_variable;
        };

        template <typename caps_type>
        device_info generate_device_info(const caps_type& caps)
        {
            device_info info;
            info.name = std::string(caps.szPname);
            info.manufacturer = caps.wMid;
            info.product = caps.wPid;
            info.driver_major_version = (caps.vDriverVersion >> 8) & 0xFF;
            info.driver_minor_version = caps.vDriverVersion & 0xFF;
            return info;
        }

        std::vector<device_info> generate_output_device_list()
        {
            std::vector<device_info> devices;

            UINT devices_count = midiOutGetNumDevs();
            for (UINT device_idx = 0; device_idx < devices_count; device_idx++)
            {
                MIDIOUTCAPS caps = {0};
                check_midi_return_value(midiOutGetDevCaps(static_cast<UINT_PTR>(device_idx), &caps, sizeof(caps)));
                devices.push_back(generate_device_info(caps));
            }

            return std::move(devices);
        }

        std::vector<device_info> generate_input_device_list()
        {
            std::vector<device_info> devices;

            UINT devices_count = midiInGetNumDevs();
            for (UINT device_idx = 0; device_idx < devices_count; device_idx++)
            {
                MIDIINCAPS caps = {0};
                check_midi_return_value(midiInGetDevCaps(static_cast<UINT_PTR>(device_idx), &caps, sizeof(caps)));
                devices.push_back(generate_device_info(caps));
            }

            return std::move(devices);
        }

        UINT find_device_index(const std::vector<device_info>& devices, const std::string& name)
        {
            for (UINT device_idx = 0; device_idx < devices.size(); device_idx++)
            {
                if (devices[device_idx].name == name)
                {
                    return device_idx;
                }
            }

            throw std::exception("no device with provided name.");
        }

        class system final : public smidi::system
        {
          public:
            system()
                : _output_devices(std::move(generate_output_device_list()))
                , _input_devices(std::move(generate_input_device_list()))
            {
            }

            const std::vector<device_info>& output_devices() const noexcept override
            {
                return _output_devices;
            }

            std::unique_ptr<smidi::output_device> create_output_device(const std::string& name) override
            {
                return std::make_unique<winmm::output_device>(find_device_index(_output_devices, name));
            }

            const std::vector<device_info>& input_devices() const noexcept
            {
                return _input_devices;
            }

            std::unique_ptr<input_device> create_input_device(const std::string& name)
            {
                return nullptr;
            }

          private:
            const std::vector<device_info> _output_devices;
            const std::vector<device_info> _input_devices;
        };
    }

    std::unique_ptr<system> create_system()
    {
        return std::make_unique<winmm::system>();
    }
}
