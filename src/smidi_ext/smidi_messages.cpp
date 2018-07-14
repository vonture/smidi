#include "smidi_ext/smidi_messages.h"
#include <assert.h>

namespace smidi
{
    namespace
    {
        constexpr uint8_t prefix(uint8_t status)
        {
            return status >> 4;
        }

        constexpr uint8_t stuffix(uint8_t status)
        {
            return status & 0xF;
        }

        constexpr uint8_t create_channel_message_status(uint8_t prefix, uint8_t channel)
        {
            assert((prefix & 0xF) == prefix);
            assert((channel & 0xF) == channel);
            return (prefix << 4) | channel;
        }

        constexpr uint8_t system_exclusive_message_status = 0xF0;
        constexpr uint8_t system_exclusive_message_footer = 0xF7;
        constexpr uint8_t midi_time_code_quarter_message_status = 0xF1;
        constexpr uint8_t song_position_pointer_message_status = 0xF2;
        constexpr uint8_t song_select_message_status = 0xF3;
        constexpr uint8_t tune_request_message_status = 0xF6;

        constexpr uint8_t timing_clock_message_status = 0xF8;
        constexpr uint8_t start_message_status = 0xFA;
        constexpr uint8_t continue_message_status = 0xFB;
        constexpr uint8_t stop_message_status = 0xFC;
        constexpr uint8_t active_sensing_message_status = 0xFE;
        constexpr uint8_t reset_message_status = 0xFF;

        constexpr uint8_t note_off_message_prefix = 0x8;
        constexpr uint8_t note_on_message_prefix = 0x9;
        constexpr uint8_t polyphonic_key_pressure_message_prefix = 0xA;
        constexpr uint8_t control_change_message_prefix = 0xB;
        constexpr uint8_t program_change_message_prefix = 0xC;
        constexpr uint8_t channel_pressure_message_prefix = 0xD;
        constexpr uint8_t pitch_bend_message_prefix = 0xE;

        std::vector<uint8_t> generate_system_exclusive_data(uint8_t manufacturer_id, const std::vector<uint8_t>& message)
        {
            std::vector<uint8_t> data;
            data.push_back(system_exclusive_message_status);
            data.push_back(manufacturer_id);
            data.insert(data.end(), message.begin(), message.end());
            data.push_back(system_exclusive_message_footer);

            return data;
        }

        std::vector<uint8_t> generate_system_exclusive_data(uint8_t manufacturer_id[3], const std::vector<uint8_t>& message)
        {
            std::vector<uint8_t> data;
            data.push_back(system_exclusive_message_status);
            data.insert(data.end(), manufacturer_id, manufacturer_id + 3);
            data.insert(data.end(), message.begin(), message.end());
            data.push_back(system_exclusive_message_footer);

            return data;
        }

        using message_data_info = std::pair<const uint8_t*, size_t>;

        template <typename message_type>
        bool get_message_data_info_typed(const message_variant& message, message_data_info& info)
        {
            if (std::holds_alternative<message_type>(message))
            {
                const message_type& typed_message = std::get<message_type>(message);
                info.first = typed_message.data();
                info.second = typed_message.size();
                return true;
            }
            else
            {
                return false;
            }
        }

        message_data_info get_message_data_info(const message_variant& message)
        {
            message_data_info info{nullptr, 0};
            if (!get_message_data_info_typed<system_exclusive_message>(message, info) &&
                !get_message_data_info_typed<control_change_message>(message, info))
            {
                assert(std::holds_alternative<empty_message>(message));
            }

            return info;
        }
    } // namespace

    system_exclusive_message::system_exclusive_message() noexcept
        : system_exclusive_message(0, {})
    {
    }

    system_exclusive_message::system_exclusive_message(const uint8_t* data, size_t size) noexcept
        : _data(data, data + size)
    {
        assert(size > 0);
        assert(is_system_exclusive_message(data[0]));
        assert(system_exlusive_message_length(data, size) == size);
    }

    system_exclusive_message::system_exclusive_message(uint8_t manufacturer_id, const std::vector<uint8_t>& data) noexcept
        : _data(generate_system_exclusive_data(manufacturer_id, data))
    {
    }

    system_exclusive_message::system_exclusive_message(uint8_t manufacturer_id[3], const std::vector<uint8_t>& data) noexcept
        : _data(generate_system_exclusive_data(manufacturer_id, data))
    {
    }

    std::vector<uint8_t> system_exclusive_message::message() const noexcept
    {
        return std::vector<uint8_t>(_data.data() + 1, _data.data() + _data.size() - 2);
    }

    const uint8_t* system_exclusive_message::data() const noexcept
    {
        return _data.data();
    }

    size_t system_exclusive_message::size() const noexcept
    {
        return _data.size();
    }

    control_change_message::control_change_message() noexcept
        : control_change_message(0, 0, 0)
    {
    }

    control_change_message::control_change_message(const uint8_t* data, size_t size) noexcept
        : _data{data[0], data[1], data[2]}
    {
        assert(is_control_change_message(data[0]));
    }

    control_change_message::control_change_message(uint8_t channel, uint8_t controller, uint8_t value) noexcept
        : _data{create_channel_message_status(control_change_message_prefix, channel), controller, value}
    {
        assert(controller <= 127);
        assert(value <= 127);
    }

    uint8_t control_change_message::channel() const noexcept
    {
        return stuffix(_data[0]);
    }

    uint8_t control_change_message::controller() const noexcept
    {
        return _data[1];
    }

    uint8_t control_change_message::value() const noexcept
    {
        return _data[2];
    }

    const uint8_t* control_change_message::data() const noexcept
    {
        return _data.data();
    }

    size_t control_change_message::size() const noexcept
    {
        return _data.size();
    }

    message_variant message_from_data(const uint8_t* data, size_t size)
    {
        size_t msg_len = message_length(data, size);
        if (size < msg_len)
        {
            return empty_message();
        }

        if (is_system_exclusive_message(data[0]))
        {
            return system_exclusive_message(data, size);
        }
        else if (is_control_change_message(data[0]))
        {
            return control_change_message(data, size);
        }
        else
        {
            return empty_message();
        }
    }

    const uint8_t* message_data(const message_variant& message)
    {
        return get_message_data_info(message).first;
    }

    size_t message_size(const message_variant& message)
    {
        return get_message_data_info(message).second;
    }

    bool is_system_common_message(uint8_t status) noexcept
    {
        return is_system_exclusive_message(status) || is_midi_time_code_quarter_message(status) ||
               is_song_position_pointer_message(status) || is_song_select_message(status) || is_tune_request_message(status);
    }

    bool is_system_exclusive_message(uint8_t status) noexcept
    {
        return status == system_exclusive_message_status;
    }

    bool is_midi_time_code_quarter_message(uint8_t status) noexcept
    {
        return status == midi_time_code_quarter_message_status;
    }

    bool is_song_position_pointer_message(uint8_t status) noexcept
    {
        return status == song_position_pointer_message_status;
    }

    bool is_song_select_message(uint8_t status) noexcept
    {
        return status == song_select_message_status;
    }

    bool is_tune_request_message(uint8_t status) noexcept
    {
        return status == tune_request_message_status;
    }

    bool is_system_real_time_message(uint8_t status) noexcept
    {
        return is_timing_clock_message(status) || is_start_message(status) || is_continue_message(status) || is_stop_message(status) ||
               is_active_sensing_message(status) || is_reset_message(status);
    }

    bool is_timing_clock_message(uint8_t status) noexcept
    {
        return status == timing_clock_message_status;
    }

    bool is_start_message(uint8_t status) noexcept
    {
        return status == start_message_status;
    }

    bool is_continue_message(uint8_t status) noexcept
    {
        return status == continue_message_status;
    }

    bool is_stop_message(uint8_t status) noexcept
    {
        return status == stop_message_status;
    }

    bool is_active_sensing_message(uint8_t status) noexcept
    {
        return status == active_sensing_message_status;
    }

    bool is_reset_message(uint8_t status) noexcept
    {
        return status == reset_message_status;
    }

    bool is_channel_message(uint8_t status) noexcept
    {
        return is_note_off_message(status) || is_note_on_message(status) || is_polyphonic_key_pressure_message(status) ||
               is_control_change_message(status) || is_program_change_message(status) || is_channel_pressure_message(status) ||
               is_pitch_bend_change_message(status);
    }

    size_t get_channel(uint8_t status) noexcept
    {
        return stuffix(status);
    }

    bool is_note_off_message(uint8_t status) noexcept
    {
        return prefix(status) == note_off_message_prefix;
    }

    bool is_note_on_message(uint8_t status) noexcept
    {
        return prefix(status) == note_on_message_prefix;
    }

    bool is_polyphonic_key_pressure_message(uint8_t status) noexcept
    {
        return prefix(status) == polyphonic_key_pressure_message_prefix;
    }

    bool is_control_change_message(uint8_t status) noexcept
    {
        return prefix(status) == control_change_message_prefix;
    }

    size_t get_controller(const uint8_t* data, size_t size) noexcept
    {
        if (size < 3 || is_control_change_message(data[0]))
        {
            return 0;
        }

        constexpr uint8_t controller_mask = 0x7F;
        return (data[1] & controller_mask);
    }

    bool is_channel_mode_message(const uint8_t* data, size_t size) noexcept
    {
        constexpr size_t min_channel_mode_controller = 120;
        constexpr size_t max_channel_mode_controller = 127;
        size_t controller = get_controller(data, size);
        return controller >= min_channel_mode_controller && controller <= max_channel_mode_controller;
    }

    bool is_program_change_message(uint8_t status) noexcept
    {
        return prefix(status) == program_change_message_prefix;
    }

    bool is_channel_pressure_message(uint8_t status) noexcept
    {
        return prefix(status) == channel_pressure_message_prefix;
    }

    bool is_pitch_bend_change_message(uint8_t status) noexcept
    {
        return prefix(status) == pitch_bend_message_prefix;
    }

    uint8_t message_status(const uint8_t* data, size_t size) noexcept
    {
        if (size == 0)
        {
            return 0;
        }

        return data[0];
    }

    size_t message_length(const uint8_t* data, size_t size) noexcept
    {
        if (size == 0)
        {
            return 0;
        }

        const uint8_t status = data[0];
        if (is_system_exclusive_message(status))
        {
            return system_exlusive_message_length(data, size);
        }
        else
        {
            return non_system_exclusive_message_length(status);
        }
    }

    size_t system_exlusive_message_length(const uint8_t* data, size_t size) noexcept
    {
        if (size < 3 || is_system_exclusive_message(data[0]))
        {
            return 0;
        }

        const uint8_t* end = data + size;
        const uint8_t* end_byte = std::find(data, end, system_exclusive_message_footer);
        return (end_byte != end) ? end_byte - data : 0;
    }

    size_t non_system_exclusive_message_length(uint8_t status) noexcept
    {
        if (is_note_off_message(status) || is_note_on_message(status) || is_polyphonic_key_pressure_message(status) ||
            is_control_change_message(status) || is_pitch_bend_change_message(status) || is_song_position_pointer_message(status))
        {
            return 3;
        }
        else if (is_program_change_message(status) || is_channel_pressure_message(status) || is_midi_time_code_quarter_message(status) ||
                 is_song_select_message(status))
        {
            return 2;
        }
        else if (is_tune_request_message(status) || is_system_real_time_message(status))
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
} // namespace smidi
