# RE8 FullBright

Steam Deck / Proton에서 Resident Evil Village(RE8)의 어두운 공간을 장면별 광량 조절 없이 볼 수 있도록 하는 REFramework Native 플러그인 프로젝트입니다.

**현재는 설계 단계입니다. 실행 가능한 모드, 설치 ZIP, RE8에서 검증된 Fullbright 효과는 아직 제공하지 않습니다.**

## 개발 방향

- RE7 OTS Fullbright의 효과와 구현을 참고해 RE8의 대응 제어 지점을 분석합니다.
- 조명 제어는 Native로 구현합니다. Lua 조명 속성 실험을 반복하는 단계를 선행하지 않습니다.
- 첫 목표는 지원 빌드의 어두운 장면 한 곳에서 효과와 ON/OFF 복귀를 확인하는 것입니다.
- 이후 가까운 물체의 과노출, 먼 지형의 시인성, 회전·치료·조사·컷신·로드 전환, Steam Deck 성능을 검증합니다.
- 빌드와 오프라인 검사가 통과해도 실제 게임 화면과 성능까지 검증되었다고 간주하지 않습니다.

## 문서와 상태

| 문서 | 상태 |
| --- | --- |
| [Native Fullbright 설계안](docs/superpowers/specs/2026-10-24-native-fullbright-design.md) | Native 중심 접근 승인, 상세 설계 검토 대기 |

1차 대상은 Steam Deck/Proton입니다. 한 게임 빌드·본편 1인칭·RT OFF를 초기 범위로 제안하며, 실제 실행 파일과 REFramework/Proton 버전은 구현 전에 확인합니다.

설계 검토 후 상세 실행 계획, 코드, 조사 근거, 실기 결과를 이 저장소에서 관리합니다. 게임 실행 파일, 원본 CT, 세이브 및 개인 런타임 덤프는 배포하지 않습니다.

## 참고 프로젝트

- [RE7 OTS](https://github.com/like-shock/re7-ots), 기준 커밋 `ffde226b12c76e3dfec806e8e0a23de267e2b6a5`
- [REFramework](https://github.com/praydog/REFramework)

원본 모드와 외부 코드의 권리·고지는 각 출처를 따릅니다. 이 저장소는 비공식 모드 프로젝트입니다.
