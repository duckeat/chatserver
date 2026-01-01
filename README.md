# IOCP 기반 C++ 채팅 / 커뮤니티 서버 포트폴리오

## 프로젝트 개요

본 프로젝트는 **온라인 게임 서버 환경에서 사용되는 채팅 서버 및 커뮤니티 서버 구조**를  
**Windows IOCP 기반 C++ 서버 아키텍처**로 구현한 포트폴리오입니다.

실제 상용 게임 서버에서 사용되는 구조와 패턴을 기반으로 하여,  
다음과 같은 요소를 중점적으로 설계했습니다.

- 안정적인 세션 관리
- 명확한 책임 분리
- 멀티스레드 환경에서의 안전한 객체 수명 관리
- 네트워크 장애 상황을 고려한 재접속 설계
- 실무 중심의 코드 스타일 (과도한 추상화 배제)
- 기존 프로젝트에서 사용한 라이브러리 사용 (빌드시 관련라이브 필요)

---

## 개발 환경

- **Language**: C++17  
- **Platform**: Windows  
- **Network Model**: IOCP (I/O Completion Port)  
- **Thread Model**: Worker Thread + Task Queue  
- **Memory Management**:  
  - `std::shared_ptr`
  - `std::weak_ptr`
  - `std::atomic`
- **Build Tool**: Visual Studio (MSVC)

---

## 주요 구성 요소

### Session 계층

#### CommunityServerSession
- 커뮤니티 서버와의 TCP 연결을 담당
- 연결 실패 및 Disconnect 발생 시 **지수 백오프 기반 자동 재접속**
- 재접속 중복 방지를 위한 `atomic` 플래그 관리
- weak_from_this()를 사용하여 비동기 스레드에서 안전한 객체 참조
- 세션 수명과 재접속 로직을 명확히 분리

### ChatSession / ChatSessionManager
#### ChatSession
- 하나의 유저(SessionId)에 대응되는 논리 세션
- 여러 채팅 타입(Normal / Zone 등)에 대해 Room 참여 상태 관리
- 세션 ID 교체(Exchange) 시 Room 재입장 처리

##### ChatSessionManager
- 고정 인덱스 기반 세션 테이블 관리
- 세션 등록 / 해제 / 교체 담당
- Unregister 이후에도 필요한 정리 작업을 IOCP Task로 비동기 처리

### Room / RoomManager / RoomService
#### Room 구조
- (RoomID + ChatType) 조합으로 Room 식별
- 세션은 Room을 소유하지 않으며, Room 역시 Session의 생명주기에 의존하지 않음
- 순환 참조가 발생하지 않는 구조

#### RoomService
- Room 생성 / 입장 / 퇴장 로직을 서비스 레이어로 분리
- Session → Service → Manager → Room 흐름 유지
- 게임 서버에서 흔히 사용되는 Application Service 패턴 적용

### 재접속 설계 (Reconnect Strategy)
- 연결 실패 또는 Disconnect 발생 시 자동 재접속
- 지수 백오프 적용 (최대 10초)
- 중복 재접속 방지
- 객체 파괴 시 즉시 중단

### 멀티스레드 안전성
- 상태 플래그는 모두 std::atomic 사용
- IOCP Worker Thread 외부 작업은 Task Queue로 전달
- 객체 수명은 명시적으로 관리하며 암묵적 소멸에 의존하지 않음

### 설계 철학
- 실무에서 검증된 패턴 우선
- 불필요한 스마트 포인터 남용 지양
- “모던 C++”보다 “안전하고 예측 가능한 서버 코드”를 우선
- 디버깅과 유지보수를 고려한 구조

---

## 참고 사항
본 코드는 학습 및 포트폴리오 용도로 공개됩니다.
상용 프로젝트 적용 시 환경에 맞는 추가 보완이 필요합니다.