# Native foundation 개발 검증

이 브랜치는 개발 기반이며 Fullbright 모드 릴리스가 아니다. 지원 RE8 빌드가 0개이므로 어떤 실행 파일에도 조명 훅을 설치하지 않는다. 실제 Deck에서의 로드·UI·프레임 성능은 미확인이다.

## 빌드와 검사

Ubuntu 24.04에서 `deps/toolchain-lock.txt`의 MinGW 패키지를 설치한 뒤 저장소 루트에서 실행한다.

```bash
python3 tools/fetch_deps.py
bash scripts/build.sh
FB_REQUIRE_BINARY=1 bash scripts/test.sh
```

Linux GCC만 있는 환경은 `bash scripts/test.sh`로 호스트 검사를 실행할 수 있다. 이때 DLL이 없으면 바이너리 검사는 명시적으로 skip된다. Lua UI 검사는 해시 검증한 `fetch_deps.py` 실행이 먼저 필요하다. CI는 바이너리 검사를 필수로 실행한다.

빌드 결과는 `build/RE8_FullBright.dll`, `build/manifest.json`이며 CI artifact에도 포함된다. manifest에는 DLL·소스 해시, 커밋, dirty 상태, 컴파일러가 기록된다. CI Windows 툴체인 패키지는 첫 성공 빌드에서 관측한 버전으로 고정했다. 실행 runner/호스트 도구가 완전히 고정된 비트 단위 재현 빌드라는 주장은 하지 않는다.

## 구현된 동작

- 시작 시 요청 OFF. 프로필 없는 foundation은 `UNSUPPORTED_BUILD`.
- 메뉴 연결은 내장 Lua UI 바인딩만 사용한다. 별도 autorun이나 Lua 조명 탐색은 없다.
- 설정: `reframework/data/RE8_FullBright/config.ini`. schema_version=1, toggle_vk=0 또는 F1–F12(112–123), detailed_log=0/1. ON 요청은 저장하지 않는다.
- 단축키 기본값은 None. 추후 지원 프로필에서 Steam Input 후면 버튼을 선택한 F키에 연결한다. 키 반복과 포커스 복귀 중 누른 키는 중복 토글하지 않는다.
- 설정 쓰기는 worker에서 sibling 임시 파일을 배타적으로 만들고 원자적으로 교체한다. 실패/잘못된 설정은 메뉴에서 표시한다. 실패한 저장은 설정을 다시 변경하면 재시도한다.
- 로그 큐는 256개, writer는 파일 2개×4 MiB로 제한한다. 파일은 매 게임 시작 시 새로 시작한다. 큐 포화·경합은 생략 계수, 파일 실패는 I/O 계수로 표시한다. 실패는 게임 상태와 분리한다.
- DLL/콜백/worker는 게임 프로세스 수명 동안 유지한다. 실행 중 DLL unload/hot replacement는 지원하지 않는다.
- UI detach는 Lua 상태를 접근하지 않고 세대 토큰을 무효화한다. REFramework에 개별 draw callback 삭제 API가 없어 같은 상태에 인위적으로 detach/attach하면 이전 closure는 inert 상태로 남고 Lua state 파괴 시 해제된다.

## UI ABI 근거와 한계

고정 REFramework 커밋 `d052e8fd...`의 `dependencies/lua/src/` 59개 C/header를 공식 Lua 5.4.3과 Git blob hash로 대조했다. 모든 헤더와 57개 파일이 일치하며 `ldebug.c`와 `lvm.c`만 오류/문자열 처리 수정이 있다(구조 배치 변경 없음). `cmake.toml`은 이 Lua를 static library로 빌드한다. combo의 선택값은 Lua table key이므로 None=1, F12=13으로 연결한다.

이 확인은 해당 소스 버전의 ABI 근거다. 사용자 설치 REFramework 바이너리와의 호환성은 아직 확인하지 않았으며 이 artifact를 검증된 설치 ZIP으로 배포하지 않는다.

## 로그 요약

```bash
python3 tools/summarize_run.py tests/fixtures/run.jsonl
python3 tools/summarize_run.py /path/to/play_0.jsonl /path/to/play_1.jsonl
```

요약은 상태·이유·생략 계수를 제공하며 시각적 성공을 판정하지 않는다. 실제 화면/성능 관측은 후속 실기 절차에서 진행한다.
