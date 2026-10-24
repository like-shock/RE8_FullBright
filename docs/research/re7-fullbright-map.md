# RE7 Fullbright 최소 의존성 분석

분석 기준: [re7-ots / ffde226b](https://github.com/like-shock/re7-ots/tree/ffde226b12c76e3dfec806e8e0a23de267e2b6a5). 아래 이름은 기존 모드의 명칭이며 엔진의 공식 의미를 보장하지 않는다. RE8에 대응하는 주소·객체·효과는 아직 확인하지 않았다.

근거: [native_fullbright.c](https://github.com/like-shock/re7-ots/blob/ffde226b12c76e3dfec806e8e0a23de267e2b6a5/diagnostics/src/native_fullbright.c), [manifest.h](https://github.com/like-shock/re7-ots/blob/ffde226b12c76e3dfec806e8e0a23de267e2b6a5/diagnostics/src/manifest.h). 원본 CT/게임 파일은 이 저장소에 포함하지 않는다.

## 7개 Fullbright 위치와 별도 밝기 의존성

| RE7 위치 | 원래 명령 / 참조 코드의 동작 | RE8에서 새로 입증할 계약 |
|---|---|---|
| LIGHT_SOURCE_DIAMETER_READ | RAX+0x80/90/A0/B0의 4개 벡터를 XMM14/15/12/11로 읽고 RAX+0xD0의 ECX를 원래 RSP+0x5C에 저장. 대상 마커를 재확인한 뒤 벡터 성분 3=1000, 10=-1.9, 12–14=RGB 보정, 15=-500으로 치환 | 이 계산을 소비하는 광원, 소유 관계, 스택 저장과 벡터 읽기의 순서·겹침, 반환 시 살아 있는 레지스터 |
| LIGHT_RADIUS_MULSS_READ | `mulss xmm0,[rbx+0x0c]`를 대상 광원에서만 스칼라 ×1000으로 치환. RBX-0x80이 추적 광원인지 확인 | 반경인지 다른 계수인지, 스칼라 상위 lane 보존, 광원 수명 |
| LIGHT_RADIUS_MOVSS_READ | `movss xmm0,[rbx+0x0c]`를 대상에서 1000으로 치환 | MOVSS의 정확한 lane 효과, 동일 객체 판정 |
| EXAMINE_LIGHT_CMP_1 | RSI+0x13의 바이트 CMP와 긴 JNE. 포인터 체인으로 대상 확인 시 flags를 equal 결과로 바꾸고 검증한 fallthrough 사용 | RE8의 광원 활성 조건, flags의 후속 소비, 분기 대상 재배치 |
| EXAMINE_LIGHT_CMP_2 | RBX+0x13의 바이트 CMP와 짧은 JNE(+0x10). 대상이 아니면 원본 트램펄린 실행 | 실제 기계어 분기를 기준으로 검증. CT의 모순된 주석 주소를 따르지 않음 |
| GLOBAL_BRIGHTNESS | R14+0x180 벡터를 읽어 XMM0의 첫 성분만 0.125로 치환, shadow_override 설정 | 전역/지역/객체 중 어떤 계산인지, 읽기 실패 시 상태 철회 |
| G_BRIGHTNESS_2 | R14+0x1D0의 스칼라 읽기를 XMM10=0.27로 치환 | 원래 lane 효과와 대상 범위, 다른 렌더링 경로에 미치는 영향 |
| OLD_HOUSE_BRIGHTNESS_READ (별도) | R14+0x228의 EAX 읽기를 shadow_override가 있을 때 RAX=0으로 치환 | RE7 특정 경로. RE8에 필요하다는 근거가 없으므로 자동 이식하지 않음 |

RGB 계산은 바이트×(1/255 근사 float)×사용자 밝기×150000을 x87 순서로 계산한다. 이 수치·순서·마커 `0x48127c00`·객체 오프셋은 **RE7의 관측값**이다. RE8 기본 프리셋으로 사용하지 않는다. 대상 판별 포인터 체인(base+0x8FF2848 → +0x150 → +0x50 → 최종 +0xF93)도 RE7 전용이다.

## 준비 조건과 최소 수명 관측

`first_person_ready`는 OTS 카메라 OFF에서도 player/aim/area/timer가 유효하고 필요한 observer가 실행되었는지 확인한다. 플레이어 존재, 이전/현재 aim 소유자 일치, 로딩 화면 아님, 유한한 게임 시간 ≥1이 필요하다. RE7 지역 35의 예외와 BETTER_TIMER 조건은 RE8로 옮기지 않는다.

옵션 OFF, 준비 상태 상실, 플레이어·aim·지역 변경, 대상 마커 읽기 실패/변경 시 캐시 광원과 shadow_override를 철회한다. 공유 명령이 다른 광원을 방문했다는 이유만으로 정상 대상 캐시를 지우지는 않는다. RE8에서는 최소한의 소유자/세대 관측으로 이 수명 계약을 다시 증명해야 한다. 카메라/FOV/모델/HUD/충돌 기능 전체는 이식 대상이 아니다.

## 독립 기반의 참고·재사용 구분

| 후보 근거 | 가져올 원칙/테스트 | 이번 구현 상태 |
|---|---|---|
| diagnostics/src/native_bridge.{c,h,S}, tests/test_native_bridge.py | GPR·flags·확장 레지스터 보존과 원래 명령 재현 검증 | 아직 복사·링크하지 않음. Task 6에서 RE8 site 계약을 기준으로 재검증 |
| diagnostics/native_build.sh 및 MinHook 고지 | 독립 DLL 빌드, 입력 고정, 재배치 의존성 관리 | 공식 REFramework 헤더와 새 빌드 스크립트 사용. MinHook은 아직 링크하지 않음 |
| tests/test_native_runtime.py, native_optional_host.c | OFF 원복, 대상 아닌 호출, 읽기 실패, 준비 조건 상실 | RE8 foundation에는 새 profile/runtime 합성 검사를 작성함 |
| tests/test_native_fullbright_trace.py | 이벤트 큐 포화, 상태 전환과 관측 이유 분리 | 새 bounded journal로 구현. RE7 런타임을 복사하지 않음 |
| tests/test_native_options.py, test_native_lua.py | 설정·최소 UI와 Native 상태 분리 | 새 구현, 실제 Lua 5.4.3 host harness로 검사 |

출처·재사용 조건은 [RE7 NOTICE](https://github.com/like-shock/re7-ots/blob/ffde226b12c76e3dfec806e8e0a23de267e2b6a5/diagnostics/NOTICE.md)를 따른다. RE7에서 파생 코드를 실제 옮길 때 원 저작자/ORIGINAL_CREDITS와 파일별 조건을 함께 반영한다. 분석 문서가 코드 재사용 허가 전체를 대체하지 않는다.

RE7에서 동작한 설계라는 사실만으로 RE8 성공률을 수치화할 수 없다. 다음 판정 기준은 RE8 명령·객체 수명·실기 OFF/ON/OFF 화면이다. 회복 동작 뒤 어두워지는 RE7의 기존 미해결 사례 역시 RE8 실기 매트릭스에서 확인한다.
