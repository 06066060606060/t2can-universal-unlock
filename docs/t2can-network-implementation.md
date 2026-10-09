# T2CAN Android 1.2.1 네트워크 구현

이 문서는 `dev.t2can.unlock` 1.2.1(versionCode 5)의 앱 계층 구현을 설명합니다. Java WebView 셸을 유지하며 minSdk 26, target/compile SDK 36입니다. 기존 서명 키를 재사용하고, 펌웨어·대시보드 HTML·장치 API는 수정하지 않습니다. 실제 검증 결과는 [VALIDATION.md](../VALIDATION.md)를 기준으로 확인하십시오.

## 앱 실행 자동 연결 (1.2.1)

2026-10-09 사용자의 추가 요청으로 최초 명세의 실행 정책을 확장했습니다. `LaunchConnectionPolicy`는 선택 프로필이 자동 연결에 유효하고 API29 이상이며 Wi-Fi 권한이 이미 있으면 최초 `onResume()`에서 `startConnection(true)`를 한 번 호출하도록 결정합니다. 권한이 없으면 설명만 표시하고 명시적인 Connect에서 권한을 요청합니다.

소비 상태는 해당 Activity 실행 동안 유지하고 `onRetainNonConfigurationInstance()`로 구성 변경에 따른 재생성에도 전달합니다. 프로세스가 종료된 뒤의 새 실행에서는 새 정책 객체를 만들어 다시 한 번 시도합니다. 사용자 연결·해제도 자동 시도를 소비하며, 백그라운드 복귀·파일 선택 복귀만으로 새 요청이 생성되지 않습니다. 1.2.1에서는 연결 관리자가 onUnavailable에 한해 즉시 1회 재시도합니다. 암호화 저장 형식과 WebView 라우팅은 변경하지 않습니다.

## 연결과 라우팅

`ConnectionManager`가 네트워크 요청·콜백·프로세스 바인딩을 단독 소유합니다. `ConnectionSession`의 세대 번호로 종료되거나 교체된 요청의 콜백과 HTTP 결과를 거부합니다. 새 연결은 이전 세션을 정리한 후 시작합니다.

자동 모드는 API29 이상에서 정확한 SSID와 OPEN/WPA2/WPA3 설정으로 `WifiNetworkSpecifier`를 만듭니다. Wi-Fi transport를 지정하고 `NET_CAPABILITY_INTERNET`을 제거한 요청을 Android에 제출합니다. 시스템 연결 승인은 OS가 관리하며 승인 생략을 보장하지 않습니다. 이는 Android의 [Wi-Fi Network Request API](https://developer.android.com/develop/connectivity/wifi/wifi-bootstrap)를 사용한 로컬 연결 방식입니다.

기존 Wi-Fi를 재사용하려면 자동 모드에서는 읽을 수 있는 SSID가 선택 프로필과 같아야 합니다. SSID가 가려져 있으면 다른 장치를 추측하지 않고 정확한 specifier 요청으로 진행합니다. 수동 모드는 사용자가 Android 설정에서 연결한 Wi-Fi의 기존 HTTP 응답을 확인하며 앱이 AP 연결을 강제하지 않습니다.

네이티브 검사는 받은 `Network.openConnection()`으로 기존 `/api/profile/status`에 접근합니다. HTTP 200과 JSON의 `setupMode`, `profile` 필드를 확인합니다. 리다이렉트를 따르지 않으며 응답 크기·읽기 시간·시도 횟수를 제한합니다. 최대 3회 HTTP 확인은 OS 승인 창을 반복 요청하는 동작과 구분됩니다. 전체 연결에는 제한 시간이 있습니다.

HTTP 성공 이후 `bindProcessToNetwork()` 결과와 실제 바인딩을 확인하고, 그 다음에 새 WebView를 생성하여 대시보드를 엽니다. 네이티브 HTTP 성공과 WebView 로드 성공은 서로 다른 결과로 기록합니다. 프로세스 바인딩은 이 앱에 적용되며 다른 앱을 셀룰러로 강제하지 않습니다. [ConnectivityManager API](https://developer.android.com/reference/android/net/ConnectivityManager)의 프로세스 바인딩·기본 네트워크 관측 기능을 사용합니다.

네트워크 상실·해제·프로필 변경·Activity 종료 시 콜백과 바인딩을 해제하고 WebView·파일 브리지를 닫습니다. 사용자는 다시 `Connect`를 누릅니다. 이전 페이지의 소켓이나 전송을 다른 프로필에서 재사용하지 않으며 POST 요청을 자동 재전송하지 않습니다. 네트워크가 실제로 끊기면 진행 중 전송도 완료되지 않을 수 있습니다.

## 권한과 지원 범위

| Android API | 연결 모드 | 앱 요청 권한 |
| --- | --- | --- |
| 26–28 | 수동 연결 | Wi-Fi 연결용 런타임 권한 없음 |
| 29–30 | 자동·수동 | 자동 모드에서 FINE_LOCATION |
| 31–32 | 자동·수동 | 자동 모드에서 FINE_LOCATION과 COARSE_LOCATION 함께 요청; 정확한 위치 권한 필요 |
| 33 이상, 현재 target36 | 자동·수동 | 자동 모드에서 NEARBY_WIFI_DEVICES |

API29–32에서는 자동 연결에 Android 위치 설정도 필요합니다. 앱이 위치 좌표를 수집하기 위한 기능은 없습니다. API31–32의 동시 요청은 FINE 단독 요청이 무시되는 환경을 피하기 위한 처리입니다. [위치 권한 런타임 요청](https://developer.android.com/develop/sensors-and-location/location/permissions/runtime)과 [Nearby Wi-Fi 권한](https://developer.android.com/develop/connectivity/wifi/wifi-permissions)을 기준으로 합니다.

현재 target36을 유지하므로 `ACCESS_LOCAL_NETWORK`는 선언하지 않습니다. target37 전환은 별도 권한·실기기 검증이 필요합니다. 듀얼 STA 지원 여부는 Wi-Fi와 셀룰러 병행 가능 여부를 판단하는 조건으로 사용하지 않습니다. FGS·VPN·기기 전체 라우팅 변경은 구현하지 않습니다.

## 프로필과 URL 제한

`DeviceProfile`은 ID, 이름, 정확한 SSID, 보안 방식, 암호, 대시보드 URL을 보관합니다. SSID는 UTF-8 최대 32바이트이고 자동 모드에서는 필수입니다. 암호를 추측하거나 펌웨어 기본 암호를 앱에 넣지 않습니다. 최초 프로필은 기존 주소 `http://192.168.4.1/`와 빈 SSID의 수동 연결 프로필입니다.

`ProfileRepository`는 최대 20개의 프로필과 선택 ID를 하나의 JSON으로 직렬화한 뒤 Android Keystore AES-256-GCM으로 암호화하여 저장합니다. 암호화 키와 암호문이 없으면 새 기본 프로필로 시작하지만, 기존 암호문이 손상되거나 키를 읽지 못하면 오류를 표시하고 저장 데이터를 덮어쓰지 않습니다. 앱 백업은 비활성화되어 있으며 키를 소스 ZIP에 넣지 않습니다.

`UrlPolicy`와 Network Security Config의 허용 범위는 다음과 같습니다.

- HTTP는 확인된 호스트 `192.168.4.1`에서만 허용하며 사용자가 지정한 유효한 포트를 사용할 수 있습니다.
- 다른 RFC1918 사설 IPv4 주소는 HTTPS만 허용하고 Android가 신뢰하는 인증서가 필요합니다. 인증서 검증을 우회하지 않습니다.
- 등록 URL은 루트 주소만 허용하며 사용자 정보, query, fragment는 받지 않습니다.
- WebView 문서·하위 HTTP 요청은 선택한 scheme/host/port와 같은 origin으로 제한합니다. 프로필 전환 전에 기존 문서를 폐기합니다.

## 기존 기능 연결

`MainActivity`가 새 연결 화면과 기존 WebView 설정을 통합하고 `ConnectionPanel`이 프로필·수동 연결·진단 메뉴를 제공합니다. 앱 소유 UI 문구는 기존처럼 영어입니다.

OTA 파일은 기존 시스템 파일 선택창으로 전달하며 로그 Blob은 `FileExporter`와 `dashboard-bridge.js`를 통해 48 KiB ACK 청크로 전달합니다. 파일 제한은 128 MiB입니다. 저장소 전체 권한이나 `addJavascriptInterface`를 추가하지 않습니다. 뒤로가기·테마·로그 저장 기능을 유지하고, 사용자 주도 연결 메뉴는 진행 중 OTA·파일 작업 여부를 확인합니다. 실제 네트워크 상실은 이 보호와 관계없이 연결을 정리합니다.

## 진단과 남은 검증

진단에는 SDK·제조사·모델·앱 버전, 연결 상태와 시각, 바인딩된 Network, capability·주소/route 개수·Private DNS 상태, 관측된 기본 네트워크, 네이티브 HTTP 결과와 지연, WebView 결과가 포함됩니다. 비밀번호·SSID·대시보드 콘텐츠를 복사하지 않습니다. `observedDefault`는 관측 결과이며 다른 앱의 실제 인터넷 성공 증거가 아닙니다.

실기기에서 AP 연결, OEM 승인 UI, 셀룰러 병행, VPN/Private DNS, 화면 잠금·앱 전환, 실제 OTA·파일 전송을 별도로 확인해야 합니다. 무기한 백그라운드 연결을 보장하지 않습니다. 에뮬레이터나 호스트 검사 성공을 Wi-Fi와 셀룰러 병행 성공으로 보고하지 않습니다. 이번 변경만으로 실제 장치 플래시·CAN 접근·차량 시험이 승인되지는 않습니다.


## 1.2.1 retry policy (supersedes earlier no-retry descriptions)

If Android reports onUnavailable during automatic Wi-Fi connection, the app immediately releases the old request and makes one fresh request. This applies to both launch auto-connect and the Connect button. Each sequence has at most two attempts. Each attempt has a 40-second overall deadline and a 35-second Android request timeout; the sequence may therefore take roughly 80 seconds. Other errors (including the overall deadline), manual Wi-Fi mode, and loss of an established connection do not automatically retry. Disconnect/close cancels the sequence. Android does not distinguish approval rejection from other unavailable reasons here, so dismissing the first system request can also cause the one retry and another approval prompt.
