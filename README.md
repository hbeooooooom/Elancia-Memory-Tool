# Elancia-Memory-Tool

# Windows API 기반 프로세스 메모리 조작 게임핵 개발

## 개요

### 개발 목적

- Windows 프로세스 간 메모리 조작 실습
- Win32 API를 활용한 시스템 프로그래밍 학습
- 게임 클라이언트 보안 취약점 분석

### 개발 환경

- 언어 : C++(Win32 API)

## 아키텍처

```c
[사용자 인터페이스]
       ↓
[프로세스 탐지 모듈] → [대상 프로세스 선택]
       ↓
[메모리 주소 분석] → [모듈 베이스 주소 계산]
       ↓
[메모리 조작 엔진] → [WriteProcessMemory API]
       ↓
[대상 프로세스 메모리 수정]
```
자세한 변경값은 공개하지 않습니다.
### 접근 권한 획득

```c
HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, targetPID);
```

- `OpenProcess` 함수를 통해 Elancia에 대한 핸들러를 가져옴

### 메모리 주소 계산

```c
DWORD_PTR GetModuleBaseAddress(DWORD processID, const wchar_t* moduleName) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processID);
    // 모듈 스냅샷을 통한 베이스 주소 계산
}
```

- Base 주소에 직접 구한 offset을 더한다.
- ASLR로 인해 바뀌는 주소 문제 해결

### 메모리 내용 획득

```c
ReadProcessMemory(hProcess, targetAddress, buffer, size, &bytesRead);
```

- 앞에서 가져온 핸들러를 이용해 해당 프로세스의 원하는 메모리 주소를 읽어 buffer에 저장을 한다.

### 메모리 덮어 쓰기

```c
WriteProcessMemory(hProcess, targetAddress, data, size, &bytesWritten);
```

- 핸들러를 이용해 해당 프로세스의 원하는 메모리 주소에 data 값을 쓴다.

### 공격력 조작

```c
void CheatEngine_AttackAlwaysPerFect() {
    // 게임 코드에서 데미지 계산 부분을 수정
    const BYTE PERFECT_OPCODE = ;  // "항상 크리티컬" 명령어
    
    WriteProcessMemory(hProcess, attackFunctionAddress, &PERFECT_OPCODE, 1);
    // 결과: 모든 공격이 크리티컬(2배 데미지)로 처리됨
}
```

- 직접 메모리 분석을 통해 알아낸 주소에 0xA2를 넣어 항상 크리티컬이 뜨도록 덮어쓴다.

### 이동속도 조작

```c
void CheatEngine_MoveSpeedTo(int speed) {
    // 1단계: 속도 체크 코드를 무력화
    const BYTE NOP_BYTE = 0x90;  // NOP 명령어 (아무것도 안함)
    
    // 게임 코드: if (speed > 200) speed = 200;  
    // NOP으로 덮어써서: (아무것도 안함)   
    WriteProcessMemory(hProcess, gameBase, &NOP_BYTE, 1);
    
    // 2단계: 실제 속도 값 변경
    WriteProcessMemory(hProcess, playerSpeedAddress, &speed, sizeof(speed));
}
```

- 적정 이동속도의 조건을 걸고 넘어가는 속도를 입력시 동작을 무시한다.
- 이후 조건문을 통과하면 실제 속도를 입력한 값으로 변경한다.


