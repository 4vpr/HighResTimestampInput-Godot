#include "high_res_timestamp_input/raw_input_event.h"

using namespace godot;

void RawInputEvent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_raw_keycode", "value"), &RawInputEvent::set_raw_keycode);
	ClassDB::bind_method(D_METHOD("get_raw_keycode"), &RawInputEvent::get_raw_keycode);
	ClassDB::bind_method(D_METHOD("set_keycode", "value"), &RawInputEvent::set_keycode);
	ClassDB::bind_method(D_METHOD("get_keycode"), &RawInputEvent::get_keycode);
	ClassDB::bind_method(D_METHOD("set_pressed", "value"), &RawInputEvent::set_pressed);
	ClassDB::bind_method(D_METHOD("is_pressed"), &RawInputEvent::is_pressed);
	ClassDB::bind_method(D_METHOD("set_timestamp_usec", "value"), &RawInputEvent::set_timestamp_usec);
	ClassDB::bind_method(D_METHOD("get_timestamp_usec"), &RawInputEvent::get_timestamp_usec);
	ClassDB::bind_method(D_METHOD("set_native_timestamp_usec", "value"), &RawInputEvent::set_native_timestamp_usec);
	ClassDB::bind_method(D_METHOD("get_native_timestamp_usec"), &RawInputEvent::get_native_timestamp_usec);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "raw_keycode"), "set_raw_keycode", "get_raw_keycode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "keycode"), "set_keycode", "get_keycode");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "pressed"), "set_pressed", "is_pressed");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "timestamp_usec"), "set_timestamp_usec", "get_timestamp_usec");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "native_timestamp_usec"), "set_native_timestamp_usec", "get_native_timestamp_usec");
}

void RawInputEvent::set_raw_keycode(uint32_t p_value) {
	raw_keycode_ = p_value;
}

uint32_t RawInputEvent::get_raw_keycode() const {
	return raw_keycode_;
}

void RawInputEvent::set_keycode(uint32_t p_value) {
	keycode_ = p_value;
}

uint32_t RawInputEvent::get_keycode() const {
	return keycode_;
}

void RawInputEvent::set_pressed(bool p_value) {
	pressed_ = p_value;
}

bool RawInputEvent::is_pressed() const {
	return pressed_;
}

void RawInputEvent::set_timestamp_usec(uint64_t p_value) {
	timestamp_usec_ = p_value;
}

uint64_t RawInputEvent::get_timestamp_usec() const {
	return timestamp_usec_;
}

void RawInputEvent::set_native_timestamp_usec(uint64_t p_value) {
	native_timestamp_usec_ = p_value;
}

uint64_t RawInputEvent::get_native_timestamp_usec() const {
	return native_timestamp_usec_;
}
