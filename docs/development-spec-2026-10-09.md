# T-2CAN Android 앱 — 로컬 전용 Wi-Fi + 셀룰러 인터넷 병행 사용 개발 명세서

> **버전:** 1.0 (Codex 전달용)  
> **작성일:** 2026-10-09  
> **범위:** 기존 Android 대시보드 접속 앱 수정  
> **우선순위:** P0 — 네트워크 연결 안정성 / 기존 기능 보존  
> **근거:** Android 공식 개발자 문서(문서 하단 링크 참조)  
> **주의:** 현재 앱 소스코드와 실제 T-2CAN AP 설정을 이 문서 작성 시점에 확인하지 못했다. 아래의 프로젝트 구조, SSID, 비밀번호, 포트, IP 주소는 **추측하여 하드코딩하지 말 것**.

---

## 0. Codex에게 가장 먼저 전달할 핵심 지시

기존 **T-2CAN 대시보드용 Android 앱**에, 앱 자체가 T-2CAN의 **인터넷이 없는 Wi-Fi AP(local-only Wi-Fi)** 연결을 요청하고, 그 Wi-Fi를 **T-2CAN 앱의 대시보드 통신에만 사용**하는 기능을 구현한다. 동시에 **다른 Android 앱의 일반 인터넷은 운영체제가 선택한 기본 인터넷 연결(가능하면 LTE/5G)** 을 계속 사용하게 한다.

1. **신규 앱을 재작성하지 말고 기존 앱 소스를 먼저 분석한다.** 앱이 `WebView` 기반인지, 외부 브라우저/PWA/TWA/Capacitor/Cordova 등인지 확인하고 변경 범위를 결정한다.
2. 기본 설계는 **Android 10(API 29)+의 `WifiNetworkSpecifier` + `ConnectivityManager.requestNetwork()` + `NetworkCallback` + 필요한 경우 `bindProcessToNetwork()`** 이다.
3. **T-2CAN 펌웨어 및 대시보드 HTML/JS/API는 원칙적으로 수정하지 않는다.** 앱 계층만으로 먼저 구현하고, 불가피한 변경은 근거와 별도 승인 사항으로 제시한다.
4. **모든 Android 기기에서 100% 동일 동작을 보장한다고 주장하지 않는다.** 버전·OEM·권한·네트워크 정책별 정상 모드와 호환 모드를 준비한다.
5. 실제 소스의 빌드/동작을 확인할 때까지 구현 완료라고 보고하지 않는다. 빌드 미실행·실기기 미검증은 명시한다.

---

## 1. 배경 및 사용자 요구사항

### 1.1 현재 상황

- T-2CAN은 자체 Wi-Fi AP로 로컬 웹 대시보드를 제공한다.
- 해당 AP는 일반 인터넷 접속을 제공하지 않는다.
- 기존 Android 대시보드 접속 앱은 이미 제작되어 있으며 **그 앱을 개선하는 작업**이다.
- Android Wi-Fi 설정에서 T-2CAN을 수동 연결할 때, 제조사/설정에 따라 다른 앱의 인터넷 접속에 문제가 발생할 수 있다.
- iOS의 Wi-Fi Assist 유사 사용 경험이 목적이지만, Android에서는 **기기 차원의 전체 네트워크 정책 변경이 아닌 앱 전용 로컬 네트워크 요청**으로 접근한다.

### 1.2 목표 아키텍처

```text
                  ┌───────────────────────────────┐
                  │       Android Smartphone      │
                  │ Wi-Fi ON + Mobile Data ON     │
                  └──────────────┬────────────────┘
                                 │
             ┌───────────────────┴──────────────────┐
             │                                      │
   T-2CAN Android App                         Other Apps
   (WebView / native)                          (Chrome, etc.)
             │                                      │
   app-scoped local-only Wi-Fi                OS default network
   via WifiNetworkSpecifier                   (LTE / 5G if valid)
             │                                      │
       T-2CAN SoftAP                           Public Internet
       Local Dashboard
```

**중요한 차이:** 앱에서 **자신의 통신만** 특정 `Network`로 라우팅할 수 있다. 일반 앱은 **다른 앱들을 셀룰러로 강제로 지정할 권한이 없다**. 따라서 다른 앱은 **시스템 기본 네트워크**를 사용하고, 그것이 셀룰러로 유지되는지 기기별로 검증해야 한다.

### 1.3 완료 기준(최상위)

- 앱 안에서 T-2CAN 장치를 등록·선택하고 연결 요청할 수 있다.
- 시스템의 첫 접속 승인 창은 허용하되, 매번 Android Wi-Fi 설정을 열도록 요구하지 않는다(지원 기기 한정).
- WebView 대시보드가 T-2CAN 로컬 주소에 정상 접속한다.
- T-2CAN 앱 사용 중 다른 앱에서 LTE/5G 인터넷을 사용할 수 있다(가능한 기기에서 실제 측정).
- T-2CAN 대시보드의 기존 UI, API, OTA, 파일 업로드/다운로드, 상태 실시간 갱신은 회귀되지 않는다.
- 연결 실패 시 앱이 멈추지 않으며 수동 연결 모드로 전환할 수 있다.

---

## 2. 범위 / 비범위

### In Scope

- 기존 앱 네이티브 네트워크 계층 추가·수정.
- 앱 내 T-2CAN 프로필(SSID, 보안 방식, 필요 시 암호, 대시보드 URL) 등록·선택·수정.
- Android 로컬 전용 Wi-Fi 요청과 시스템 승인 처리.
- 앱 전용 라우팅 및 WebView 초기화 순서.
- 연결 실패, Wi-Fi 이탈, 앱 전·후면 전환, 재접속, 권한 취소 복구.
- 버전별 호환성 모드, 진단 화면/로그, 실기기 테스트.

### Out of Scope (별도 요청 없이는 변경 금지)

- T-2CAN 펌웨어, CAN A/B 처리, 차량 제어 동작, Summon 관련 로직.
- 앱과 무관한 다른 앱의 라우팅 정책을 강제로 변경하는 기능.
- `VpnService`를 이용한 전체 기기 트래픽 가로채기 또는 VPN 우회 구현.
- Wi-Fi 테더링/인터넷 공유/NAT 구축.
- 무제한 백그라운드 상시 연결 보장, 제조사 절전 정책 무력화.
- 디자인 전체 리뉴얼. 연결 관리 UI는 최소 수정.

---

## 3. 지원 전략 및 기술적 한계

| 환경 | 앱에서 AP 연결 요청 | 앱 전용 통신 라우팅 | 기대 동작 / 주의점 |
|---|---|---|---|
| Android 10–11 (API 29–30) | `WifiNetworkSpecifier` 가능 | `Network` 지정 및 프로세스 바인딩 가능 | 로컬 Wi-Fi + 셀룰러 동시 사용은 **실기기 검증 필수**. 시스템이 현재 Wi-Fi를 전환할 수 있음 |
| Android 12–12L (API 31–32) | 가능 | 가능 | 일부 하드웨어에서 동시 STA Wi-Fi 지원. **Wi-Fi + LTE 동시 사용에 듀얼 STA 필수 아님** |
| Android 13–16 (API 33–36) | 가능 | 가능 | `NEARBY_WIFI_DEVICES`와 위치 권한 등 사용 API별 요구사항 검토 |
| Android 17 (API 37) / target SDK 37+ | 가능(권한 준수 필요) | 가능(권한 준수 필요) | 로컬 IP 통신을 위해 `ACCESS_LOCAL_NETWORK` 런타임 권한 검토·요청 |
| Android 9 이하 (API 28-) | `WifiNetworkSpecifier` 사용 불가 | API 23+에서 기존 네트워크 바인딩 가능 | **Android 설정에서 수동 연결하는 호환 모드**. 다른 앱의 셀룰러 사용은 보장 불가 |

- 기존 앱의 `minSdk`는 소스 확인 전 변경하지 않는다. 이미 지원하는 구형 버전을 임의로 제거하지 않는다.
- Android 12의 `WifiManager.isStaConcurrencyForLocalOnlyConnectionsSupported()`는 **Wi-Fi 두 개 동시 연결** 관련 기능 확인용이다. **T-2CAN Wi-Fi + LTE/5G 병행 사용 가능 여부의 판정 조건으로 사용하면 안 된다.**
- Wi-Fi 전용 태블릿, 데이터가 꺼진 폰, 셀룰러 통신 불가 상태에서는 다른 앱의 셀룰러 인터넷 자체가 불가능하다.
- 시스템이 다른 인터넷 Wi-Fi를 기본 네트워크로 유지하는 경우 다른 앱의 경로가 LTE/5G가 아닐 수 있다. 목표는 **다른 앱의 기본 인터넷 연결을 방해하지 않는 것**이다.

---

## 4. 구현 전 기존 소스 감사 (반드시 최초 수행)

Codex는 코드 변경 전에 아래를 조사하고 `docs/t2can-network-audit.md`에 **실제 경로/클래스/설정값**을 정리한다.

1. 언어(Kotlin/Java), 프레임워크(WebView/Compose/React Native/Flutter/TWA/PWA/Capacitor 등), 모듈 구조.
2. `minSdk`, `targetSdk`, `compileSdk`, Android Gradle Plugin, Gradle/JDK, 의존성.
3. Manifest 권한 및 `android:usesCleartextTraffic`, `networkSecurityConfig`.
4. 앱이 대시보드를 여는 방법(`WebView.loadUrl`, 외부 Intent, localhost proxy 등).
5. 실제 T-2CAN Wi-Fi **SSID 규칙**, 인증 방식(WPA2/WPA3/open), 현재 저장 방식, 여러 AP를 구분할 수 있는지.
6. 실제 URL의 **scheme/host/IP/port/path** 및 대시보드 내부 fetch, WebSocket, SSE, Ajax 요청.
7. 기존 WebView 설정, 쿠키, 파일 선택, 파일 저장, Blob/CSV 다운로드, OTA 바이너리 업로드 경로.
8. 연결 상태 업데이트·폴링·재연결 타이머·화면 잠금·뒤로 가기·멀티윈도우 동작.
9. 이미 사용 중인 전역 네트워크 바인딩, DNS, 프록시, VPN 충돌 가능성.
10. 패키지명, 서명, 마이그레이션(기존 사용자 업데이트 설치 가능 여부).

**감사 결과가 이 명세의 전제와 다르면 무조건 현재 소스 근거를 우선한다.** 특히 네이티브 셸이 없다면 Android 연결 API를 호출할 수 있는 계층 추가가 먼저 필요하다.

---

## 5. 권장 구성 요소

폴더명은 제안일 뿐 실제 프로젝트 구조에 맞춰 정한다.

```text
app/
  network/
    T2CanConnectionManager.kt    # 로컬 Wi-Fi 요청/해제 및 상태 머신
    NetworkRouteController.kt    # Network 보관/바인딩/복원
    NetworkPermissionGate.kt     # Android 버전별 권한 판단
    NetworkDiagnostics.kt        # 민감정보 없는 진단 정보
  data/
    T2CanDeviceProfile.kt        # SSID/URL/보안 설정 모델
    DeviceProfileRepository.kt   # 프로필 저장/마이그레이션
  ui/
    ConnectionSheet.kt           # 장치 선택/연결/오류/수동 모드
    DashboardHost.*              # 기존 WebView 또는 대응 컴포넌트
  test/
    ...
```

`T2CanConnectionManager` 공개 계약(언어/패턴은 기존 앱에 맞춤):

- `connect(profile)`: 사용자가 지정한 프로필로 로컬 전용 연결 요청.
- `disconnect()`: 요청 해제, 앱 바인딩 원상복구.
- `retry()`: 사용자 주도 또는 명시적으로 허용한 조건의 재요청.
- `observeState()`: 상태 스트림.
- `activeT2CanNetwork`: 콜백으로 받은 `Network` 객체(내부 관리).
- `getDiagnostics()`: 네트워크 및 앱 접속 상태(자격증명 제외).

**권장:** 연결 요청·라우팅·WebView 라이프사이클을 하나의 Activity 안에서 뒤섞지 말 것. 중복 요청이 발생하지 않도록 단일 소유자(single owner)로 관리.

---

## 6. 로컬 전용 Wi-Fi 요청 (Android 10 이상)

### 6.1 기본 API

- `WifiNetworkSpecifier.Builder()`로 사용자가 선택한 AP를 지정한다.
- 확인된 **정확한 SSID**가 있다면 `.setSsid(ssid)`를 우선한다. 동일 SSID의 복수 장치나 BSSID 변경 시나리오를 검토한다.
- 실제 AP 인증 타입이 WPA2-PSK라면 `.setWpa2Passphrase(...)`; WPA3/open 등은 실물 설정을 확인하고 분기한다.
- `NetworkRequest.Builder()`에는:
  - `.addTransportType(NetworkCapabilities.TRANSPORT_WIFI)`
  - `.removeCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)` — 필수
  - `.setNetworkSpecifier(specifier)`
- `ConnectivityManager.requestNetwork(request, callback)` 호출.
- 시스템이 표시하는 접속 선택/승인 UI를 존중한다. 앱 자체 UI에서 이미 선택했더라도 OS 승인은 사라지지 않을 수 있다.
- 첫 특정 AP 승인 후 **동일 AP에 대한 특정 요청**은 이후 승인 생략이 가능하나, 기기 정책이나 사용자의 네트워크 삭제에 따라 다시 표시될 수 있다. **항상 무승인 자동접속을 보장하지 않는다.**

### 6.2 라이프사이클 처리

- `onAvailable(network)` 수신 후 **WebView를 시작하기 전에** 라우팅/네트워크 준비 여부를 확인한다.
- `onLinkPropertiesChanged`에서 IP/route 등이 준비되었는지 추적한다.
- `onCapabilitiesChanged`에서 해당 `Network`가 Wi-Fi이며 적절한지 확인한다.
- `onLost(network)`: 바인딩 해제, UI 상태 업데이트, 필요 시 제한적 재접속.
- `onUnavailable()`: 사용자 거부·연결 실패 등 상세 원인 추론을 피하고 정확한 안내와 재시도/수동 연결 제공.
- 성공했더라도 **실제 로컬 대시보드 HTTP 접근 성공**까지 확인해야 '대시보드 연결됨'으로 표시한다.
- 화면 회전·Activity 재생성·WebView 재생성 시 동일 요청을 무분별하게 중복하지 않는다.
- 연결 모드 종료 시 `unregisterNetworkCallback()` 호출, 필요 시 `bindProcessToNetwork(null)` 복원. 콜백 등록 여부와 timeout 후 해제 여부를 안전하게 관리한다.

### 6.3 간략 구현 스케치 (설명용, 그대로 붙여넣어 완성으로 처리 금지)

```kotlin
// Android 10+ 전용 분기 내부 — 권한/SSID/인증방식/예외/스레드 처리 생략
val specifier = WifiNetworkSpecifier.Builder()
    .setSsid(profile.ssid)
    // 실제 AP 보안 방식 확인 후 필요한 passphrase 설정
    .build()

val request = NetworkRequest.Builder()
    .addTransportType(NetworkCapabilities.TRANSPORT_WIFI)
    .removeCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
    .setNetworkSpecifier(specifier)
    .build()

val callback = object : ConnectivityManager.NetworkCallback() {
    override fun onAvailable(network: Network) {
        // 1) Network 저장  2) 라우팅 설정  3) 로컬 HTTP 연결 확인
        // 4) 성공 시 기존 WebView 로드 (실제 준비 상태 검증 필수)
    }
    override fun onLost(network: Network) { /* 바인딩 정리, 상태 갱신 */ }
    override fun onUnavailable() { /* 실패/거부 안내, 수동 연결 제시 */ }
}
connectivityManager.requestNetwork(request, callback)
// 실제 종료 경로에서 connectivityManager.unregisterNetworkCallback(callback)
```

이 코드는 구현 방향만 나타낸다. 현재 프로젝트의 threading, WebView, 권한, Android 버전에 맞춰 완전한 코드를 작성할 것.

---

## 7. 앱 전용 라우팅 및 다른 앱 인터넷 보존

### 7.1 기본 방식: WebView 프로세스 바인딩

- T-2CAN `Network`가 유효해지면, **대시보드 WebView가 요청을 시작하기 전에** `ConnectivityManager.bindProcessToNetwork(t2canNetwork)`를 적용한다.
- 반환값 및 `getBoundNetworkForProcess()`를 통해 바인딩 상태를 검증한다.
- T-2CAN WebView 외에 해당 **앱 프로세스의 일반 인터넷 요청도 Wi-Fi로 향할 수 있음**을 명시한다.
- 연결 종료 시 반드시 `bindProcessToNetwork(null)`로 원상복구한다.
- 프로세스 바인딩은 **다른 앱 프로세스에는 적용되지 않는다**. 다른 앱이 LTE/5G를 쓸지 여부는 OS가 관리한다.

### 7.2 네이티브 요청이 섞이는 경우

- 앱 자체에 분석/업데이트/원격 인증 등 **인터넷이 필요한 기능**이 있다면, 프로세스 전체 바인딩이 해당 기능을 방해할 수 있다.
- 필요한 요청은 `Network.openConnection(...)`, `Network.getSocketFactory()` 등 **특정 Network 기반 네이티브 연결**로 분리하거나, 검증된 앱 아키텍처 수준에서 처리한다.
- WebView 네트워킹은 일반 네이티브 소켓 API와 다를 수 있으므로 **단순히 특정 네이티브 HTTP 클라이언트를 바인딩했다고 WebView 전체가 바인딩된다고 가정하지 않는다.**
- WebView HTTP/HTTPS, fetch/XHR, WebSocket, SSE, service worker, Blob/CSV, OTA 통신까지 실기기 확인. WebView가 별도 프로세스 또는 네트워크 서비스로 동작하는 경우 동작 확인 및 대체 설계를 기록한다.
- `DownloadManager` 등 **앱 외부 시스템 서비스로 위임한 로컬 다운로드**는 앱 프로세스 바인딩이 적용되지 않을 수 있다. T-2CAN 로컬 다운로드는 실제 경로에서 검증하고 필요 시 바인딩된 네이티브 스트림 방식으로 처리한다.
- WebView의 외부 인터넷 링크는 시스템 브라우저로 열어 앱의 로컬 전용 경로에 영향을 받지 않게 한다(기존 기능과 사용자 의도 고려).

### 7.3 기본 인터넷 상태 표시 규칙

- `T-2CAN Wi-Fi 연결됨`과 `인터넷 사용 가능`은 서로 다른 상태다.
- `T-2CAN Wi-Fi 연결됨`은 로컬 네트워크 접속/라우팅/대시보드 도달 검증 결과에만 근거한다.
- `인터넷 사용 가능`은 시스템 기본 인터넷 네트워크의 실제 `NET_CAPABILITY_VALIDATED` 여부 등으로 확인한다.
- `셀룰러 사용 중`이라고 표시하려면 실제 해당 기본 네트워크의 `TRANSPORT_CELLULAR` 확인이 있어야 한다.
- 타 앱 트래픽을 직접 강제하거나 관찰할 수 없는 일반 앱에서 **다른 앱 전부가 반드시 셀룰러를 사용한다는 표시를 금지**한다.

---

## 8. 기존 수동 연결 감지 및 구형 기기 호환 모드

앱에 다음 모드를 제공한다.

1. **자동 요청 모드 (권장, API 29+)**: 앱 안에서 T-2CAN 프로필을 선택하고 로컬 전용 Wi-Fi 연결 요청.
2. **수동 연결 모드 (호환)**: 사용자에게 Android 설정에서 T-2CAN Wi-Fi에 접속하도록 안내하고, 앱은 해당 네트워크 존재 및 대시보드 접근을 확인한 후 기존처럼 동작.

구현 원칙:

- Android Wi-Fi 전체 켜기/끄기를 앱이 무단 제어하지 않는다. Wi-Fi가 꺼져 있으면 사용자에게 시스템 Wi-Fi 패널/설정을 연다.
- 수동 모드에서 다른 앱 인터넷 경로가 셀룰러인지 **보장하지 않는다**. 별도의 경고 또는 도움말을 제공한다.
- Android 9 이하 및 일부 OEM의 자동 요청 실패를 '지원되지 않음'으로 방치하지 말고 수동 연결로 접근 가능하게 한다.
- 이미 수동으로 연결된 T-2CAN에 앱이 접속하는 경우, 굳이 새로운 네트워크 요청으로 정상 연결을 끊지 않는다. 실제 `Network`를 안전하게 식별하고 검증한다.
- `WifiNetworkSuggestion`은 인터넷 연결용 AP 제안에 적합하며 이 요구의 **주 구현 API로 사용하지 않는다**.

---

## 9. 권한 및 Android 17 대응

**Manifest에 무조건 모든 권한을 추가하지 말고** 기존 타겟 SDK와 사용 API를 먼저 검사한다.

| 권한/상태 | 검토 사항 |
|---|---|
| `INTERNET` | WebView/HTTP 통신에 필요 |
| `ACCESS_NETWORK_STATE` | `NetworkCapabilities`, `LinkProperties` 조회 |
| `CHANGE_NETWORK_STATE` | `requestNetwork()`를 통해 네트워크를 요청하는 경우 필요 |
| `ACCESS_WIFI_STATE` | 사용 중인 Wi-Fi 정보 조회 API에서 필요한지 확인 |
| `NEARBY_WIFI_DEVICES` | Android 13+, Wi-Fi 연결 관리 사용 API별 필요. 런타임 승인 처리 |
| `ACCESS_FINE_LOCATION` | Android 12L 이하 지원 및 일부 스캔/SSID API에 필요할 수 있음. 사용 API별로 최소 권한 적용 |
| `ACCESS_LOCAL_NETWORK` | **Android 17 실행 + targetSdk 37 이상**에서 로컬 네트워크 접근 시 런타임 권한 대응. targetSdk ≤36 앱에서는 선언/요청하지 말 것(공식 전환 가이드 기준) |
| Wi-Fi ON / 모바일 데이터 ON | 사용자가 기기 설정에서 활성화해야 하며 앱이 임의로 셀룰러 활성화 불가 |

- **런타임 Android 버전**과 **앱 target SDK**의 차이를 구분하여 분기한다.
- `NEARBY_WIFI_DEVICES`의 `neverForLocation`은 실제로 Wi-Fi 정보를 위치 추정에 사용하지 않는 경우에만 설정한다.
- 일부 스캔/SSID 조회 API는 Android 13+에서도 위치 권한을 요구할 수 있다. **SSID 자동 스캔 UI를 만들기 전에** 해당 요구사항을 재검토한다.
- Android 17의 `ACCESS_LOCAL_NETWORK`가 거부되었을 때는 'T-2CAN에 연결되어도 대시보드의 로컬 통신이 제한될 수 있다'고 설명하고 다시 요청/설정 경로를 제공한다.
- 권한 거부가 반복될 때 임의 반복 팝업을 띄우지 않는다.
- `usesCleartextTraffic`/Network Security Configuration은 **실제 기존 T-2CAN HTTP 주소와 현재 설정에 맞춰** 최소 범위로 변경한다. 외부 도메인에 광범위한 HTTP 예외를 만드는 것은 피한다.

---

## 10. 앱 내 UX — 최소 변경

기존 대시보드 디자인을 최대한 보존한다. 연결 상태를 표시할 작은 설정 패널(또는 시트)만 추가한다.

### 10.1 첫 이용

1. **장치 선택/등록** 화면 표시. 실제 SSID/암호/대시보드 주소를 등록하거나 이미 확인된 기존 설정을 불러온다.
2. **연결** 탭 → 필요한 런타임 권한 확인 → Android 시스템 Wi-Fi 선택/승인.
3. `NetworkCallback` 수신 → 로컬 IP/API health check → 대시보드 자동 표시.
4. 다음 실행: 기존 프로필을 선택 상태로 보존. 승인 생략이 가능한 기기에서는 재사용하되 시스템 팝업 가능성을 수용.

### 10.2 접속 화면 상태

| 앱에 표시할 상태 | 기준 | 대표 UX |
|---|---|---|
| `장치 미등록` | 프로필 없음 | `T-2CAN 추가` |
| `연결 준비` | Wi-Fi/권한 확인 전 | `연결` |
| `시스템 승인 대기` | Android Wi-Fi 승인 UI 진행 | 승인 안내만 |
| `Wi-Fi 연결 중` | 네트워크 요청 진행 | 취소 가능 진행 표시 |
| `대시보드 확인 중` | `Network` 가용, 로컬 HTTP 검사 중 | 로딩 |
| `연결됨` | 실제 대시보드 도달 성공 | 기존 WebView 표시 |
| `연결 끊김` | `onLost` 또는 대시보드 접근 실패 | 재연결/수동 연결 |
| `권한 필요` | 필수 권한 거부 | 권한 설명/설정 열기 |
| `호환 모드` | API 미지원 또는 자동 연결 실패 | Android Wi-Fi 설정 안내 |

- 앱은 SSID(또는 별명), Wi-Fi 연결 상태, 대시보드 상태, 기본 인터넷 경로(LTE/5G 등, **실제로 확인한 경우만**)를 구별하여 표시한다.
- 부가 설명/팝업은 간결하게, 대시보드 영역을 불필요하게 차지하지 않도록 한다.
- 여러 T-2CAN 장치 프로필과 잘못된 암호 변경을 지원한다. 기본 실험 범위가 단일 장치여도 확장을 막는 구조는 피한다.

---

## 11. 재접속 / 백그라운드 / 안전한 해제

### 11.1 연결 상태 머신

```text
IDLE
  └─ user Connect → CHECKING_PREREQUISITES
        ├─ invalid → NEEDS_PERMISSION / WIFI_OFF / UNSUPPORTED
        └─ valid → REQUESTING_WIFI
                     ├─ onUnavailable → FAILED → RETRY or MANUAL
                     └─ onAvailable → VERIFYING_NETWORK
                                         ├─ bind/check fail → FAILED
                                         └─ HTTP OK → DASHBOARD_READY
                                                      ├─ onLost → DISCONNECTED
                                                      └─ user Disconnect → IDLE
```

- 네트워크 콜백은 늦게 오거나 중복 발생할 수 있다. 요청마다 ID/generation을 붙여 stale callback을 무시한다.
- **동시 요청 중복 금지** 및 클릭 연타 방지.
- 재접속은 지수 백오프 또는 제한적 간격으로 시행하고, **시스템 승인 창을 자동으로 반복 생성하지 않는다.** 실패가 누적되면 사용자에게 명시적 재시도 제공.
- 재접속 성공 시 대시보드 WebSocket/SSE/폴링이 정상 재구성되는지 확인한다.
- 사용자가 명시적으로 연결 해제한 뒤 앱이 자동으로 재연결하지 않는다.

### 11.2 백그라운드 정책

- **P0:** 앱 전면 사용 시 안정적 연결 및 다른 앱 인터넷 보존.
- **P1:** 앱 전환 후에도 가능한 범위에서 T-2CAN 연결 상태 유지, 앱 복귀 시 정상 복구.
- '백그라운드 전환 후 무기한 연결 유지'는 **별도의 기술 검증 결과가 나오기 전까지 보장하지 않는다.**
- Foreground Service 도입은 **필요성·사용자 가시성·Android 12+ 실행 제약·Android 14+ 서비스 타입·Android 15+ 타임아웃** 검토 후 결정한다. 서비스 하나 추가한다고 OS가 Wi-Fi 연결을 영구 보장하는 것은 아니다.
- 앱 종료/프로세스 사망·기기 절전·Wi-Fi 토글·비행기 모드·화면 잠금 동작을 따로 테스트한다.

---

## 12. 실제 대시보드 통신 및 기능 보존 체크

기존 T-2CAN 앱이 수행하는 기능을 먼저 목록화하고, 그 목록을 회귀 테스트 기준으로 사용한다. 적어도 다음 항목은 확인한다.

- 초기 페이지 HTML/CSS/JS 로드, 캐시·WebView 설정, 로그인/세션(있는 경우).
- 주기적 상태 API 폴링, Ajax/fetch, WebSocket/SSE 등의 실시간 데이터.
- 설정값 조회/저장, 폼 입력 포커스, 뒤로 가기, 화면 회전.
- WebView 외부 링크 이동 처리(앱 바인딩 영향 여부).
- **OTA 펌웨어 업로드 UI, 실제 전송 경로**, 파일 선택기 및 권한.
- **CSV/로그 다운로드와 다운로드 종료 처리**, Blob URL, Android 파일 저장 위치.
- 네트워크 재접속 후 WebView 상태·요청 재시작·중복 제출 방지.
- 로컬 IP가 바뀌었을 경우 안내 및 프로필 수정.
- 디바이스 접근 성공을 표시하기 전에 기존 앱의 **정상 응답 경로를 사용한 health check**가 필요하다. `/health` 같은 새 엔드포인트를 펌웨어에 임의로 가정하거나 추가하지 말 것.

> 실제 차량 제어 로직·CAN 프레임 전송·서먼 관련 펌웨어 동작은 이번 변경과 완전히 분리한다. 테스트는 **차량 정차 상태에서 네트워크/대시보드 접근만** 확인한다.

---

## 13. 장애 대응 / 진단 정보

최소한 다음 케이스를 서로 구분하도록 설계한다.

1. Wi-Fi OFF, 비행기 모드, 모바일 데이터 OFF.
2. SSID 미검출/AP 전원 OFF/거리 문제.
3. 시스템 승인 거부 또는 취소.
4. 잘못된 AP 암호·지원하지 않는 보안 방식.
5. `onUnavailable`, `onLost`, `SecurityException`, 기타 요청 예외.
6. 연결은 성립했지만 DHCP/IP/라우팅/HTTP 연결 실패.
7. WebView에서만 접속 실패하나 네이티브 HTTP는 성공하는 경우.
8. 앱은 T-2CAN에 연결되지만 다른 앱 기본 인터넷은 사용할 수 없는 OEM 제약.
9. VPN, Private DNS, 방화벽/광고 차단기, Data Saver, 배터리 절약 모드 충돌.
10. Android 17 로컬 네트워크 권한 거부.

진단 정보(비밀번호/토큰/차량 식별 정보/민감한 대시보드 콘텐츠는 마스킹):

- Android API/OS 버전, 제조사·모델, app version, `targetSdk`.
- 연결 모드, 상태 머신 현재 상태, `Network` ID, Wi-Fi transport 및 capability, default network의 transport/validated 여부(접근 가능한 범위).
- 요청 시작/성공/해제/실패 시각과 HTTP 체크 지연.
- WebView 및 네이티브 HTTP 요청 결과 구분(상태 코드·오류 종류).
- 사용자 동의 하에 **진단 정보 복사** 또는 로컬 텍스트 내보내기 기능.

---

## 14. 보안 요구사항

- AP 암호·비밀 정보는 로그와 크래시 리포트에서 제외한다. 영구 저장이 필요하다면 Android Keystore 기반 암호화 등 기존 앱과 호환되는 저장 구조를 사용한다.
- Android 시스템의 사용자 연결 승인을 우회하거나 타 앱 권한을 요구하지 않는다.
- WebView의 위험한 설정(`addJavascriptInterface`, 파일 접근, universal access 등)은 기능상 필요성과 기존 구현을 검증하고 불필요하게 개방하지 않는다.
- 외부 사이트에서 임의 앱 URL을 호출해 차량 대시보드에 접근하지 못하도록 URL/호스트 제어를 검토한다.
- HTTP cleartext 예외 범위는 가능한 최소화한다. 인증서 검증 무효화, 전체 인터넷 HTTP 허용 같은 편법을 기본안으로 채택하지 않는다.
- 'AP Wi-Fi 암호를 알고 있음'과 '차량 제어 권한이 검증됨'은 동일하지 않다. 기존 인증·보안 기능을 약화하지 않는다.

---

## 15. 테스트 매트릭스 (실기기 우선)

### 15.1 기기 범위

| 그룹 | 예시 | 테스트 목적 |
|---|---|---|
| Google Pixel | Android 13–17 범위의 보유 가능 기기 | AOSP에 가까운 기준 |
| Samsung Galaxy | 최신 One UI + 구버전 기기 | 제조사 모바일 데이터 전환/배터리 정책 |
| Xiaomi / Redmi / POCO | HyperOS / MIUI | 백그라운드 및 Wi-Fi 정책 |
| Motorola / OPPO 등 | 가능한 실기기 | OEM 호환성 검증 |
| Android 10–11 구형 기기 | 최소 1대 이상 | 이전 Wi-Fi 동시 경로 검증 |
| Android 9 이하 | 가능하다면 | 수동 모드 및 비지원 안내 |
| Wi-Fi 전용 태블릿 | 1대 | 셀룰러 없는 경우 정확한 상태 안내 |

에뮬레이터는 API/권한/상태 머신 테스트에 사용하고, **Wi-Fi와 셀룰러 병행 성공 판정은 실기기에서만** 한다.

### 15.2 필수 수락 시나리오

| ID | 시험 절차 | 통과 조건 |
|---|---|---|
| AC-01 | 셀룰러 ON + Wi-Fi ON, 앱에서 T-2CAN 선택 | 시스템 승인 및 `NetworkCallback` 처리, 대시보드 접속 |
| AC-02 | T-2CAN 대시보드 표시 중 Chrome에서 외부 사이트 열기 | Chrome 실제 인터넷 정상. 가능 시 셀룰러 경로 검증 |
| AC-03 | T-2CAN 표시 중 다른 앱(메신저/스트리밍) 사용 | 앱 인터넷 정상, 재진입 후 대시보드도 정상 |
| AC-04 | 신규 AP 최초 연결 | 시스템 승인 UX 정상, 취소 시 앱 멈춤 없음 |
| AC-05 | 승인했던 특정 AP 재연결 | OS가 허용하면 승인 재요청 생략. 재승인도 정상 처리 |
| AC-06 | 연결 중 AP 전원 OFF/거리 이탈 | `onLost` 반영, 명시적 복구 UI, 무한 팝업 없음 |
| AC-07 | Wi-Fi OFF / 데이터 OFF / 비행기 모드 | 정확한 상태와 설정 안내, 크래시 없음 |
| AC-08 | 잘못된 비밀번호 | 실패 및 편집/재시도 가능 |
| AC-09 | WebView 첫 로드/폴링/WebSocket/SSE | 기존 동작과 일치 |
| AC-10 | OTA/파일 업로드/CSV 다운로드 | 기존 기능 회귀 없음, 전송 완료 결과 확인 |
| AC-11 | 화면 회전, 앱 전환, 화면 잠금 후 복귀 | 요청 중복/콜백 누수 없음, 회복 또는 명확한 오류 |
| AC-12 | Android 17 + targetSdk 37+ | 로컬 권한 승인/거부/철회 대응 |
| AC-13 | 수동 연결 모드로 전환 | Android 설정을 통한 연결 후 앱 접속 가능, 인터넷 경로 제한 고지 |
| AC-14 | 연결 해제 후 다른 일반 앱 인터넷 사용 | 바인딩 해제, 앱 리소스/콜백 정리 |
| AC-15 | VPN/Private DNS 설정된 실기기 | 충돌 감지/오류 고지, 보안 우회 없음 |
| AC-16 | 연속 연결·해제, 앱 강제 종료 후 재실행 | 메모리/요청 누수·프로세스 바인딩 잔존 없음 |

각 테스트 로그에 **기기·OS·targetSdk·연결 모드·결과·재현 절차**를 남긴다.

### 15.3 인터넷 병행 동작의 확증 방법

- 앱 내부에서 T-2CAN 연결 상태와 시스템 기본 인터넷 네트워크를 **별도**로 기록한다.
- Chrome 등 별도 앱에서 일반 인터넷 실제 요청이 성공하는지 확인한다.
- 가능하면 실제 LTE/5G 사용 여부를 기기 네트워크 진단/adb 또는 외부 트래픽 테스트로 검증한다.
- **대시보드 통신 성공만으로 다른 앱의 셀룰러 인터넷 사용이 성공했다고 판단하지 않는다.**
- 실측 실패 기기는 모델/OS/재현 조건을 명시하고 자동 모드 지원 여부를 조정한다.

---

## 16. 구현 순서 / 우선순위

### Phase 0 — 감사 (필수, 변경 전)

- [ ] 기존 앱 컴파일 및 베이스라인 스크린샷/동작 확인.
- [ ] 실제 WebView·대시보드 URL·AP 보안/SSID·다운로드/OTA 경로 확인.
- [ ] 버전/권한/네트워크 코드 감사 문서 작성.
- [ ] 기본 브랜치/백업 생성. 기존 사용자 데이터·앱 서명 보존.

### Phase 1 — P0 네트워크 기능

- [ ] 앱 내 장치 프로필 선택/등록.
- [ ] Android 10+ `WifiNetworkSpecifier` 로컬 전용 요청.
- [ ] `NetworkCallback` + 단일 소유자 상태 머신.
- [ ] WebView 라우팅 검증 및 필요 시 바인딩.
- [ ] 대시보드 준비 후 WebView 표시.
- [ ] 정확한 실패 UX + 수동 연결 모드.
- [ ] 연결 해제/콜백 정리.

### Phase 2 — P0 회귀·인터넷 병행 검증

- [ ] 기존 OTA/다운로드/실시간 연결/설정 페이지 검증.
- [ ] 앱 외부 브라우저 인터넷 동시 사용 실측.
- [ ] Android 10/11, 삼성/픽셀 등 가용 실기기 매트릭스 실행.
- [ ] `Network` 경로/HTTP 체크/콜백 진단 결과 기록.

### Phase 3 — P1 안정화

- [ ] 재접속 백오프, 앱 복귀/백그라운드 대응.
- [ ] Android 13+ 및 17 target 37+ 권한 경로 완성·테스트.
- [ ] 여러 장치 프로필 지원 및 보안 저장.
- [ ] 진단 로그 내보내기, 제조사별 호환 안내.

### Phase 4 — 릴리스

- [ ] 모든 변경 파일/기존 코드 영향 정리.
- [ ] 테스트/빌드 증빙 기록, 실패/미검증 항목 별도 기재.
- [ ] 기존 앱에서 업데이트 설치 테스트(패키지명/서명/프로필 유지).
- [ ] 사용자 설명: 첫 승인, 연결 실패 해결, 구형 폰 제한.

---

## 17. Codex 작업 규칙 (실제 실행 지시)

1. 먼저 리포지토리 구조를 파악하고 **분석 보고서**를 작성한다. 추측한 SSID/IP/HTTP path를 구현하지 않는다.
2. 가능하면 소규모 독립 커밋으로 작업하고, 기존 패키지명/서명·대시보드 기능을 유지한다.
3. 앱이 Android 네이티브 WebView가 아닐 경우 해당 프레임워크의 네이티브 브리지/플러그인 가능성을 검증하고, 최소 변경안을 우선한다.
4. `WifiNetworkSpecifier` 요청, 권한, 연결 상태, WebView 라우팅, 실패 복구를 각각 테스트 가능한 모듈로 분리한다.
5. UI는 **장치 선택/연결/상태/수동 모드** 위주로만 추가하고 불필요한 설정·문구를 늘리지 않는다.
6. 빌드가 불가능하면 정확히 어떤 SDK/Gradle/JDK/기기/서명 파일이 없어 막혔는지 보고한다. 빌드 성공을 추정하지 않는다.
7. **다른 앱의 LTE/5G 동시 사용**은 에뮬레이터 단독으로 검증할 수 없다. 반드시 `NEEDS_DEVICE_TEST`로 표시한다.
8. 차량 관련 펌웨어/CAN 제어·서먼 로직은 요청 범위에서 제외한다.
9. 권한·API에 관해 현재 공식 Android 문서를 다시 확인하고 최신 target SDK의 차이를 명시한다.
10. 아래 산출물을 최종 전달한다.

### 필수 산출물

- 수정된 Android 앱 **전체 소스코드**(기존 프로젝트 구조 유지).
- 변경 파일 목록 + 변경 이유.
- `docs/t2can-network-audit.md`: 기존 앱 구조 및 확정된 SSID/URL/통신 방식.
- `docs/t2can-network-implementation.md`: 실제 선택한 API와 각 Android 버전에서의 라우팅·권한 처리.
- `docs/t2can-network-test-results.md`: 기기별 수락 시험 결과/미검증 영역.
- APK/AAB 등 빌드 결과물(**실제로 빌드된 경우에만**).
- 짧은 사용자 안내: 최초 접속, 수동 모드, 인터넷 병행 사용의 기기별 제약.

### Codex 최종 보고 형식

```text
1) Existing code audit: 실제 구조 / 현재 동작 / 확인된 AP 설정
2) Implemented changes: 파일별 변경 이유
3) Android compatibility: 지원 API / fallback / 권한
4) Build and tests: 실행 명령, 성공·실패 결과
5) Device-validation status: 확인 기기 / 미검증 기기
6) Known limitations: OEM, background, WebView, OTA/download
7) Next actions: 사용자 실기기 테스트 절차
```

---

## 18. 공식 근거 및 확인할 API 문서

1. [Android — Wi-Fi Network Request API for peer-to-peer connectivity](https://developer.android.com/develop/connectivity/wifi/wifi-bootstrap)  
   `WifiNetworkSpecifier`, `removeCapability(NET_CAPABILITY_INTERNET)`, 시스템 사용자 승인 및 특정 AP 재연결 승인 생략 규칙.
2. [Android — Wi-Fi infrastructure overview](https://developer.android.com/develop/connectivity/wifi/wifi-infrastructure)  
   Network request API는 로컬 peer device 용도이며 시스템/다른 앱의 인터넷 네트워크와 구별됨.
3. [Android — WifiNetworkSpecifier reference](https://developer.android.com/reference/android/net/wifi/WifiNetworkSpecifier)  
   API 29+, 일부 기기 동시 STA Wi-Fi 동작/API 31+ 관련 조건.
4. [Android — ConnectivityManager reference](https://developer.android.com/reference/android/net/ConnectivityManager)  
   `requestNetwork()`, `bindProcessToNetwork()`, `unregisterNetworkCallback()`, 권한/콜백 생명주기.
5. [Android — Request permission to access nearby Wi-Fi devices](https://developer.android.com/develop/connectivity/wifi/wifi-permissions)  
   `NEARBY_WIFI_DEVICES` 및 구형 Android 위치 권한.
6. [Android — Local network permission](https://developer.android.com/privacy-and-security/local-network-permission)  
   Android 17 / target SDK 37+의 `ACCESS_LOCAL_NETWORK`.
7. [Android — WifiManager reference](https://developer.android.com/reference/android/net/wifi/WifiManager)  
   `isStaConcurrencyForLocalOnlyConnectionsSupported()` 의미.
8. [Android — Network security configuration](https://developer.android.com/privacy-and-security/security-config)  
   로컬 HTTP cleartext 허용 범위 최소화.
9. [Android — Foreground service restrictions](https://developer.android.com/develop/background-work/services/fgs/restrictions-bg-start)  
   Android 12+ 백그라운드에서 FGS 실행 제한.
10. [Android — Foreground service timeouts](https://developer.android.com/develop/background-work/services/fgs/timeout)  
    Android 15+ 특정 FGS 타입의 시간 제한.

---

## 19. 최종 개발 목표 한 문장

> **T-2CAN 앱에서 Wi-Fi를 선택·연결하면, 해당 앱의 웹 대시보드는 T-2CAN 로컬 Wi-Fi로 통신하고, 다른 앱은 가능한 한 LTE/5G 등의 시스템 기본 인터넷을 정상 사용하도록 한다. 지원이 불완전한 Android 기기에서는 기능을 과장하지 않고 수동 연결 모드를 제공한다.**
