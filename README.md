# IOCP 기반 C++ 채팅 / 커뮤니티 서버 포트폴리오

## 📌 프로젝트 개요

본 프로젝트는 **온라인 게임 서버 환경에서 사용되는 채팅 서버 및 커뮤니티 서버 구조**를  
**Windows IOCP 기반 C++ 서버 아키텍처**로 구현한 포트폴리오입니다.

실제 상용 게임 서버에서 사용되는 구조와 패턴을 기반으로 하여,  
다음과 같은 요소를 중점적으로 설계했습니다.

- 안정적인 세션 관리
- 명확한 책임 분리
- 멀티스레드 환경에서의 안전한 객체 수명 관리
- 네트워크 장애 상황을 고려한 재접속 설계
- 실무 중심의 코드 스타일 (과도한 추상화 배제)

---

## 🛠 개발 환경

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

## 🧩 주요 구성 요소

### 1️⃣ Session 계층

#### CommunityServerSession
- 커뮤니티 서버와의 TCP 연결을 담당
- 연결 실패 및 Disconnect 발생 시 **지수 백오프 기반 자동 재접속**
- 재접속 중복 방지를 위한 `atomic` 플래그 관리