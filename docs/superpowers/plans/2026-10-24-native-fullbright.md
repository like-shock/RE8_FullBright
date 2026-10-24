# RE8 Native Fullbright Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Steam Deck/Proton의 RE8에서 장면별 광량 조절 없이 사용할 수 있는 Native Fullbright를 만들고, 한 장면의 효과부터 검증해 지원 범위를 넓힌다.

**Architecture:** REFramework가 Windows x64 DLL을 로드하고, 검증한 빌드 프로필에 따라 필요한 명령 위치만 후킹한다. 대상 소유 관계와 수명을 확인한 뒤 레지스터 결과를 치환하며, OFF·무효 대상에서는 원래 명령을 실행한다. 조명 제어는 Native에서 수행하고 메뉴 연결에만 최소 Lua 바인딩을 사용할 수 있다.

**Tech Stack:** C11, 필요한 최소 x86-64 어셈블리, MinGW-w64 POSIX GCC, Linux x86_64 호스트 검사, Python 3 표준 라이브러리 분석·패키징, Bash, REFramework C API, MinHook. UI 연결용 Lua core는 필요 시 고정 버전 사용.

**Spec:** [승인된 설계](../specs/2026-10-24-native-fullbright-design.md). 설계와 이 계획을 함께 읽는다.

**Status:** 2026-10-24 작성·자체 검토 완료. 설계 승인됨. 구현 작업은 모두 미착수이며 실행 방식 선택을 기다린다.

## Global Constraints

- 1차 실행·검증 환경은 Steam Deck/Proton이다.
- 첫 지원 범위는 Steam판 RE8의 **한 실행 파일 빌드**, 본편 1인칭, 게임 내 RT OFF다.
- 배포 단위는 `RE8_FullBright.dll`인 Windows x64 REFramework Native 플러그인으로 한다.
- **C11 + 필요한 최소 x86-64 어셈블리**, Linux CI에서 Windows x64로 교차 빌드한다.
- 로드 직후 기능은 OFF로 시작한다. 재실행 자동 ON은 초기 범위에 넣지 않는다.
- 초기에는 검증된 기본 프로필 하나를 제공한다. Lua 조명 시제품·카메라 부착 추가 광원·ReShade 우회는 만들지 않는다.
- 최소 상태는 `OFF`, `UNSUPPORTED_BUILD`, `WAIT_TARGET`, `ACTIVE`, `FAULT`다. `ACTIVE`는 실제 훅 적용 관측을 뜻하며 화면 품질 인증이 아니다.
- 미지원 빌드/패턴 불일치에서는 조명 훅을 설치하지 않는다. 핵심 훅 묶음 검증 실패 시 일부만 활성화하지 않는다.
- 기본 로그는 두 파일을 순환하며 합계 8 MiB 이내로 제한한다. 훅 안에서 파일 I/O·광범위한 SDK 탐색을 수행하지 않는다.
- 성능은 같은 조건에서 OFF/ON/OFF를 각 60초 이상 비교한다. 초기 검토 기준은 중앙값과 p95 프레임 시간 증가가 각각 10% 이내다. 측정 전 보장하지 않는다.
- 게임 실행 파일·원본 CT·세이브·덤프는 공개 Git에 넣지 않는다. 빌드·모의 검사 통과를 실기 성공으로 발표하지 않는다.

## Review Focus

1. 정상 빌드 해시 뒤에 로드 메모리나 훅 대상이 바뀌는 경우: 쓰기 직전 재검증과 훅 묶음의 안전한 철회/통과 필요. Task 3·6에서 검사한다.
2. OFF 요청·지역 교체가 다른 렌더 스레드의 처리와 겹치는 경우: 이전 세대에 대한 새 보정 금지와 해제 중 코드 수명 보장 필요. Task 3·6·8에서 검사한다.
3. REFramework 메뉴 재로드·키 자동 반복·포커스 복귀: 중복 토글, 중복 콜백, 이전 Lua 상태 참조 금지. Task 4에서 검사한다.
4. 로그·설정 저장 공간 부족/손상: 게임은 계속 동작하고 원인과 저장 실패를 표시해야 함. Task 4에서 검사한다.
5. 밝은 영역·자동 노출·프레임 제한 때문에 성공처럼 보이는 경우: 동일 조건 비교와 미확인 판정, 설치 후 실제 효과 재확인 필요. Task 7·8·9에서 검사한다.

## 진행 순서와 자료 의존성

| 작업 | 자료 없이 가능한 범위 | 자료가 있어야 완료되는 범위 |
| --- | --- | --- |
| 1–4 | 환경 수집 도구, Native 기반, 검증/상태 로직, 설정·진단 | 대상 환경 기록은 Deck 실행이 필요 |
| 5 | RE7 최소 의존성·재사용 코드 검토 | RE8 대응 명령·객체·수명 근거 |
| 6 | 검증된 공통 ABI 브리지의 재사용 검토 | RE8 후보에 연결한 실제 명령 재현 |
| 7–8 | 테스트 절차·패키지·분석 도구 | 실제 밝기 효과·전환 안정성·성능 |
| 9 | 설치·제거 검사 | 지원 범위 확정과 최종 Deck 설치 확인 |

의존 순서는 `1 → 2 → 3 → 4 → 5 → 6 → 7 → 8 → 9`다. Task 5의 RE7 정적 분석은 자료를 기다리는 동안 앞당길 수 있다. 광범위한 기능 추가로 RE8 매핑 실패를 덮지 않는다.

Task 1–4가 끝나도 결과는 **Native 기반**이다. Task 5의 근거가 없으면 가짜 프로필을 만들거나 Fullbright 바이너리를 완성했다고 표시하지 않는다. 정적 분석만으로 충분한 경우 별도 Probe 배포를 생략하고 Task 7 시험 DLL에 진단을 합친다.

## 파일 구조와 책임

현재 저장소에는 README와 설계서·이 계획만 있다. 아래는 앞으로 만들 파일이며 현재 존재한다고 가정하지 않는다.

| 경로 | 책임 / 최초 작업 |
| --- | --- |
| `tools/collect_environment.sh`, `docs/testing/ENVIRONMENT_KO.md` | Deck에서 실행 파일 해시·버전/설정 기록 지원 / 1 |
| `tools/fetch_deps.py`, `deps/lock.json`, `scripts/build.sh`, `scripts/test.sh`, `.github/workflows/verify.yml` | 고정 의존성·빌드·검사 / 2 |
| `src/plugin.c`, `src/build_identity.c`, `src/build_identity.h` | REFramework 진입과 PE/해시 식별 / 2 |
| `src/profile.c`, `src/profile.h`, `profiles/index.json` | 지원 프로필 검증과 등록 / 3 |
| `src/runtime.c`, `src/runtime.h` | 사용자 요청·대상 세대·적용 상태 / 3 |
| `src/options.c`, `src/options.h`, `src/options_ui.c` | 설정과 최소 메뉴 바인딩 / 4 |
| `src/journal.c`, `src/journal.h`, `tools/summarize_run.py` | 제한된 이벤트 기록·요약 / 4 |
| `docs/research/re7-fullbright-map.md`, `docs/research/re8-mapping.md`, `tools/verify_profile.py` | 이식 근거·프로필 오프라인 검증 / 5 |
| `src/hooks.c`, `src/hooks.h`, `src/bridge.c`, `src/bridge.h`, `src/bridge.S` | 설치·재배치·CPU 상태·원래 명령 재현 / 6 |
| `src/fullbright.c`, `src/fullbright.h`, `profiles/<id>.json` | 검증된 대상에 조명 계산 적용 / 7 |
| `tools/package.py`, `docs/testing/FIRST_SCENE_KO.md` | 첫 실험 패키지·비교 절차 / 7 |
| `docs/validation/matrix.md`, `tools/summarize_validation.py` | 상태 전환·장면·성능 결과 / 8 |
| `packaging/install.sh`, `packaging/uninstall.sh`, `docs/INSTALL_KO.md`, `docs/release-notes/<version>.md` | 배포와 복구 / 9 |

`<id>`는 실제 EXE SHA-256 전체 64자리, `<version>`은 해당 릴리스의 명시적 버전이다. 값을 추측해 채우지 않는다. `tests/`에는 각 작업에서 명시한 검사만 추가하며, 원본 게임 자료 없이 실행되는 합성 fixture와 비공개 실제 명령 대조를 구분한다. `build/`, `dist/`, `private/`, 런타임 로그는 추적에서 제외한다.

## 공통 계약과 재사용 기준

- 모든 C 공개 함수는 `fb_` 접두사를 쓴다. `src/common.h`(Task 2)는 `FbError {code, message}`와 고정 폭 정수 등 공통 정의를 제공한다. 실패는 반환값+오류 정보로 알리며 `assert`로 게임을 종료하지 않는다.
- Task 2의 `FbBuildIdentity`에는 파일 SHA-256 32바이트, PE machine, timestamp, SizeOfImage가 있다. ASLR 기준주소는 여기에 저장하지 않는다.
- Task 3의 `FbProfile`은 ID/단계(candidate 또는 validated), 빌드 식별, 필수 site 목록, 원래 명령/마스크, 섹션·대상 검증 요구사항을 가진 불변 데이터다. `FbImageView`는 현재 모듈 범위와 읽기 접근을, `FbVerifiedPlan`은 검증된 RVA·원본 바이트·프로필 식별을 보관한다. 오프라인 검증은 실제 메모리 적용 권한이 아니다.
- `FbObservation`은 대상 유효성·소유자 식별·세대 번호를 가진다. `FbRuntime`은 요청과 관측 상태를 관리하며, 실제 훅이 `fb_runtime_note_applied`를 호출하기 전에는 ACTIVE가 되지 않는다. 게임의 세대 판별 방식은 Task 5 근거로 확정한다.
- 모든 runtime 호출은 공통 `fb_control_lock(void)` / `fb_control_unlock(void)` 안에서 수행한다. 이 경계는 `runtime.h`에 선언하고 `runtime.c`에서 Windows SRWLOCK(호스트 검사에서는 pthread mutex)으로 구현한다. UI/worker/훅은 같은 잠금을 쓰며 파일 I/O·SDK 호출·트램펄린 실행 중에는 유지하지 않는다. OFF의 선형화 이후 새 보정을 금지하되 이미 확정되어 반환 중인 프레임까지 소급 취소한다고 약속하지 않는다.
- `FbCpuContext`의 레지스터/확장 상태 배치와 트램펄린 경계는 Task 6에서 어셈블리와 함께 정의한다. C 헤더와 어셈블리 오프셋은 같은 정의에서 생성·검사한다.
- RE7 재사용 기준 커밋: `ffde226b12c76e3dfec806e8e0a23de267e2b6a5`. 카메라·모델·원본 CT 전체를 복제하지 않는다. 이식한 독립 코드와 동작 참조의 출처·고지를 구별한다.
- REFramework 헤더 기준 커밋: `d052e8fd3fd41d0a1a33394fcf6e6c19be39b1ee`의 `include/reframework/API.h`(API 1.15.0). 설치된 런타임과의 실제 호환성은 Task 2에서 확인한다. 자체 축약 ABI 선언 대신 고정 공식 헤더를 사용한다.
- MinHook: `c3fcafdc10146beb5919319d0683e44e3c30d537`(v1.3.4). RE7 `diagnostics/data/minhook.json`의 파일별 SHA-256을 교차 확인해 lock에 기록한다.
- 메뉴 바인딩의 Lua core: 5.4.3, 공식 archive SHA-256 `f8612276169e3bfcbcfb8f226195bfc6e466fe13042f1076cbde92b7ec96bbfb`. 현재 REFramework Lua ABI를 확인한 경우에만 같은 상태에 연결한다. 불일치하면 지원 런타임을 먼저 조정하며 강행하지 않는다.
- CI 기본은 Ubuntu 24.04 x86_64, `x86_64-w64-mingw32-gcc-posix`, 호스트 GCC, Python 3다. 실제 패키지 버전·컴파일러 출력·소스/의존성 해시를 Task 2의 `deps/toolchain-lock.txt`와 빌드 manifest에 고정한다. 재현 빌드는 lock에 맞춰야 하며 "latest"로 바꾸지 않는다.
- 아래 검사 코드 블록은 각 fixture 실행 후의 핵심 단언이다. 전체 fixture와 assertion 변수의 준비는 해당 `tests/` 파일에 구현하며, 향후 명령을 이번에 실행한 결과로 취급하지 않는다.

## Task 1: 대상 환경을 재현할 수 있는 수집 도구

**Files:** Create `tools/collect_environment.sh`, `docs/testing/ENVIRONMENT_KO.md`, `.gitignore`; Test `tests/test_collect_environment.py`.

**Interfaces:** `bash tools/collect_environment.sh GAME_DIR OUTPUT_DIR` → `environment.txt`와 선택 설정 입력 안내, 성공 0/필수 게임 파일 없음 2. 결과에는 re8.exe SHA-256, dinput8.dll 존재/해시, SteamOS 정보, 확인 가능한 Steam build ID만 쓴다. Proton/REFramework 버전·그래픽 조건은 확인값과 수동 기록값을 구분한다. PE 분석은 Task 2가 담당한다.

- [ ] **1. 실패 검사 작성:** `test_collect_environment.py`에서 공백 경로의 가짜 re8.exe가 정확한 해시로 기록됨, 없으면 exit=2, 기존 출력이 있으면 덮어쓰기 거부, 게임 디렉터리 파일 해시가 전후 동일함을 검증한다.

```python
# test_collect_space_path_preserves_game
assert result.returncode == 0
assert recorded_exe_sha256 == expected_sha256
assert game_hashes_after == game_hashes_before
```

- [ ] **2. 실패 확인:** `python3 -m unittest discover -s tests -p test_collect_environment.py -v` → script 미구현으로 실패. shell·coreutils 기반이며 Deck에 Python 설치를 요구하지 않는다.
- [ ] **3. 최소 구현:** 스크립트와 기록 안내를 작성한다. 확인할 수 없는 값을 추정해 채우지 않고 `unconfirmed`로 기록한다. 파일 생성은 새 OUTPUT_DIR에 한정한다.
- [ ] **4. 통과 확인:** 같은 unittest 명령 → 모든 사례 통과. 실제 Deck 실행 전에는 환경 수집 완료로 표시하지 않는다.
- [ ] **5. 커밋:** `git add tools/collect_environment.sh docs/testing/ENVIRONMENT_KO.md .gitignore tests/test_collect_environment.py` 후 `git commit -m "feat: add read-only target environment collector"`.

## Task 2: 후킹 없는 Native 플러그인과 빌드 식별

**Files:** Create `src/common.h`, `src/plugin.c`, `src/build_identity.{c,h}`, `scripts/{build,test}.sh`, `tools/fetch_deps.py`, `deps/{lock.json,toolchain-lock.txt}`, `.github/workflows/verify.yml`, `NOTICE.md`; Test `tests/build_identity_host.c`, `tests/test_build_identity.py`, `tests/test_binary.py`, `tests/test_deps.py`.

**Interfaces:** `bool fb_identify_file(const wchar_t *path, FbBuildIdentity *out, FbError *error)`; `bool fb_parse_pe(const uint8_t *data, size_t length, FbBuildIdentity *out, FbError *error)`(PE 필드만 작성; SHA는 별도 전체 파일 검증). 플러그인은 공식 `reframework_plugin_required_version`/`reframework_plugin_initialize`를 내보내고 최초 상태 OFF를 기록한다. 무거운 해시 작업은 로더 잠금/DllMain 밖에서 수행한다.

- [ ] **1. 실패 검사 작성:** PE32+ AMD64 합성 fixture, 잘린 헤더, 범위를 넘는 섹션, 다른 machine 거부 및 이미 알려진 SHA-256 test vector를 검증한다. 파일이 해시 도중 변경되면 결과를 거부한다. 의존성 파일 1바이트 변경 시 빌드가 중단되는 fixture도 추가한다.

```c
/* test_reject_empty_pe; data는 1바이트 fixture이고 유효 길이는 0 */
assert(!fb_parse_pe(data, 0, &identity, &error));
assert(error.code != 0);
```

- [ ] **2. 실패 확인:** `python3 -m unittest discover -s tests -p 'test_*identity.py' -v` 및 `python3 -m unittest discover -s tests -p test_deps.py -v` → 미구현 실패.
- [ ] **3. 최소 구현:** 고정 헤더·의존성·도구 버전을 검증해 캐시에 받는다. C11 DLL과 호스트 검사를 빌드하고 버전/해시 정보를 기록한다. 이 단계의 DLL에는 게임 detour나 광원 쓰기를 넣지 않는다.
- [ ] **4. 통과 확인:** `bash scripts/test.sh --suite identity`와 `bash scripts/build.sh` → 성공; `python3 -m unittest discover -s tests -p test_binary.py -v` → PE AMD64, 필수 export, 허용 import, 소스 manifest 검사 통과. `scripts/test.sh`는 이후 suite를 명시적으로 지원하며 없는 suite는 오류로 끝낸다.
- [ ] **5. 커밋:** 위 신규 파일을 지정해 stage한 뒤 `git commit -m "feat: add passive native plugin and build identity checks"`. Deck에서의 로드 확인은 사용자 실행 결과로 따로 기록한다.

## Task 3: 빌드 프로필 검증과 OFF 우선 상태기계

**Files:** Create `src/profile.{c,h}`, `src/runtime.{c,h}`, `profiles/index.json`; Modify `src/plugin.c`, `scripts/test.sh`; Test `tests/profile_host.c`, `tests/runtime_host.c`, `tests/test_profile.py`, `tests/test_runtime.py`.

**Interfaces:** `bool fb_verify_profile(const FbImageView*, const FbBuildIdentity*, const FbProfile*, FbVerifiedPlan*, FbError*)`; `void fb_runtime_init(FbRuntime*, bool supported)`; `void fb_runtime_request(FbRuntime*, bool on)`; `void fb_runtime_observe(FbRuntime*, const FbObservation*)`; `bool fb_runtime_note_applied(FbRuntime*, uint64_t generation)`; `void fb_runtime_fault(FbRuntime*, const FbError*)`; `FbStatus fb_runtime_status(const FbRuntime*)`. FbStatus의 C enum 이름은 `FB_OFF`, `FB_UNSUPPORTED_BUILD`, `FB_WAIT_TARGET`, `FB_ACTIVE`, `FB_FAULT`다. 공통 control lock을 이 작업에서 구현하고 Task 4·6 연결부가 같은 경계를 사용한다.

- [ ] **1. 실패 검사 작성:** `test_profile.py`는 잘못된 해시·0/복수 패턴 매치·비실행 섹션·원본 바이트 불일치를 거부한다. `test_runtime.py`는 초기 OFF, ON+대상 없음 WAIT_TARGET, 현재 세대 적용 관측 후 ACTIVE, OFF 뒤 이전 세대 note 거부, 대상 교체 후 WAIT_TARGET, 미지원에서는 항상 UNSUPPORTED_BUILD를 검증한다.

```c
/* test_supported_runtime_starts_off; 검사도 동일 control lock을 사용 */
fb_runtime_init(&runtime, true);
assert(fb_runtime_status(&runtime) == FB_OFF);
fb_runtime_request(&runtime, true);
assert(fb_runtime_status(&runtime) == FB_WAIT_TARGET);
```

- [ ] **2. 실패 확인:** `python3 -m unittest discover -s tests -p test_profile.py -v`와 `python3 -m unittest discover -s tests -p test_runtime.py -v` → 미구현 실패.
- [ ] **3. 최소 구현:** 지원 registry는 빈 상태로 시작한다. 합성 테스트 프로필은 tests에만 둔다. 오류 원인을 보존하고, fault 복구는 게임 재시작을 기본으로 한다. 파일 identity가 맞아도 live image 확인을 생략하지 않는다.
- [ ] **4. 통과 확인:** `bash scripts/test.sh --suite profile`와 `bash scripts/test.sh --suite runtime` → 모든 거부/복귀 조건 통과. 현재 지원 RE8 빌드 수는 0으로 표시한다.
- [ ] **5. 커밋:** 위 파일을 stage 후 `git commit -m "feat: gate fullbright on verified profiles and target state"`.

## Task 4: 설정·메뉴·진단을 Native 상태와 연결

**Files:** Create `src/options.{c,h}`, `src/options_ui.c`, `src/journal.{c,h}`, `tools/summarize_run.py`; Modify `src/plugin.c`, `scripts/test.sh`, `deps/lock.json`; Test `tests/test_options.py`, `tests/test_journal.py`, `tests/test_ui_lifecycle.py`, `tests/ui_harness.lua`.

**Interfaces:** `FbConfig {schema_version=1, toggle_vk=0, detailed_log=false}`; `bool fb_config_load(const wchar_t*, FbConfig*, FbError*)`, `bool fb_config_save(const wchar_t*, const FbConfig*, FbError*)`; `bool fb_ui_attach(struct lua_State*, FbRuntime*)`, `void fb_ui_detach(struct lua_State*)`; `bool fb_journal_start(const wchar_t*, FbError*)`, `bool fb_journal_enqueue(const FbEvent*)`, `void fb_journal_drain(void)`, `void fb_journal_stop(void)`. `FbEvent`는 seq/time/status/generation/site/reason과 계수이며 전체 메모리 덤프를 받지 않는다.

- [ ] **1. 실패 검사 작성:** config의 손상/알 수 없는 버전은 기본값+오류 표시, 재실행은 저장된 값과 무관하게 OFF. 누른 채 반복 입력은 한 번만 토글, 포커스 복귀 뒤 새 press 필요. Lua attach/detach 반복에서 콜백/참조가 남지 않음, SDK가 없는 메뉴 fixture에서도 광원 탐색을 호출하지 않음을 검증한다. 로그 검사는 4 MiB 파일 두 개, 합계 8 MiB 이하, 큐 포화·disk-full·stop 후 enqueue에서 게임 상태 불변과 해제된 메모리 미접근을 검증한다.

```python
# test_log_full_does_not_change_runtime
assert sum(log_file_sizes) <= 8 * 1024 * 1024
assert len(log_file_sizes) <= 2
assert runtime_after == runtime_before
assert dropped_events > 0
```

- [ ] **2. 실패 확인:** `python3 -m unittest discover -s tests -p test_options.py -v`, `python3 -m unittest discover -s tests -p test_ui_lifecycle.py -v`, `python3 -m unittest discover -s tests -p test_journal.py -v` → 미구현 실패.
- [ ] **3. 최소 구현:** `reframework/data/RE8_FullBright/config.ini`에 flat key/value 설정만 저장한다. 메뉴는 ON/OFF·상태·단축키·진단 설정을 제공한다. 요청 ON은 저장하지 않는다. 단축키는 기본 미지정이며 Steam Input 안내를 제공한다. 별도 autorun 파일과 Lua 광원 제어를 만들지 않는다.
- [ ] **4. 통과 확인:** `bash scripts/test.sh --suite options` / `--suite journal` / `--suite ui` → 성공. 파일 출력은 훅 밖에서 실행하고, 로그 생략 계수·설정 저장 실패를 표시한다. `python3 tools/summarize_run.py tests/fixtures/run.jsonl`은 동일 입력에 동일 요약을 낸다(fixture는 이 작업에서 생성).
- [ ] **5. 커밋:** 위 변경을 stage 후 `git commit -m "feat: add fullbright controls and bounded diagnostics"`.

## Task 5: RE7 최소 의존성과 RE8 대응 지점 확정 — 자료 의존 관문

**Files:** Create `docs/research/re7-fullbright-map.md`, `docs/research/re8-mapping.md`, `tools/verify_profile.py`, `tests/test_profile_artifact.py`; Create 비공개 `private/<sha>/`(커밋 금지). 프로필 후보 `profiles/<sha>.json`은 검증 자료 확보 후에만 작성한다.

**Interfaces:** `python3 tools/verify_profile.py --exe PRIVATE_EXE --profile profiles/<sha>.json` → file identity·site별 섹션/매치/명령 길이 보고서, 일치 0/불일치 2. 이는 오프라인 판정이며 런타임 소유자·ABI·효과가 입증되었다는 의미가 아니다. 다음 작업은 이 보고서와 site별 계약을 소비한다.

- [ ] **1. RE7 근거 작성:** 고정 커밋의 Fullbright 7개 명령 위치와 OLD_HOUSE/준비 조건의 의존성을 나열한다. 재사용할 독립 훅 코드·테스트·출처를 별도 표로 분리하고, RE8 이식 대상이 아닌 카메라/모델/지역 상수를 표시한다.
- [ ] **2. 입력 확인:** Task 1의 환경 기록과 실제 분석 가능한 EXE 또는 충분한 디스어셈블리·런타임 자료를 연결한다. 자료가 없으면 RE8 매핑은 `WAITING_FOR_TARGET_EVIDENCE`로 기록하고 이 관문에서 멈춘다. RE7 분석은 계속할 수 있다.
- [ ] **3. 후보 분석:** 범위/색 계산, 광원 활성 경로, 밝기 계산의 실제 소비 위치를 찾는다. 각 site에 RVA, 원래 명령, 살아 있는 레지스터, flags/stack 부작용, 호출 조건, 객체 소유 관계, 재배치 요구, 의미를 지지하는 근거를 기록한다. 같은 AOB만으로 대응 판정을 내리지 않는다.
- [ ] **4. 검증기 TDD:** 합성 PE+profile 쌍에서 올바른 매핑 exit=0, 잘못된 해시/겹치는 site/복수 매치/범위 초과 exit=2인 `test_profile_artifact.py`를 먼저 작성·실패 확인한 뒤 `verify_profile.py`를 구현한다. `python3 -m unittest discover -s tests -p test_profile_artifact.py -v`가 통과해야 한다.

```python
# test_ambiguous_site_is_rejected
assert verify_result.returncode == 2
assert ambiguous_site_id in report["rejected_sites"]
```

- [ ] **5. 관문 판정:** 필요한 모든 site의 명령 계약이 있으면 `STATIC_MAPPED`로 기록한다. 동적 소유자 확인이 부족하면 가설·한 번의 재현 절차·수집 항목·종료 조건을 정하고 Task 6의 안전한 pass-through 계측으로 연결한다. 독립 Probe 배포를 자동 선행 조건으로 만들지 않는다.
- [ ] **6. 커밋:** 비공개 파일 제외를 확인하고 근거 문서/검증기/증거가 있는 candidate profile만 stage 후 `git commit -m "research: map RE8 fullbright candidates with explicit evidence"`. 근거 없이 주소·프로필 파일을 만들지 않는다.

## Task 6: 검증된 명령 단위 훅과 CPU 상태 보존

**Files:** Create `src/hooks.{c,h}`, `src/bridge.{c,h,S}`; Modify `src/plugin.c`, `src/profile.c`, `scripts/{test,build}.sh`; Test `tests/bridge_host.{c,S}`, `tests/hooks_host.c`, `tests/test_bridge.py`, `tests/test_hooks.py`, `tests/test_unwind.py`.

**Interfaces:** `bool fb_hooks_install(const FbVerifiedPlan*, FbError*)`; `void fb_hooks_request_passthrough(void)`; `FbHookAction fb_dispatch(unsigned site, FbCpuContext*)` with `FB_REPLAY_ORIGINAL` / `FB_HANDLED`. `FbCpuContext`와 호출 프레임은 Task 5의 site 계약에 따라 정의한다. 설치 검증·generation·in-flight 수명 처리는 hooks.c, 계산 정책은 Task 7에 둔다.

- [ ] **1. 실패 검사 작성:** 실제 x64 instruction fixture에서 원래 실행과 pass-through 결과의 GPR/flags/XMM·필요한 확장 상태·stack write 일치를 비교한다. 명령 재배치·분기·스택 정렬과 Windows unwind 메타데이터를 검사한다. 해당 CPU가 지원하지 않는 확장 상태 검사는 이유와 함께 명시적으로 건너뛰며 통과로 세지 않는다.
- [ ] **2. 설치 실패 검사:** 설치 중 N번째 site 실패, 설치 직전 원본 바이트 변경, 이미 다른 mod가 후킹한 site, in-flight 처리 중 OFF/대상 교체를 주입한다. 부분 조명 보정 0회, 새 보정 중지, 활성 코드/트램펄린 미해제, 오류 기록을 검증한다.

```c
/* test_install_failure_never_enables_policy */
assert(!fb_hooks_install(&verified_plan, &error));
assert(applied_count == 0);
assert(freed_inflight_trampolines == 0);
```

- [ ] **3. 실패 확인:** `python3 -m unittest discover -s tests -p test_bridge.py -v`와 `python3 -m unittest discover -s tests -p test_hooks.py -v` → 미구현 실패. 기존 RE7 검사 재사용 시 RE8 adapter가 검사에 실제 연결되었는지 확인한다.
- [ ] **4. 최소 구현:** 검토된 브리지를 필요한 부분만 이식한다. 모든 site를 생성·검증한 뒤 묶어서 활성화하고, 실패 시 원래 코드로 철회 가능한지 검증한다. 철회가 불확실하면 조명 정책을 영구 OFF로 두고 유효한 pass-through trampoline/DLL을 유지한다. 동기화는 명시적 control lock·generation 확인·in-flight 수명으로 구성하며 훅 안에서 I/O를 하지 않는다.
- [ ] **5. 통과 확인:** `bash scripts/test.sh --suite bridge` / `--suite hooks` / `--suite unwind`와 `bash scripts/build.sh` → 성공. 동적 근거가 필요한 경우에만 제한된 계측을 실행하고 결과를 Task 5 문서에 연결한다. 실기 예외 안전성까지 증명됐다고 주장하지 않는다.
- [ ] **6. 커밋:** 명시한 변경을 stage 후 `git commit -m "feat: add verified native hook bridge and passthrough lifecycle"`.

## Task 7: 한 장면 Fullbright와 첫 실험 패키지

**Files:** Create `src/fullbright.{c,h}`, `tests/fullbright_host.c`, `tests/test_fullbright.py`, `tools/package.py`, `tests/test_package.py`, `docs/testing/FIRST_SCENE_KO.md`; Modify `src/hooks.c`, `profiles/<sha>.json`, `profiles/index.json`, `docs/research/re8-mapping.md`, `README.md`.

**Interfaces:** `FbHookAction fb_fullbright_apply(FbRuntime*, unsigned site, FbCpuContext*)`는 Task 5의 실제 피연산자 계약만 소비한다. Task 6 dispatch가 이 함수를 호출한다. `python3 tools/package.py --stage experimental --output dist` → DLL, 동일 소스 커밋의 manifest, 지원 candidate 식별과 수동 설치/제거·시험 안내가 든 ZIP 및 SHA256SUMS.txt.

- [ ] **1. 실패 검사 작성:** `test_fullbright.py`에서 OFF/비대상/다른 세대/읽기 실패/부분 피연산자/NaN·Inf 입력은 원래 명령 통과, 유효 대상만 Task 5에 기록한 결과를 냄을 검증한다. 성공 전에 모든 피연산자를 확보하고 실패 시 일부 레지스터만 바뀌지 않아야 한다. expected 값은 명령 대조와 확인된 계약에서 구하며 구현 함수를 정답 생성기로 쓰지 않는다.

```c
/* test_off_replays_without_correction; runtime은 OFF fixture */
assert(fb_fullbright_apply(&runtime, site, &context) == FB_REPLAY_ORIGINAL);
assert(corrected_operand_count == 0);
```

- [ ] **2. 실패 확인:** `python3 -m unittest discover -s tests -p test_fullbright.py -v` → 미구현 실패. `test_package.py`는 DLL/manifest hash 불일치·잘못된 stage·개인 로그 포함을 거부하는 사례로 먼저 실패시킨다.
- [ ] **3. 최소 구현:** 한 대상 빌드의 최소 유효 훅 조합만 연결한다. candidate 프로필은 명령·대상 검증을 통과한 실험 빌드에서만 허용하고 validated 배포와 구분한다. 프로필과 실험 상수의 근거를 함께 기록하며, RE7의 1000/0.125/0.27 등을 검증 없이 이식하지 않는다. 장면 객체에 지속 쓰기가 새로 필요해지면 복원 계약을 먼저 설계에 반영한다.
- [ ] **4. 오프라인 통과 확인:** `bash scripts/test.sh --suite fullbright`, `python3 -m unittest discover -s tests -p test_package.py -v`, `bash scripts/build.sh` → 성공. 깨끗한 커밋을 만든 뒤 패키징한다.
- [ ] **5. 커밋·실험 패키지:** `git commit -m "feat: add first RE8 fullbright candidate"` 후 위 package 명령을 실행한다. ZIP 안내에는 버전/빌드, 기본 OFF, ON/OFF 절차, 로그 위치, 기대 결과와 미검증 상태를 명시한다. 이 단계는 정식 안정판 배포가 아니다.
- [ ] **6. 첫 실기 판정:** 같은 장면/방향에서 기본→ON→OFF를 비교하고 가까운 벽/물체와 먼 어둠을 함께 본다. 밝기 경계·회전·손전등 비의존성·OFF 복귀를 확인한다. `APPLIED_BUT_VISUAL_FAIL`, `WAIT_TARGET`, `FAULT`를 구분해 요약하며 사용자의 목표 부합 확인 전에는 Task 7 완료로 표시하지 않는다. 실패 시 같은 후보의 구체적 원인을 수정하거나 매핑을 재검토한다. 결과는 시험 바이너리의 소스 커밋을 인용해 문서 커밋으로 남긴다.

## Task 8: 대상 수명·대표 장면·성능 검증

**Files:** Modify `src/runtime.c`, `src/hooks.c`, `src/fullbright.c`, 기존 runtime/hooks/fullbright tests; Create `docs/validation/matrix.md`, `tools/summarize_validation.py`, `tests/test_validation_summary.py`.

**Interfaces:** `python3 tools/summarize_validation.py --record PRIVATE_RECORD_JSON --frames PRIVATE_FRAME_CSV` → 조건 일치 여부, OFF 전후 기준, ON 중앙값/p95 증가율, PASS/FAIL/INCONCLUSIVE. CSV는 segment(off_before/on/off_after), monotonic timestamp, frame_ms이며 실제 Present 간격 등 측정 출처와 단위를 record에 기록한다. 훅 호출 횟수를 프레임 수로 대체하지 않는다.

- [ ] **1. 실패 검사 작성:** 치료·조사·컷신·load·사망·지역 교체의 유효/무효 대상 sequence, 같은 주소의 새 세대 객체, OFF와 동시 호출을 기존 검사에 추가한다. 누락된 소유자 근거는 통과시킨다. 메뉴 Lua 재로드가 런타임을 ON으로 만들면 실패다.
- [ ] **2. 요약기 실패 검사:** 같은 조건의 60초 이상 세 구간에서 중앙값/p95 각각 증가율 ≤10%인 fixture는 PASS, 초과는 FAIL, 조건 불일치·표본/측정 출처 부족·열 상태 변화는 INCONCLUSIVE. OFF_before/after가 10% 넘게 달라지면 기준 불안정으로 INCONCLUSIVE. p95는 정렬 후 nearest-rank, 기준은 두 OFF 구간의 더 빠른(낮은) 해당 통계량으로 한다.

```python
# test_missing_segment_cannot_pass
assert summary["performance"] == "INCONCLUSIVE"
assert "missing_off_after" in summary["reasons"]
```

- [ ] **3. 실패 확인 및 최소 수정:** `python3 -m unittest discover -s tests -p test_validation_summary.py -v`의 실패를 확인한 후 요약기를 구현한다. 전환 수정은 추가한 재현 사례에 필요한 만큼만 적용하며 임의 유예 시간이나 매 프레임 광범위 탐색으로 덮지 않는다.
- [ ] **4. 오프라인 통과·커밋:** `bash scripts/test.sh` → 기존+신규 전체 성공 후 `git commit -m "fix: preserve fullbright target lifecycle and measure regression"`로 수정과 검사·절차를 함께 기록한다.
- [ ] **5. 대표 실기:** 실내/동굴·지하/밝은 야외에서 동일 기본 프로필, OFF/ON/OFF 각 60초 이상, 동일 해상도·TDP·프레임 제한·설정으로 비교한다. 자동 노출 안정 대기·캡처 위치를 고정한다. 제한에 비용이 가려지면 별도 비교를 남기고 결과를 꾸미지 않는다.
- [ ] **6. 범위 판정:** 전환/시각/성능을 각각 PASS/FAIL/UNTESTED로 기록한다. 열려 있는 항목은 지원 범위에서 구분한다. 기본값 하나로 만족하지 못하면 전체 완성이 아니며, 실패 장면을 숨기거나 특정 구간의 결과를 전체에 일반화하지 않는다. 실기 결과 문서를 별도 커밋해 시험한 소스/바이너리와의 연결을 보존한다.

## Task 9: 설치·제거·릴리스 검증

**Files:** Create `packaging/install.sh`, `packaging/uninstall.sh`, `docs/INSTALL_KO.md`, `docs/release-notes/<version>.md`, `tests/test_install.py`; Modify `tools/package.py`, `tests/test_package.py`, `.github/workflows/verify.yml`, `README.md`, `NOTICE.md`.

**Interfaces:** `bash install.sh GAME_DIR`와 `bash uninstall.sh GAME_DIR`; 현재 패키지가 소유하는 DLL·안내 파일만 변경한다. 기존 DLL은 원본 hash와 함께 백업하며 설정·로그·다른 mod·REFramework 본체는 보존한다. 실행 중 게임이 확인되거나 안전한 종료 여부를 확인할 수 없으면 교체를 중단한다. 최종 package의 `--stage validated`는 지원 프로필·검증 결과·릴리스 노트가 일치해야 한다.

- [ ] **1. 실패 검사 작성:** 공백 경로, 최초 설치, 이전 DLL 업그레이드, 설치 중 실패, 설치 후 사용자 수정 파일, 제거/복원, 게임 실행/종료 확인 불가를 fixture로 검증한다. 알 수 없는 DLL 덮어쓰기·사용자 변경 파일 삭제·설정/다른 mod 삭제는 금지한다.

```python
# test_uninstall_preserves_user_modified_file
assert installed_dll_sha_after == user_modified_sha
assert config_sha_after == config_sha_before
assert unrelated_mod_hashes_after == unrelated_mod_hashes_before
```

- [ ] **2. 실패 확인:** `python3 -m unittest discover -s tests -p test_install.py -v` → 미구현 실패. package 검사는 dirty source, DLL/manifest 불일치, 검증 기록 없는 validated stage를 거부한다.
- [ ] **3. 최소 구현:** 소유 파일 manifest·백업·복구와 명확한 실패 반환을 구현한다. 설치 자료에는 Steam Deck REFramework 전제, Steam Input 단축키, 기본 OFF, UI 상태의 의미, 로그 수집, 확인된 지원 범위를 적는다. 과도한 자동 정리나 다른 mod 삭제는 하지 않는다.
- [ ] **4. 통과·커밋:** `bash scripts/test.sh` 및 `python3 -m unittest discover -s tests -p test_install.py -v` → 성공 후 `git commit -m "feat: package verified Fullbright builds with reversible install"`.
- [ ] **5. 배포물 확인:** 깨끗한 커밋에서 `bash scripts/build.sh`, `python3 tools/package.py --stage validated --output dist`를 수행해 체크섬·소스 커밋·기본 OFF를 확인한다. 배포 전 Deck 새 설치/업데이트/제거·효과 ON/OFF를 확인한다. 성능·장면 미검증을 릴리스 노트에서 감추지 않는다.
- [ ] **6. 릴리스:** CI는 일반 push/PR에 검증과 artifact만 제공한다. 실기 근거가 있는 버전만 태그·릴리스로 승격하며 태그 생성 자체를 자동 구현 완료 조건으로 삼지 않는다.

## 자체 검토와 실행 인계

- 설계의 대상·Native 경계·단계적 효과 검증은 Tasks 1–7, 상태 수명·전환은 3/6/8, 로그·UI는 4, 실기·성능은 7/8, 설치·저장소·출처는 2/5/9에 대응한다.
- 공통 타입·함수는 생산 작업과 소비 작업을 맞췄다. site별 주소·피연산자·상수는 Task 5 근거의 산출물이며 미래 작업자가 추측할 값이 아니다.
- Review Focus 5개는 각 소유 작업의 실패 사례/실기 절차에 포함했다. 사용자 목표의 시각적 판정은 모의 검사로 대체하지 않는다.
- 실행 추천은 **이 세션의 주 에이전트가 순차 구현하고 마지막에 독립 검토**하는 방식이다. 역분석 근거와 상태·훅 인터페이스 의존성이 높아 단계별 구현자 교체의 이득이 작다. 이는 제품의 Native 구현 방식과 별개인 작업 운영 방식이다.
- 첫 실행 묶음은 Tasks 1–4 및 Task 5의 RE7 정적 분석이다. RE8 자료 확보 이후 Task 5 매핑을 확정하고 6–7의 첫 효과 검증으로 진행한다. 게임 입력이 부족하면 해당 관문에서 필요 자료를 구체적으로 요청한다.
- 이 문서는 실행 계획이다. 여기에 적힌 명령·예상 통과 결과는 향후 검증 계약이며 이번 문서 작성에서 실행된 제품 테스트 결과가 아니다.
