#pragma once

#include <cstdint>

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>

class RawInputEvent final : public godot::RefCounted {
	GDCLASS(RawInputEvent, godot::RefCounted)

public:
	RawInputEvent() = default;
	~RawInputEvent() override = default;

	void set_raw_keycode(uint32_t p_value);
	uint32_t get_raw_keycode() const;

	void set_keycode(uint32_t p_value);
	uint32_t get_keycode() const;

	void set_pressed(bool p_value);
	bool is_pressed() const;

	void set_timestamp_usec(uint64_t p_value);
	uint64_t get_timestamp_usec() const;

	void set_native_timestamp_usec(uint64_t p_value);
	uint64_t get_native_timestamp_usec() const;

protected:
	static void _bind_methods();

private:
	uint32_t raw_keycode_ = 0;
	uint32_t keycode_ = 0;
	bool pressed_ = false;
	uint64_t timestamp_usec_ = 0;
	uint64_t native_timestamp_usec_ = 0;
};
