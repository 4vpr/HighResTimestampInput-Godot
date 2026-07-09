# HighResTimestampInput

A Windows-only Godot extension made because Godot’s input latency felt a bit lacking.

This is mostly intended for rhythm games. Its main purpose is to provide highly accurate timestamps for judgment timing.

It is **not** intended to replace Godot’s built-in Input system.

Using Window's LawInput

## Supported Version

Godot 4.6+

## Features

Collects key down / key up events based on Windows Raw Input.

```gdscript
TimestampInput.start()
# Starts collecting Windows RawInputEvents.
# Returns false if something goes wrong.

TimestampInput.stop()
# Stops collecting events.

TimestampInput.poll_events()
# Retrieves the collected RawInputEvents.
# Returns Array[RawInputEvent].

TimestampInput.get_time_usec()
# Same as Time.get_ticks_usec().
```

## Installation

Add the following files to your project:

```text
high_res_timestamp_input.gdextension
bin/high_res_timestamp_input.windows.template_debug.x86_64.dll
bin/high_res_timestamp_input.windows.template_release.x86_64.dll
```

## Usage Example

```gdscript
extends Node

var start_usec := 0

func _ready() -> void:
    TimestampInput.start()

    # Start playing the song and handle sync logic here.
    start_usec = TimestampInput.get_time_usec()
    # Store the start time so you can subtract it from RawInputEvent.timestamp_usec
    # to determine where in the song the key was pressed.
    # Ideally, the song playback position should be as close to 0 as possible
    # when this line runs. If needed, add audio-related correction values
    # to start_usec.
    

func _process(_delta: float) -> void:
    for event: RawInputEvent in TimestampInput.poll_events():
        print(event.keycode, event.pressed, start_usec - event.timestamp_usec)
    # Maybe is_down would have been a better name than pressed.
```
