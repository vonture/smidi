#include "smidi/smidi.h"

#include <array>
#include <assert.h>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <list>
#include <mutex>
#include <optional>
#include <system_error>
#include <thread>
#include <type_traits>

#define NOMINMAX
#include <windows.h>

#include <Mmsystem.h>

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

        using shared_midi_out_ptr = std::shared_ptr<std::remove_pointer<HMIDIOUT>::type>;
        using shared_midi_in_ptr = std::shared_ptr<std::remove_pointer<HMIDIIN>::type>;

        class buffer
        {
          public:
            buffer(const uint8_t* initial_data, size_t size, DWORD_PTR user, DWORD flags)
                : _data(size)
            {
                if (initial_data)
                {
                    memcpy(_data.data(), initial_data, size);
                }

                _header.lpData = reinterpret_cast<LPSTR>(_data.data());
                _header.dwBufferLength = size;
                _header.dwUser = user;
                _header.dwFlags = flags;
            }

            virtual ~buffer() = default;

            MIDIHDR& header() noexcept
            {
                return _header;
            }

            const MIDIHDR& header() const noexcept
            {
                return _header;
            }

            uint8_t* data() noexcept
            {
                return _data.data();
            }

            const uint8_t* data() const noexcept
            {
                return _data.data();
            }

            size_t size() const noexcept
            {
                return _data.size();
            }

          private:
            MIDIHDR _header = {0};
            std::vector<uint8_t> _data;
        };

        class output_buffer : public buffer
        {
          public:
            output_buffer(shared_midi_out_ptr midi, const uint8_t* input_data, size_t size)
                : buffer(input_data, size, 0, 0)
                , _midi(midi)
            {
                check_midi_return_value(midiOutPrepareHeader(_midi.get(), &header(), sizeof(decltype(header()))));
            }

            virtual ~output_buffer()
            {
                check_midi_return_value(unprepare_header());
            }

            MMRESULT unprepare_header()
            {
                MMRESULT result = MMSYSERR_NOERROR;
                if (_midi)
                {
                    result = midiOutUnprepareHeader(_midi.get(), &header(), sizeof(decltype(header())));
                }
                if (result == MMSYSERR_NOERROR)
                {
                    _midi = nullptr;
                }
                return result;
            }

          private:
            shared_midi_out_ptr _midi;
        };

        struct input_buffer : public buffer
        {
          public:
            input_buffer(shared_midi_in_ptr midi, size_t size)
                : buffer(nullptr, size, reinterpret_cast<DWORD_PTR>(this), 0)
                , _midi(midi)
            {
                check_midi_return_value(midiInPrepareHeader(_midi.get(), &header(), sizeof(decltype(header()))));
            }

            virtual ~input_buffer()
            {
                check_midi_return_value(midiInUnprepareHeader(_midi.get(), &header(), sizeof(decltype(header()))));
            }

          private:
            shared_midi_in_ptr _midi;
        };

        class output_device final : public smidi::output_device
        {
          public:
            output_device(UINT index)
            {
                HMIDIOUT midi_out = nullptr;
                check_midi_return_value(
                    midiOutOpen(&midi_out, index, static_cast<DWORD_PTR>(NULL), static_cast<DWORD_PTR>(NULL), CALLBACK_NULL));
                _midi_out.reset(midi_out, [](HMIDIOUT midi_out) {
                    midiOutReset(midi_out);
                    midiOutClose(midi_out);
                });

                _buffer_cleanup_thread = std::thread(std::bind(&output_device::cleanup_buffers, this));
            }

            virtual ~output_device()
            {
                _destroy_cleanup_thread = true;
                _pending_cleanup_buffers_cv.notify_one();
                _buffer_cleanup_thread.join();
            }

            size_t send(const uint8_t* data, size_t size) override
            {
                if (data == nullptr)
                {
                    throw std::invalid_argument("NULL buffer.");
                }

                if (size == 0)
                {
                    throw std::invalid_argument("Invalid buffer size.");
                }

                constexpr uint8_t system_exclusive_message_status = 0xF0;
                if (data[0] == system_exclusive_message_status)
                {
                    send_buffered_message(data, size);
                }
                else
                {
                    send_single_message(data, size);
                }

                return size;
            }

          private:
            void cleanup_buffers()
            {
                using namespace std::chrono_literals;

                while (true)
                {
                    std::unique_lock<decltype(_pending_cleanup_buffers_mutex)> unique_lock(_pending_cleanup_buffers_mutex);
                    _pending_cleanup_buffers_cv.wait_for(unique_lock, 10ms);

                    if (_destroy_cleanup_thread)
                    {
                        return;
                    }

                    while (!_pending_cleanup_buffers.empty())
                    {
                        output_buffer* pending_cleanup_buffer = _pending_cleanup_buffers.front().get();
                        MMRESULT result = pending_cleanup_buffer->unprepare_header();
                        if (result == MIDIERR_STILLPLAYING)
                        {
                            continue;
                        }
                        check_midi_return_value(result);

                        _pending_cleanup_buffers.pop_front();
                    }
                }
            }

            void send_buffered_message(const uint8_t* data, size_t size)
            {
                std::unique_ptr<output_buffer> message = std::make_unique<output_buffer>(_midi_out, data, size);
                check_midi_return_value(midiOutLongMsg(_midi_out.get(), &message->header(), sizeof(decltype(message->header()))));

                {
                    std::lock_guard<decltype(_pending_cleanup_buffers_mutex)> lock(_pending_cleanup_buffers_mutex);
                    _pending_cleanup_buffers.push_back(std::move(message));
                }
                _pending_cleanup_buffers_cv.notify_one();
            }

            void send_single_message(const uint8_t* data, size_t size)
            {
                assert(size > 0 && size <= 3);
                assert(sizeof(DWORD) == 4);

                DWORD message = 0;
                memcpy(&message, data, size);

                check_midi_return_value(midiOutShortMsg(_midi_out.get(), message));
            }

            shared_midi_out_ptr _midi_out;

            std::thread _buffer_cleanup_thread;
            bool _destroy_cleanup_thread = false;
            std::list<std::unique_ptr<output_buffer>> _pending_cleanup_buffers;
            std::mutex _pending_cleanup_buffers_mutex;
            std::condition_variable _pending_cleanup_buffers_cv;
        };

        class input_device final : public smidi::input_device
        {
          public:
            input_device(UINT index)
            {
                HMIDIIN midi_in = nullptr;
                check_midi_return_value(midiInOpen(&midi_in, index, reinterpret_cast<DWORD_PTR>(&midi_input_proc),
                                                   reinterpret_cast<DWORD_PTR>(this), CALLBACK_FUNCTION));
                _midi_in.reset(midi_in, [](HMIDIIN midi_in) {
                    midiInReset(midi_in);
                    midiInClose(midi_in);
                });

                for (std::unique_ptr<input_buffer>& buffer : _input_buffers)
                {
                    buffer = std::make_unique<input_buffer>(_midi_in, buffer_size);
                    check_midi_return_value(midiInAddBuffer(_midi_in.get(), &buffer->header(), sizeof(decltype(buffer->header()))));
                }

                check_midi_return_value(midiInStart(_midi_in.get()));
            }

            virtual ~input_device() {}

            size_t receive(uint8_t* data, size_t size, time_stamp* time_stamp) override
            {
                std::unique_lock<decltype(_messages_mutex)> unique_lock(_messages_mutex);
                _messages_cv.wait(unique_lock, [this]() { return !_messages.empty(); });

                assert(!_messages.empty());

                if (data != nullptr)
                {
                    timestamped_message message = std::move(_messages.front());
                    _messages.pop_front();

                    if (size < message.first.size())
                    {
                        throw std::invalid_argument("Buffer size is not large enough.");
                    }
                    std::copy(message.first.begin(), message.first.end(), data);

                    if (time_stamp != nullptr)
                    {
                        *time_stamp = message.second;
                    }

                    return message.first.size();
                }
                else
                {
                    // Just return the message size if data is null
                    return _messages.front().first.size();
                }
            }

          private:
            static void CALLBACK midi_input_proc(HMIDIIN midi_in, UINT message, DWORD_PTR instance, DWORD_PTR param1, DWORD_PTR param2)
            {
                input_device* device = reinterpret_cast<input_device*>(instance);
                device->on_message(message, param1, param2);
            }

            void on_message(UINT message, DWORD_PTR param1, DWORD_PTR param2)
            {
                std::vector<uint8_t> data;
                if (message == MIM_DATA)
                {
                    const uint8_t* midi_message = reinterpret_cast<uint8_t*>(&param1);
                    size_t message_size = sizeof(param1);
                    data.resize(message_size);
                    memcpy(data.data(), &param1, message_size);
                }
                else if (message == MIM_LONGDATA || message == MIM_LONGERROR)
                {
                    MIDIHDR& header = *reinterpret_cast<MIDIHDR*>(param1);
                    data.resize(header.dwBytesRecorded);
                    memcpy(data.data(), header.lpData, header.dwBytesRecorded);

                    // requeue the buffer
                    if (header.dwBytesRecorded > 0)
                    {
                        check_midi_return_value(midiInAddBuffer(_midi_in.get(), &header, sizeof(header)));
                    }
                }

                if (!data.empty())
                {
                    std::lock_guard<decltype(_messages_mutex)> lock(_messages_mutex);

                    if (!_first_time_stamp.has_value())
                    {
                        _first_time_stamp = time_stamp(param2);
                    }

                    timestamped_message message{
                        data,
                        time_stamp(param2) - _first_time_stamp.value(),
                    };

                    _messages.push_back(message);
                }

                _messages_cv.notify_one();
            }

            shared_midi_in_ptr _midi_in;

            static constexpr size_t buffer_count = 4;
            static constexpr size_t buffer_size = 1024;
            std::array<std::unique_ptr<input_buffer>, buffer_count> _input_buffers;

            using timestamped_message = std::pair<std::vector<uint8_t>, time_stamp>;
            std::deque<timestamped_message> _messages;
            std::optional<time_stamp> _first_time_stamp;
            std::mutex _messages_mutex;
            std::condition_variable _messages_cv;
        };

        template <typename caps_type>
        device_info generate_device_info(const caps_type& caps)
        {
            device_info info;
            memset(info.name, 0, sizeof(info.name));
            memcpy(info.name, caps.szPname, std::min(sizeof(info.name), sizeof(caps.szPname)));
            info.name[sizeof(info.name) - 1] = 0;
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

            std::unique_ptr<smidi::input_device> create_input_device(const std::string& name)
            {
                return std::make_unique<winmm::input_device>(find_device_index(_input_devices, name));
            }

          private:
            const std::vector<device_info> _output_devices;
            const std::vector<device_info> _input_devices;
        };
    } // namespace winmm

    std::unique_ptr<system> create_system()
    {
        return std::make_unique<winmm::system>();
    }
} // namespace smidi
