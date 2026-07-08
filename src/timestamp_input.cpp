#include "high_res_timestamp_input/timestamp_input.h"
#include "high_res_timestamp_input/raw_input_event.h"

#include <cstddef>
#include <vector>

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

TimestampInput *TimestampInput::singleton_ = nullptr;

TimestampInput::TimestampInput() {
	singleton_ = this;
	QueryPerformanceFrequency(&qpc_frequency_);
	key_down_.fill(false);
	key_godot_codes_.fill(0);
}

TimestampInput::~TimestampInput() {
	stop();
	singleton_ = nullptr;
}

TimestampInput *TimestampInput::get_singleton() {
	return singleton_;
}

void TimestampInput::_bind_methods() {
	ClassDB::bind_method(D_METHOD("start"), &TimestampInput::start);
	ClassDB::bind_method(D_METHOD("stop"), &TimestampInput::stop);
	ClassDB::bind_method(D_METHOD("poll_events"), &TimestampInput::poll_events);
	ClassDB::bind_method(D_METHOD("get_time_usec"), &TimestampInput::get_time_usec);
}

bool TimestampInput::start() {
	if (running_.load(std::memory_order_acquire)) {
		return true;
	}

	DisplayServer *display_server = DisplayServer::get_singleton();
	if (display_server == nullptr) {
		UtilityFunctions::push_error("DisplayServer singleton is unavailable.");
		return false;
	}

	const int64_t native_window_handle = display_server->window_get_native_handle(DisplayServer::WINDOW_HANDLE);
	window_handle_ = reinterpret_cast<HWND>(native_window_handle);
	if (window_handle_ == nullptr) {
		UtilityFunctions::push_error("Failed to get Godot main window handle.");
		return false;
	}

	if (!SetWindowSubclass(
		window_handle_,
		&TimestampInput::subclass_proc,
		SUBCLASS_ID,
		reinterpret_cast<DWORD_PTR>(this)
	)) {
		window_handle_ = nullptr;
		UtilityFunctions::push_error("Failed to install the Godot main window subclass.");
		return false;
	}

	if (!setup_rawkey_input()) {
		clear_main_window_subclass();
		UtilityFunctions::push_error("Failed to register raw keyboard input on the Godot main window.");
		return false;
	}

	sync_clock_offset();
	key_down_.fill(false);
	key_godot_codes_.fill(0);
	running_.store(true, std::memory_order_release);
	return true;
}

void TimestampInput::stop() {
	if (!running_.exchange(false, std::memory_order_acq_rel)) {
		clear_main_window_subclass();
		return;
	}

	push_focus_out_releases();
	clear_main_window_subclass();
}

TypedArray<RawInputEvent> TimestampInput::poll_events() {
	TypedArray<RawInputEvent> events;
	RawKeyEvent event;

	while (ring_buffer_.pop(event)) {
		Ref<RawInputEvent> event_obj;
		event_obj.instantiate();
		event_obj->set_raw_keycode(event.keycode);
		event_obj->set_keycode(event.godot_keycode);
		event_obj->set_pressed(event.pressed);
		event_obj->set_timestamp_usec(align_to_godot_usec(event.timestamp_usec));
		event_obj->set_native_timestamp_usec(event.timestamp_usec);
		events.push_back(event_obj);
	}

	const uint64_t dropped = dropped_events_.exchange(0, std::memory_order_acq_rel);
	if (dropped > 0) {
		UtilityFunctions::push_warning(
			String("TimestampInput dropped ") +
			String::num_uint64(dropped) +
			" events because the ring buffer was full."
		);
	}

	return events;
}

uint64_t TimestampInput::get_time_usec() const {
	return align_to_godot_usec(get_native_time_usec());
}

bool TimestampInput::EventRingBuffer::push(const RawKeyEvent &p_event) {
	const uint32_t head = head_.load(std::memory_order_relaxed);
	const uint32_t next = (head + 1) % CAPACITY;
	const uint32_t tail = tail_.load(std::memory_order_acquire);

	if (next == tail) {
		return false;
	}

	events_[head] = p_event;
	head_.store(next, std::memory_order_release);
	return true;
}

bool TimestampInput::EventRingBuffer::pop(RawKeyEvent &r_event) {
	const uint32_t tail = tail_.load(std::memory_order_relaxed);
	const uint32_t head = head_.load(std::memory_order_acquire);

	if (tail == head) {
		return false;
	}

	r_event = events_[tail];
	tail_.store((tail + 1) % CAPACITY, std::memory_order_release);
	return true;
}

LRESULT CALLBACK TimestampInput::subclass_proc(
	HWND p_hwnd,
	UINT p_message,
	WPARAM p_wparam,
	LPARAM p_lparam,
	UINT_PTR p_subclass_id,
	DWORD_PTR p_ref_data
) {
	if (p_subclass_id != SUBCLASS_ID) {
		return DefSubclassProc(p_hwnd, p_message, p_wparam, p_lparam);
	}

	auto *instance = reinterpret_cast<TimestampInput *>(p_ref_data);
	if (instance == nullptr || p_hwnd != instance->window_handle_) {
		return DefSubclassProc(p_hwnd, p_message, p_wparam, p_lparam);
	}

	switch (p_message) {
		case WM_INPUT:
			instance->proc_raw_input(reinterpret_cast<HRAWINPUT>(p_lparam));
			break;

		case WM_KILLFOCUS:
			instance->push_focus_out_releases();
			break;

		case WM_ACTIVATEAPP:
			if (p_wparam == FALSE) {
				instance->push_focus_out_releases();
			}
			break;

		default:
			break;
	}

	return DefSubclassProc(p_hwnd, p_message, p_wparam, p_lparam);
}

bool TimestampInput::setup_rawkey_input() {
	RAWINPUTDEVICE device {};
	device.usUsagePage = 0x01;
	device.usUsage = 0x06;
	device.dwFlags = 0;
	device.hwndTarget = window_handle_;

	return RegisterRawInputDevices(&device, 1, sizeof(device)) == TRUE;
}

void TimestampInput::clear_main_window_subclass() {
	if (window_handle_ != nullptr && IsWindow(window_handle_)) {
		RemoveWindowSubclass(window_handle_, &TimestampInput::subclass_proc, SUBCLASS_ID);
	}

	window_handle_ = nullptr;
	key_down_.fill(false);
	key_godot_codes_.fill(0);
}

void TimestampInput::push_focus_out_releases() {
	const uint64_t timestamp_usec = get_native_time_usec();

	for (uint32_t keycode = 0; keycode < key_down_.size(); ++keycode) {
		if (!key_down_[keycode]) {
			continue;
		}

		key_down_[keycode] = false;

		const RawKeyEvent event {
			keycode,
			key_godot_codes_[keycode] != 0 ? key_godot_codes_[keycode] : vkey_to_godot_key(RAWKEYBOARD {}, keycode),
			false,
			timestamp_usec
		};

		key_godot_codes_[keycode] = 0;

		if (!ring_buffer_.push(event)) {
			dropped_events_.fetch_add(1, std::memory_order_acq_rel);
		}
	}
}

void TimestampInput::proc_raw_input(HRAWINPUT p_raw_input) {
	UINT size = 0;
	if (GetRawInputData(p_raw_input, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) != 0 || size == 0) {
		return;
	}

	std::vector<std::byte> buffer(size);
	if (GetRawInputData(p_raw_input, RID_INPUT, buffer.data(), &size, sizeof(RAWINPUTHEADER)) != size) {
		return;
	}

	const auto *raw = reinterpret_cast<const RAWINPUT *>(buffer.data());
	if (raw->header.dwType != RIM_TYPEKEYBOARD) {
		return;
	}

	const RAWKEYBOARD &keyboard = raw->data.keyboard;
	if (keyboard.VKey == 255) {
		return;
	}

	const uint32_t keycode = fix_vkey(keyboard);
	if (keycode == 0) {
		return;
	}
	const uint32_t godot_keycode = vkey_to_godot_key(keyboard, keycode);

	const bool pressed = (keyboard.Flags & RI_KEY_BREAK) == 0;
	if (keycode < key_down_.size()) {
		if (pressed) {
			if (key_down_[keycode]) {
				return;
			}
			key_down_[keycode] = true;
			key_godot_codes_[keycode] = godot_keycode;
		} else {
			if (!key_down_[keycode]) {
				return;
			}
			key_down_[keycode] = false;
		}
	}

	const RawKeyEvent event {
		keycode,
		godot_keycode,
		pressed,
		get_native_time_usec()
	};

	if (!ring_buffer_.push(event)) {
		dropped_events_.fetch_add(1, std::memory_order_acq_rel);
	}

	if (!pressed && keycode < key_godot_codes_.size()) {
		key_godot_codes_[keycode] = 0;
	}
}

uint32_t TimestampInput::fix_vkey(const RAWKEYBOARD &p_keyboard) const {
	UINT scan_code = p_keyboard.MakeCode;
	if ((p_keyboard.Flags & RI_KEY_E0) != 0) {
		scan_code |= 0xE000;
	}
	if ((p_keyboard.Flags & RI_KEY_E1) != 0) {
		scan_code |= 0xE100;
	}

	switch (p_keyboard.VKey) {
		case VK_SHIFT:
		case VK_CONTROL:
		case VK_MENU:
			return static_cast<uint32_t>(MapVirtualKeyW(scan_code, MAPVK_VSC_TO_VK_EX));

		default:
			return static_cast<uint32_t>(p_keyboard.VKey);
	}
}

uint32_t TimestampInput::vkey_to_godot_key(const RAWKEYBOARD &p_keyboard, uint32_t p_virtual_key) const {
	if (p_virtual_key >= '0' && p_virtual_key <= '9') {
		return p_virtual_key;
	}
	if (p_virtual_key >= 'A' && p_virtual_key <= 'Z') {
		return p_virtual_key;
	}
	if (p_virtual_key >= VK_F1 && p_virtual_key <= VK_F24) {
		return KEY_F1 + (p_virtual_key - VK_F1);
	}
	if (p_virtual_key >= VK_NUMPAD0 && p_virtual_key <= VK_NUMPAD9) {
		return KEY_KP_0 + (p_virtual_key - VK_NUMPAD0);
	}

	switch (p_virtual_key) {
		case VK_ESCAPE:
			return KEY_ESCAPE;
		case VK_TAB:
			return KEY_TAB;
		case VK_BACK:
			return KEY_BACKSPACE;
		case VK_RETURN:
			if ((p_keyboard.Flags & RI_KEY_E0) != 0) {
				return KEY_KP_ENTER;
			}
			return KEY_ENTER;
		case VK_SPACE:
			return KEY_SPACE;
		case VK_INSERT:
			return KEY_INSERT;
		case VK_DELETE:
			return KEY_DELETE;
		case VK_PAUSE:
			return KEY_PAUSE;
		case VK_SNAPSHOT:
			return KEY_PRINT;
		case VK_CLEAR:
			return KEY_CLEAR;
		case VK_HOME:
			return KEY_HOME;
		case VK_END:
			return KEY_END;
		case VK_LEFT:
			return KEY_LEFT;
		case VK_UP:
			return KEY_UP;
		case VK_RIGHT:
			return KEY_RIGHT;
		case VK_DOWN:
			return KEY_DOWN;
		case VK_PRIOR:
			return KEY_PAGEUP;
		case VK_NEXT:
			return KEY_PAGEDOWN;
		case VK_SHIFT:
		case VK_LSHIFT:
		case VK_RSHIFT:
			return KEY_SHIFT;
		case VK_CONTROL:
		case VK_LCONTROL:
		case VK_RCONTROL:
			return KEY_CTRL;
		case VK_MENU:
		case VK_LMENU:
		case VK_RMENU:
			return KEY_ALT;
		case VK_LWIN:
		case VK_RWIN:
			return KEY_META;
		case VK_CAPITAL:
			return KEY_CAPSLOCK;
		case VK_NUMLOCK:
			return KEY_NUMLOCK;
		case VK_SCROLL:
			return KEY_SCROLLLOCK;
		case VK_APPS:
			return KEY_MENU;
		case VK_MULTIPLY:
			return KEY_KP_MULTIPLY;
		case VK_DIVIDE:
			return KEY_KP_DIVIDE;
		case VK_SUBTRACT:
			return KEY_KP_SUBTRACT;
		case VK_DECIMAL:
			return KEY_KP_PERIOD;
		case VK_ADD:
			return KEY_KP_ADD;
		case VK_BROWSER_BACK:
			return KEY_BACK;
		case VK_BROWSER_FORWARD:
			return KEY_FORWARD;
		case VK_BROWSER_REFRESH:
			return KEY_REFRESH;
		case VK_BROWSER_STOP:
			return KEY_STOP;
		case VK_BROWSER_SEARCH:
			return KEY_SEARCH;
		case VK_BROWSER_FAVORITES:
			return KEY_FAVORITES;
		case VK_BROWSER_HOME:
			return KEY_HOMEPAGE;
		case VK_VOLUME_MUTE:
			return KEY_VOLUMEMUTE;
		case VK_VOLUME_DOWN:
			return KEY_VOLUMEDOWN;
		case VK_VOLUME_UP:
			return KEY_VOLUMEUP;
		case VK_MEDIA_NEXT_TRACK:
			return KEY_MEDIANEXT;
		case VK_MEDIA_PREV_TRACK:
			return KEY_MEDIAPREVIOUS;
		case VK_MEDIA_STOP:
			return KEY_MEDIASTOP;
		case VK_MEDIA_PLAY_PAUSE:
			return KEY_MEDIAPLAY;
		case VK_LAUNCH_MAIL:
			return KEY_LAUNCHMAIL;
		case VK_LAUNCH_MEDIA_SELECT:
			return KEY_LAUNCHMEDIA;
		case VK_LAUNCH_APP1:
			return KEY_LAUNCH0;
		case VK_LAUNCH_APP2:
			return KEY_LAUNCH1;
		case VK_SLEEP:
			return KEY_STANDBY;
		case VK_HELP:
			return KEY_HELP;
		case VK_OEM_1:
			return KEY_SEMICOLON;
		case VK_OEM_PLUS:
			return KEY_EQUAL;
		case VK_OEM_COMMA:
			return KEY_COMMA;
		case VK_OEM_MINUS:
			return KEY_MINUS;
		case VK_OEM_PERIOD:
			return KEY_PERIOD;
		case VK_OEM_2:
			return KEY_SLASH;
		case VK_OEM_3:
			return KEY_QUOTELEFT;
		case VK_OEM_4:
			return KEY_BRACKETLEFT;
		case VK_OEM_5:
			return KEY_BACKSLASH;
		case VK_OEM_6:
			return KEY_BRACKETRIGHT;
		case VK_OEM_7:
			return KEY_APOSTROPHE;
		default:
			return KEY_UNKNOWN;
	}
}

void TimestampInput::sync_clock_offset() {
	Time *time_singleton = Time::get_singleton();
	if (time_singleton == nullptr) {
		godot_clock_offset_usec_.store(0, std::memory_order_release);
		return;
	}

	const int64_t godot_usec = time_singleton->get_ticks_usec();
	const int64_t native_usec = static_cast<int64_t>(get_native_time_usec());
	godot_clock_offset_usec_.store(godot_usec - native_usec, std::memory_order_release);
}

uint64_t TimestampInput::get_native_time_usec() const {
	LARGE_INTEGER counter {};
	QueryPerformanceCounter(&counter);
	return qpc_to_usec(static_cast<uint64_t>(counter.QuadPart));
}

uint64_t TimestampInput::align_to_godot_usec(uint64_t p_native_usec) const {
	const int64_t offset_usec = godot_clock_offset_usec_.load(std::memory_order_acquire);
	const int64_t native_usec = static_cast<int64_t>(p_native_usec);
	const int64_t aligned_usec = native_usec + offset_usec;

	if (aligned_usec <= 0) {
		return 0;
	}

	return static_cast<uint64_t>(aligned_usec);
}

uint64_t TimestampInput::qpc_to_usec(uint64_t p_counter) const {
	if (qpc_frequency_.QuadPart <= 0) {
		return 0;
	}

	const uint64_t frequency = static_cast<uint64_t>(qpc_frequency_.QuadPart);
	const uint64_t whole_seconds = p_counter / frequency;
	const uint64_t remainder = p_counter % frequency;

	return (whole_seconds * 1000000ULL) + ((remainder * 1000000ULL) / frequency);
}
