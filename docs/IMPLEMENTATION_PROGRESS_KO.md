# Native foundation 구현 진행 — 2026-10-24

승인된 첫 실행 묶음(Tasks 1–4 및 Task 5의 RE7 정적 분석)을 구현했다. 실제 Fullbright는 아직 미구현이며 지원 RE8 프로필은 0개다. main은 설계/계획 커밋에 두고 feature/native-foundation에서 작업한다.

| 범위 | 구현·검증 상태 |
|---|---|
| Task 1 환경 수집 | 도구와 7개 합성 검사를 완료. 실제 Deck 환경 기록 대기 |
| Task 2 Native/식별 | PE/SHA-256/파일 변경 감지, 고정 의존성, DLL·manifest 구현. CI 빌드·바이너리 검사 통과 |
| Task 3 profile/runtime | 전체 site 묶음 검증, 유일 패턴·원본 바이트·범위 검사, OFF/세대/FAULT 상태 검사 통과 |
| Task 4 메뉴/설정/로그 | 실제 Lua host harness, 키 latch, 설정 오류·저장 경합, 큐 포화·실제 파일 쓰기 실패·회전 검사 통과 |
| Task 5 RE7 분석 | 7곳+OLD_HOUSE/준비 조건 분석 완료. RE8 매핑은 WAITING_FOR_TARGET_EVIDENCE |
| Tasks 6–9 | RE8 site 계약/효과를 전제하므로 아직 시작하지 않음 |

## 실행한 검증

- 최종 수정 후 로컬 전체 검사: 21개 통과, Windows 바이너리 검사 클래스는 DLL 부재로 명시적 skip.
- [CI 40075525963](https://github.com/like-shock/RE8_FullBright/actions/runs/40075525963): Windows DLL 빌드 + 전체 22개 검사 통과. remote commit `9f73ac65f5fff71f616a1455bd0f0299bc7ed62e`.
- 해당 remote와 local `bda9b47`의 Git tree 모두 `af863cc272ad9d77a3e0c92d7f91c70a8feac2b7`로 동일. 문서 커밋은 그 이후 추가됨.
- 실제 Deck 로드/REFramework 메뉴/Proton 단축키/시각적 효과/성능: **미검증**. 호스트·CI 성공으로 실기 성공을 대체하지 않는다.

## 작업 중 내린 결정과 비용

1. 스킬의 task-start/task-done 실행 자원이 제공되지 않아 동일 brief/기준 커밋/RED→GREEN/완료 기록을 수동 ledger로 관리했다. 비용: 재개 시 Git과의 대조가 필요하다.
2. 로컬 컴파일러 패키지 설치 제한으로 호스트 검사는 로컬, Windows DLL 검증은 CI에서 수행했다. 비용: CI 성공 전에는 Windows 빌드 성공을 주장할 수 없다.
3. CLI push 인증이 없어 연결된 GitHub API로 개발 브랜치를 내보냈다. 비용: 로컬/원격 커밋 ID가 다르므로 tree 해시로 내용 동일성을 확인한다.
4. 실제 첫 CI에서 관측한 MinGW 패키지 버전을 고정했다. 비용: 초기 artifact 및 CI runner 전체의 비트 단위 재현성을 보장하지 않는다.
5. 설정/로그에 공통 wide-path 파일 helper를 추가했다. 비용: Windows 경로/원자 교체의 실제 Proton 동작은 별도 실기 확인이 필요하다.
6. UI detach는 세대 토큰을 무효화한다. 개별 callback 삭제 API가 없어 같은 Lua 상태에서 인위적 detach/attach 반복 시 비활성 closure는 state 파괴까지 남는다. 비용: 이러한 비정상 반복에서는 일시적 메모리 보유가 있다.
7. DLL/worker/callback을 프로세스 수명에 고정했다. 비용: 게임 실행 중 DLL unload/hot replacement를 지원하지 않으며 종료까지 메모리를 유지한다.
8. foundation에서는 registry 인식만으로 지원 상태를 켜지 않는다. 비용: 후속 작업에서 live image 검증과 훅 설치 전체를 연결해야 한다.

## 독립 검토와 수정

새 컨텍스트의 독립 리뷰어가 `53b1ee7..753a16a` 전체를 검토했다. Critical/Important 0개, Minor 2개였다. 실행자는 그중 캐시에 추가된 Lua 소스가 빌드 glob에 포함되는 문제를 **빌드 입력 계약 위반(Important)**으로 재분류했다. .c/.h 회귀 fixture가 실패하는 것을 확인한 후 예상하지 않은 소스를 거부하도록 수정했다. 로컬 전체 21개 통과, 바이너리 검사는 CI의 필수 체크로 수행한다. 최종 커밋의 결과는 Draft PR의 Checks에서 확인한다. 리뷰 반복 대신 회귀 검사로 수정 여부를 검증한다.

남긴 경미한 문제: 설정 파일에 NUL 바이트가 있으면 그 뒤 문자열이 검증에서 잘려 일부 설정이 오류 없이 적용될 수 있다. ON 상태는 저장하지 않으므로 자동 활성화로 이어지지는 않는다. 길이를 인식하는 config parser 보완은 후속 항목이다.

리뷰에서 판정을 유보한 항목과 실행자의 결정:

- RE8 명령·소유자·세대: Task 5 실제 자료 대기를 유지한다. 비용은 효과 구현 지연이다.
- 훅 철회·쓰기 직전 검사·CPU/트램펄린 수명: Task 6 범위다. 현재는 훅을 허용하지 않는다.
- 조명·전환·시각 결과·Deck 성능: Tasks 7–8 실기까지 미검증이다. 효과/속도를 보장하지 않는다.
- Windows/Proton 실행·설치 REFramework·파일시스템: CI 빌드는 실행 증거가 아니다. 첫 실기에서 호환성 문제가 나올 수 있다.
- 설치·제거·릴리스 인증: Task 9로 유보한다. 현재 artifact는 정식 설치 패키지가 아니다.
- 비트 재현성: 주장하지 않는다. 다른 호스트에서 바이너리가 달라질 수 있다.
- closure 물리 삭제/DLL unload: 기존 세대 무효화/프로세스 수명 결정을 유지한다. 해당 수명까지 메모리를 보유한다.

자료 관문에서 아직 전체 계획이 끝나지 않았으므로 worktree/ledger를 보존한다. 이후 재개를 위한 기록이며 변경 결과는 개발 브랜치에 저장한다.

## 후속 입력

[환경 수집 안내](testing/ENVIRONMENT_KO.md)의 `environment.txt`와 같은 설치본의 비공개 실행 파일 분석 자료가 필요하다. [RE8 매핑 관문](research/re8-mapping.md)에 필요한 증거와 미확인 항목을 기록했다.
