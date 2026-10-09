# Tesla Unlock Android 1.2.1

등록된 장치로 앱 실행 시 자동 연결을 시도하는 버전입니다. 장치 프로필과 앱 전용 로컬 Wi-Fi 연결 기능을 유지합니다. 앱 패키지는 `dev.t2can.unlock`, versionCode는 5이며 Android 8(API26) 이상을 유지합니다. 자동 연결은 API29 이상에서 지원하고 구형 기기와 호환성 문제가 있는 기기는 수동 연결을 사용합니다.

장치의 기존 `/api/profile/status` 응답과 프로세스 라우팅을 확인한 다음 대시보드를 엽니다. 다른 앱은 OS 기본 인터넷 경로를 사용합니다. 셀룰러 병행 성공은 실기기에서 따로 확인해야 하며 이 앱이 다른 앱을 LTE/5G로 강제하지 않습니다.

## 시작하기

**Devices / connection settings → Devices**에서 실제 SSID·보안 방식·암호·주소를 등록하고 **Connect**를 누르십시오. 최초 기본 프로필은 빈 SSID의 수동 연결용이며 암호를 포함하지 않습니다. 자세한 절차는 [사용 안내](docs/user-guide.md)를 참조하십시오.

- 선택한 장치가 등록되어 있고 Wi-Fi 권한이 승인되어 있으면 앱을 새로 열 때 1회 자동 연결합니다.
- 프로필 최대 20개, Android Keystore 기반 암호화 저장.
- 기존 OTA 파일 선택·Blob 로그 저장·뒤로가기·테마 기능 유지.
- 네트워크 상실 시 대시보드를 닫고 현재 화면에서는 Connect로 재시도하며, 앱을 종료한 뒤 새로 열면 자동으로 1회 재시도합니다. 이전 POST 자동 재전송 없음.
- HTTP는 `192.168.4.1`의 지정 포트만 허용. 다른 사설 IPv4는 신뢰할 수 있는 인증서의 HTTPS만 허용.
- 펌웨어·CAN 처리·차량 제어 로직 변경 없음. 백그라운드 상시 연결 보장 없음.

## 문서와 검증

[기존 소스 감사](docs/t2can-network-audit.md), [구현과 제한](docs/t2can-network-implementation.md), [개발 명세](docs/development-spec-2026-10-09.md), [검증 결과](VALIDATION.md)를 함께 확인하십시오. 호스트·컴파일·에뮬레이터 결과와 실제 Wi-Fi/셀룰러·OTA·차량 검증은 구분합니다.

## 빌드와 서명

Java 21, Android SDK Platform 36, Build Tools 36.0.0, Python 3를 사용합니다. Gradle·외부 앱 라이브러리는 필요하지 않으며 Java 소스는 Java 8 bytecode로 컴파일합니다.

```sh
export T2CAN_JDK=/absolute/path/to/jdk/Contents/Home
export T2CAN_BUILD_TOOLS=/absolute/path/to/android-sdk/build-tools/36.0.0
export T2CAN_ANDROID_JAR=/absolute/path/to/android-sdk/platforms/android-36/android.jar
export T2CAN_SIGNING_KEY=/absolute/path/to/existing/release.p12
export T2CAN_SIGNING_PASSWORD_FILE=/absolute/path/to/existing/signing-password.txt
export T2CAN_OUTPUT=/absolute/path/to/Tesla-Unlock/releases/Tesla-Unlock-1.2.1
python3 tools/build.py
python3 tools/package_source.py
```

기존 서명 키를 재사용해야 업데이트 설치할 수 있습니다. 빌드 도구는 키가 없을 때 새 키를 만들지 않고 중단합니다. 개인 키·암호·툴체인·빌드 중간 파일은 소스 ZIP에 포함하지 않습니다. 산출물 이름은 `Tesla-Unlock-1.2.1.apk`와 `Tesla-Unlock-1.2.1-source.zip`이며 실제 크기·해시는 완료 검증 결과를 확인하십시오.

## 소스 구성

- `src/dev/t2can/unlock/`: Activity, 연결 관리자, 프로필·권한·URL 정책, 암호화 저장, 파일 내보내기.
- `assets/dashboard-bridge.js`: 대시보드 뒤로가기·파일 저장·전송 상태 연결.
- `res/`: 기존 아이콘·테마·HTTP 제한 설정.
- `tests/`: 순수 Java 정책·세션 검사, JavaScript 브리지, 로컬 에뮬레이터 검사.
- `tools/`: SDK 빌드·소스 패키징·로컬 테스트 도구.

에뮬레이터 도구는 폐기 가능한 테스트 환경용입니다. 실제 차량·CAN·시리얼 장치에 연결하여 실행하지 마십시오. APK에는 펌웨어 바이너리나 대시보드 사본·모의 데이터가 포함되지 않습니다.


## 1.2.1 retry policy (supersedes earlier no-retry descriptions)

If Android reports onUnavailable during automatic Wi-Fi connection, the app immediately releases the old request and makes one fresh request. This applies to both launch auto-connect and the Connect button. Each sequence has at most two attempts. Each attempt has a 40-second overall deadline and a 35-second Android request timeout; the sequence may therefore take roughly 80 seconds. Other errors (including the overall deadline), manual Wi-Fi mode, and loss of an established connection do not automatically retry. Disconnect/close cancels the sequence. Android does not distinguish approval rejection from other unavailable reasons here, so dismissing the first system request can also cause the one retry and another approval prompt.
