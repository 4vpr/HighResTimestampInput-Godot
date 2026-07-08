# HighResTimestampInput

Godot 인풋렉이 아쉬워서 만든 윈도우용 익스텐션

리듬게임 전용에 가깝습니다!

리듬게임에서 판정용 timestamp를 많이 정확하게 뽑는 용도!

기존 고도 Input 시스템을 대체하는 건 아닙니다

## 지원 버전

- Godot 4.6+

## 기능

- Raw Input 기반 key down / key up 수집
- `TimestampInput.start()` # 윈도우 RawInputEvent를 수집하기 시작함 먼가 문제생기면 false 반환
- `TimestampInput.stop()` # 수집을 종료함
- `TimestampInput.poll_events()` # 수집된 RawInputEvent들 꺼냄 -> Array[RawInputEvent]
- `TimestampInput.get_time_usec()` # Time.get_ticks_usec() 이랑 동일

## 설치

아래 파일 프로젝트에 삽입

- `high_res_timestamp_input.gdextension`
- `bin/high_res_timestamp_input.windows.template_debug.x86_64.dll`
- `bin/high_res_timestamp_input.windows.template_release.x86_64.dll`

## 사용 예시

```python
extends Node

var start_usec := 0

func _ready() -> void:
    TimestampInput.start()

    # (대충 노래재생 하고 싱크맞추는 로직 들어가는 자리)
    start_usec = TimestampInput.get_time_usec()
    # 키가 노래의 어느시점에서 눌렸는지 위해서 시작시간 저장 RawInputEvent.timestamp_usec 에서 빼면 됨
    # 윗줄의 코드가 실행될때 노래재생구간이 최대한 0에 가까워야하고 필요할시 start_usec에 오디오 관련 보정값을 더해줘야함
    

func _process(_delta: float) -> void:
    for event : RawInputEvent in TimestampInput.poll_events():
        print(event.keycode, event.pressed, start_usec - event.timestamp_usec)
    #pressed 말고 is_down이 더 적절했을지도 쩝
```
