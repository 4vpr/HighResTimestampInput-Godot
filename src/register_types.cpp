#include "register_types.h"

#include "high_res_timestamp_input/raw_input_event.h"
#include "high_res_timestamp_input/timestamp_input.h"

#include <godot_cpp/classes/engine.hpp>

using namespace godot;

static TimestampInput *timestamp_input_singleton = nullptr;

void initialize_high_res_timestamp_input_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	ClassDB::register_class<TimestampInput>();
	ClassDB::register_class<RawInputEvent>();

	timestamp_input_singleton = memnew(TimestampInput);
	Engine::get_singleton()->register_singleton("TimestampInput", timestamp_input_singleton);
}

void uninitialize_high_res_timestamp_input_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	if (timestamp_input_singleton != nullptr) {
		Engine::get_singleton()->unregister_singleton("TimestampInput");
		memdelete(timestamp_input_singleton);
		timestamp_input_singleton = nullptr;
	}
}
