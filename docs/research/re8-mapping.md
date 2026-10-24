# RE8 매핑 관문

상태: **WAITING_FOR_TARGET_EVIDENCE** — 2026-10-24

지원 게임 프로필: **0**. Native foundation은 밝기를 변경하지 않으며 `UNSUPPORTED_BUILD`를 유지한다. 현재 소스에는 detour 설치·게임 광원 쓰기가 없다.

## 필요한 입력

1. [환경 수집 안내](../testing/ENVIRONMENT_KO.md)를 따라 Steam Deck에서 만든 `environment.txt`.
2. 같은 설치본의 분석 가능한 `re8.exe`, 또는 SHA-256과 연결된 충분한 디스어셈블리·메모리/런타임 관측 자료. 공개 issue/PR에 EXE나 덤프를 올리지 않는다.
3. 실제 사용하는 REFramework 버전/dinput8.dll SHA, Proton 버전, RT OFF 확인. ABI 호환성·DLL 로드·UI는 실제 설치본에서 따로 확인한다.

자료는 `private/<exe-sha256>/`에만 두고 Git에 넣지 않는다. 현재 실제 게임 자료는 확보하지 않았다. Steam Deck 하드웨어 연결도 없으므로 로드 성공·Fullbright 효과·FPS 영향은 미검증이다.

## 각 대응 지점의 기록 양식

| 항목 | 현재 |
|---|---|
| 빌드 SHA / PE machine·timestamp·SizeOfImage | 미확인 |
| 섹션 / RVA / 유일 AOB / 원래 바이트 | 미확인 |
| 명령 경계·재배치·원래 분기 대상 | 미확인 |
| GPR / flags / SIMD / stack의 입력·출력 | 미확인 |
| 실제 소비 의미 / 객체 소유자 / 수명·세대 | 미확인 |
| 허용 호출 조건 / 실패 시 원본 실행 | 미확인 |
| 최소 후보 묶음 / 기본 프리셋 효과 | 미확인 |

RE7 분석은 [re7-fullbright-map.md](re7-fullbright-map.md)에 있다. RE7 주소/마커/밝기 상수를 RE8 프로필로 변환하지 않았다. `tools/verify_profile.py`, hook bridge, 조명 보정, 설치 ZIP, 장면 검증, 릴리스는 승인된 계획의 Task 5 자료 관문 이후에 진행한다.
