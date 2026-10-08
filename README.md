# ProjectSP

GAS와 Dedicated Server 기반의 **멀티플레이 액션 전투 프로토타입**입니다. 여러 플레이어가 같은 전장에 참여해 AI 적과 전투하고 스킬을 확장하는 플레이를 구현했습니다.

## 프로젝트 개요

| 항목 | 내용 |
| --- | --- |
| 개발 환경 | Unreal Engine 5, C++ |
| 개발 인원 | 1인 개인 프로젝트 |
| 1차 개발 기간 | 2026.08.01 ~ 2026.09.28 |
| 개발 목표 | 기능 확장과 네트워크 환경을 고려한 멀티플레이 전투 아키텍처 설계 |
| 주요 기술 | Gameplay Ability System(GAS), GameplayTag, Dedicated Server, Enhanced Input, Behavior Tree, AI Perception |
| 기준 커밋 | `26.09.28 1차 마감` — 2026-09-29 |

> 이 README는 2026.09.28 기준 1차 마감 범위를 정리합니다. 개발 기간 중 2026.08.24 ~ 2026.09.12에는 이사 및 개인 사정으로 개발을 일시 중단했습니다.

## 핵심 구현

### GAS 기반 전투 및 스킬 확장

- GameplayAbility의 서버·클라이언트 처리를 분리하고, 대미지 적용 등 전투 결과를 서버에서 처리합니다.
- GameplayAbility Blueprint에서 확장 GameplayTag의 장착 여부에 따라 스킬 동작을 분기합니다. 확장 요소의 구매는 서버에서 검증하고 적용합니다.
- 범위, 대상과의 관계, 정렬 기준을 정의하는 공통 표적 쿼리를 구현했습니다.
- GameplayField에 효과 적용 시점, 추종 방식, 효과 제거 조건을 두어 범위 공격의 생명주기를 관리합니다.
- 공용 CooldownGameplayEffect를 사용해 Ability별 쿨타임을 처리합니다.

### Dedicated Server 기반 멀티플레이 흐름

- 로비, 시작 준비, 플레이 단계를 상태 머신으로 분리했습니다.
- 로비에서 참가자 전원의 Ready 상태와 방장의 시작 요청을 확인합니다.
- 시작 준비 단계에서 각 클라이언트의 초기 Unit 표현 준비 완료를 확인한 뒤 플레이로 전환합니다.
- 준비 제한 시간을 초과한 플레이어와 시작 준비·플레이 중 입장한 플레이어는 관전자로 처리해 다른 참가자의 진행을 막지 않도록 구성했습니다.
- 방장 퇴장 시 역할을 다른 참가자에게 넘기고, 모든 참가자가 퇴장하면 참가자 목록과 시작 상태를 초기화합니다.

### Enhanced Input 기반 입력 해석

- InputDefinition에 InputAction, InputTag, AbilityTag, InputBehavior를 연결합니다.
- InputBehavior가 설정된 입력은 해당 Behavior에서 해석하고, 설정되지 않은 입력은 연결된 Ability를 직접 활성화합니다.
- PrimaryInputBehavior는 커서 아래 대상에 따라 이동과 공격을 구분합니다. 공격 대상이 선택되면 사거리와 방향을 맞춘 뒤 Ability를 활성화합니다.
- PlayerActionComponent에서 누르고 있는 입력과 활성 InputBehavior를 관리하며, 새 입력에 따른 행동 중단과 Ability 실행 후 입력 재해석을 처리합니다.

### 데이터 기반 Unit 구성 및 생명주기 관리

- Character를 상속한 Unit과 관련 DataAsset 설정을 UnitDefinition으로 구성합니다.
- SpawnSubsystem에서 서버의 Unit 생성, 준비, 활성화를 처리합니다.
- 관리 인터페이스를 통해 초기화, 정리, 활성화, 조작 가능 여부와 상태 태그 변경을 필요한 컴포넌트에 전달합니다.
- 서버의 준비·활성화·반납과 클라이언트의 표현 준비·해제를 분리합니다.
- ActorPoolSubsystem으로 Unit을 반납하고 재사용하며, UnitRegistrySubsystem으로 전투 참여 Unit 등록과 진영·범위 검색을 처리합니다.

### Behavior Tree AI 및 위협도 시스템

- Behavior Tree로 적의 순찰, 추적, 공격, 복귀 흐름을 구성했습니다.
- AI Perception의 시야 감지로 기본 위협도를 부여하고, 실제 적용된 피해량에 비례해 공격자의 위협도를 높입니다.
- 투사체 피해도 소유 관계를 따라 공격 Unit을 찾아 위협도에 반영합니다.
- 시야에서 벗어난 대상의 위협도를 일정 시간 유지한 뒤 감쇠·제거합니다.
- 현재 대상보다 위협도가 충분히 높은 후보가 있을 때 대상을 교체하고, 스폰 지점 기준 추적 범위를 벗어난 대상은 제외합니다.

## 코드 구조

주요 디렉터리와 역할은 다음과 같습니다. 경로는 저장소 루트 기준입니다.

| 경로 | 역할 |
| --- | --- |
| `ProjectSP/Ability/` | GAS 공통 처리, 스킬 확장, GameplayField, 표적 쿼리, 쿨타임 처리 |
| `ProjectSP/GameFramework/` | GameMode, GameState, PlayerController, PlayerState 및 게임 단계 상태 머신 |
| `ProjectSP/Input/` | Enhanced Input 연동과 InputBehavior 기반 입력 해석 |
| `ProjectSP/Definition/` | Unit, 입력, AI, Ability 등 DataAsset 기반 설정 |
| `ProjectSP/Attribute/` | HP, 공격력, 이동 속도 등 AttributeSet |
| `ProjectSP/Subsystem/` | Unit 생성, Actor 풀링, 전투 참여 Unit 등록 및 검색 |
| `ProjectSP/Unit/` | 플레이어·AI Unit, 기능별 컴포넌트, AI Controller와 Behavior Tree Task·Service |
| `ProjectSP/UI/` | Attribute 표시와 Widget 관련 클래스 |
| `ProjectSPServer.Target.cs` | Dedicated Server 빌드 타깃 |

## 시연 영상 및 포트폴리오

| 링크 | 내용 |
| --- | --- |
| [스킬별 소개 영상](https://youtu.be/DyVocE9Bfh8) | 스킬 동작 시연 |
| [2인 멀티플레이 영상](https://youtu.be/-AFjZgM8bhY) | 두 플레이어가 참여하는 멀티플레이 시연 |
| [빌드 파일 — Google Drive](https://drive.google.com/drive/u/2/folders/1mbZqjwGV8Nk7oH49FeP-MFNfSVCtYjOv) | 프로젝트 빌드 파일 |

## 한계와 향후 계획

### 현재 한계

1차 마감 기준, 클라이언트에서 해석한 입력 결과 전반을 서버에서 검증하는 구조는 추가 구현이 필요합니다. 현재 입력 검증 범위는 **Ability 활성화 조건과 대상의 공격 가능 여부**에 한정됩니다.

서버에서 전투 결과를 처리하는 구조와 별개로, 입력 해석 결과 전반에 대한 서버 검증까지 완료된 상태는 아닙니다.

### 향후 계획

- 클라이언트가 해석한 입력 결과 전반에 대한 서버 검증 구조를 보완합니다.
- 기존 GAS 및 스킬 확장 구조를 활용해 다양한 스킬과 확장 요소를 추가합니다.
