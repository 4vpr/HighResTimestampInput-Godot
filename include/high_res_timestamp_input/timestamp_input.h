#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <commctrl.h>

#include <array>
#include <atomic>
#include <cstdint>

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/typed_array.hpp>

class RawInputEvent;

class TimestampInput final : public godot::Object {
	GDCLASS(TimestampInput, godot::Object)

public:
	struct RawKeyEvent {
		uint32_t keycode = 0;
		uint32_t godot_keycode = 0;
		bool pressed = false;
		uint64_t timestamp_usec = 0;
	};

	TimestampInput();
	~TimestampInput() override;

	static TimestampInput *get_singleton();

	bool start();
	void stop();
	godot::TypedArray<RawInputEvent> poll_events();
	uint64_t get_time_usec() const;

protected:
	static void _bind_methods();

private:
	class EventRingBuffer {
	public:
		static constexpr uint32_t CAPACITY = 4096;

		bool push(const RawKeyEvent &p_event);
		bool pop(RawKeyEvent &r_event);

	private:
		std::array<RawKeyEvent, CAPACITY> events_ {};
		std::atomic<uint32_t> head_ {0};
		std::atomic<uint32_t> tail_ {0};
	};

	static constexpr UINT_PTR SUBCLASS_ID = 1;

	static LRESULT CALLBACK subclass_proc(
		HWND p_hwnd,
		UINT p_message,
		WPARAM p_wparam,
		LPARAM p_lparam,
		UINT_PTR p_subclass_id,
		DWORD_PTR p_ref_data
	);

	bool setup_rawkey_input();
	void clear_main_window_subclass();
	void push_focus_out_releases();
	void proc_raw_input(HRAWINPUT p_raw_input);
	uint32_t fix_vkey(const RAWKEYBOARD &p_keyboard) const;
	uint32_t vkey_to_godot_key(const RAWKEYBOARD &p_keyboard, uint32_t p_virtual_key) const;
	void sync_clock_offset();
	uint64_t get_native_time_usec() const;
	uint64_t align_to_godot_usec(uint64_t p_native_usec) const;
	uint64_t qpc_to_usec(uint64_t p_counter) const;

	static TimestampInput *singleton_;

	EventRingBuffer ring_buffer_;
	std::atomic<bool> running_ {false};
	std::atomic<uint64_t> dropped_events_ {0};

	HWND window_handle_ = nullptr;

	LARGE_INTEGER qpc_frequency_ {};
	std::atomic<int64_t> godot_clock_offset_usec_ {0};
	std::array<bool, 256> key_down_ {};
	std::array<uint32_t, 256> key_godot_codes_ {};
};
