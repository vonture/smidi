#ifndef SMIDI_MESSAGES_H
#define SMIDI_MESSAGES_H

#include <stdint.h>
#include <variant>
#include <vector>
#include <array>

namespace smidi
{
    class empty_message
    {
    };

    class system_exclusive_message
    {
    public:
        system_exclusive_message() noexcept;
        system_exclusive_message(const uint8_t* data, size_t size) noexcept;
        system_exclusive_message(uint8_t manufacturer_id, const std::vector<uint8_t>& data) noexcept;
        system_exclusive_message(uint8_t manufacturer_id[3], const std::vector<uint8_t>& data) noexcept;

        std::vector<uint8_t> message() const noexcept;

        const uint8_t* data() const noexcept;
        size_t size() const noexcept;

    private:
        const std::vector<uint8_t> _data;
    };

    class control_change_message
    {
    public:
        control_change_message() noexcept;
        control_change_message(const uint8_t* data, size_t size) noexcept;
        control_change_message(uint8_t channel, uint8_t controller, uint8_t value) noexcept;

        uint8_t channel() const noexcept;
        uint8_t controller() const noexcept;
        uint8_t value() const noexcept;

        const uint8_t* data() const noexcept;
        size_t size() const noexcept;

    private:
        const std::array<uint8_t, 3> _data;
    };

    using message_variant = std::variant<empty_message, system_exclusive_message, control_change_message>;

    message_variant message_from_data(const uint8_t* data, size_t size);
    const uint8_t* message_data(const message_variant& message);
    size_t message_size(const message_variant& message);

    // System common messages
    bool is_system_common_message(uint8_t status) noexcept;
    bool is_system_exclusive_message(uint8_t status) noexcept;
    bool is_midi_time_code_quarter_message(uint8_t status) noexcept;
    bool is_song_position_pointer_message(uint8_t status) noexcept;
    bool is_song_select_message(uint8_t status) noexcept;
    bool is_tune_request_message(uint8_t status) noexcept;

    // System real time messages
    bool is_system_real_time_message(uint8_t status) noexcept;
    bool is_timing_clock_message(uint8_t status) noexcept;
    bool is_start_message(uint8_t status) noexcept;
    bool is_continue_message(uint8_t status) noexcept;
    bool is_stop_message(uint8_t status) noexcept;
    bool is_active_sensing_message(uint8_t status) noexcept;
    bool is_reset_message(uint8_t status) noexcept;

    // Channel messages
    bool is_channel_message(uint8_t status) noexcept;
    size_t get_channel(uint8_t status) noexcept;
    bool is_note_off_message(uint8_t status) noexcept;
    bool is_note_on_message(uint8_t status) noexcept;
    bool is_polyphonic_key_pressure_message(uint8_t status) noexcept;
    bool is_control_change_message(uint8_t status) noexcept;
    size_t get_controller(const uint8_t* data, size_t size) noexcept;
    bool is_channel_mode_message(const uint8_t* data, size_t size) noexcept;
    bool is_program_change_message(uint8_t status) noexcept;
    bool is_channel_pressure_message(uint8_t status) noexcept;
    bool is_pitch_bend_change_message(uint8_t status) noexcept;

    // Message helpers
    uint8_t message_status(const uint8_t* data, size_t size) noexcept;
    size_t message_length(const uint8_t* data, size_t size) noexcept;
    size_t system_exlusive_message_length(const uint8_t* data, size_t size) noexcept;
    size_t non_system_exclusive_message_length(uint8_t status) noexcept;
} // namespace smidi

#endif // SMIDI_MESSAGES_H
