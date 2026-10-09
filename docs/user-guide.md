# Tesla Unlock 1.2.1 사용 안내

Android 8(API26) 이상에서 사용하며 앱 안에서 Wi-Fi 연결을 요청하는 기능은 Android 10(API29) 이상을 대상으로 합니다. 앱 화면 문구는 영어입니다. 설치·빌드 검증과 실기기 검증 범위는 [VALIDATION.md](../VALIDATION.md)를 확인하십시오.

## 설치와 최초 등록

배포 폴더는 저장소의 `releases/Tesla-Unlock-1.2.1/`이며 APK 이름은 `Tesla-Unlock-1.2.1.apk`, 소스는 `Tesla-Unlock-1.2.1-source.zip`입니다. 기존 앱과 같은 서명 키를 유지하여 업데이트 설치하도록 구성했습니다. 설치 파일과 실제 검증 결과를 확인한 뒤 설치하십시오.

1. 휴대폰 Wi-Fi를 켜십시오. 다른 앱에서 셀룰러 인터넷을 사용하려면 모바일 데이터도 켜고 정상 사용 가능한지 확인하십시오.
2. 앱에서 **Devices / connection settings → Devices**를 여십시오.
3. 기존 `T2CAN`의 **Edit** 또는 **Add device**를 선택하십시오.
4. 장치 이름, 실제 Wi-Fi SSID, OPEN/WPA2/WPA3 보안 방식과 해당 암호를 입력하십시오. 앱은 SSID나 암호를 자동으로 추측하지 않습니다.
5. 실제 장치 대시보드 주소를 확인하고 **Save**를 누르십시오. 기본 주소는 `http://192.168.4.1/`입니다.
6. **Connect**를 누르고 Android 권한 및 Wi-Fi 연결 승인 창을 처리하십시오. 승인 창은 기기·OS 정책에 따라 다시 나타날 수 있습니다.

Android 10–12에서는 이 Wi-Fi API를 사용하기 위해 위치 권한과 위치 설정이 필요합니다. Android 12에서는 정확한 위치를 허용해야 합니다. Android 13 이상에서는 Nearby Wi-Fi 권한을 요청합니다. 앱이 위치 좌표를 수집하는 기능은 없습니다.

주소를 변경할 때 HTTP는 `192.168.4.1`만 허용하며 필요한 포트를 지정할 수 있습니다. 다른 사설 IPv4 주소는 신뢰할 수 있는 인증서를 사용하는 HTTPS여야 합니다. 주소에 페이지 경로·query·fragment를 추가하지 마십시오.

## 두 번째 실행부터 자동 연결

장치를 등록하고 최초 Wi-Fi 권한을 승인해 두시면, 앱을 종료했다가 새로 열 때 현재 선택한 장치로 자동 연결 절차를 시작합니다. Android 연결 요청이 실패(onUnavailable)하면 즉시 1회 더 시도합니다. Android 시스템의 AP 연결 승인 화면은 기기 정책에 따라 다시 나타날 수 있습니다.

권한이 없으면 안내만 표시하므로 Connect를 눌러 최초 권한을 승인하십시오. 기기 미등록·수동 전용 빈 SSID 프로필·Android 8/9에서는 자동 AP 연결을 시작하지 않습니다.

백그라운드에서 돌아오기, 권한/파일 선택창 닫기, 화면 회전만으로 연결 요청을 반복하지 않습니다. 이번 실행에서 Disconnect를 누르거나 최종 연결 실패가 표시되면 Connect로 직접 재시도할 수 있습니다. 앱을 완전히 종료한 뒤 다시 열면 자동 시도가 다시 가능합니다.

## 수동 연결과 오류 복구

자동 연결이 지원되지 않거나 실패하면 **Manual connection → Wi-Fi settings**에서 장치 Wi-Fi를 연결하십시오. 앱으로 돌아와 **Manual connection → Check connection**을 누르십시오. 수동 연결에서도 다른 앱의 인터넷 경로는 OS가 선택하며 셀룰러 사용을 보장하지 않습니다.

대시보드를 연 뒤에는 하단 **Connection** 버튼으로 프로필·해제·진단·Android 설정에 접근합니다. 권한을 거부했다면 **App permissions**에서 변경한 뒤 다시 **Connect**를 누르십시오. 잘못된 암호나 SSID는 **Devices → Edit**에서 수정하십시오.

장치 전원이 꺼지거나 Wi-Fi가 끊기면 앱은 기존 대시보드를 닫고 연결 안내를 표시합니다. 장치와 Wi-Fi 상태를 확인한 뒤 명시적으로 다시 연결하십시오. 화면 전환·잠금·절전 후에도 OS가 연결을 해제할 수 있습니다. 앱은 이전 제어 명령이나 업로드를 자동 재전송하지 않습니다.

저장 프로필은 Android Keystore로 암호화합니다. 키 접근 오류가 나타나면 기존 데이터는 덮어쓰지 않습니다. 앱 데이터 초기화는 저장 프로필과 앱 로컬 설정을 지우므로, 초기화 전에 등록 정보를 다시 확보하십시오.

## OTA와 로그 파일

대시보드의 기존 OTA 파일 선택과 로그 다운로드 기능을 사용합니다. 파일 선택·저장은 Android 시스템 창으로 처리하며 로그 파일 하나의 상한은 128 MiB입니다. OTA·저장 작업 중에는 연결을 변경하지 마십시오. 네트워크가 실제로 끊기면 전송이 실패할 수 있으므로 대시보드에서 완료 결과를 확인해야 합니다. 이 안내는 실제 펌웨어 설치를 자동 실행하지 않습니다.

## 인터넷 병행 확인과 진단

대시보드가 열리는 것만으로 다른 앱의 인터넷 성공을 확인할 수는 없습니다. 안전하게 정차한 환경에서 Chrome 등 다른 앱으로 외부 페이지를 새로 요청하고 앱으로 돌아와 대시보드 상태를 확인하십시오. 기기의 기본 인터넷이 다른 Wi-Fi일 수도 있으며, LTE/5G 사용 여부는 별도 확인해야 합니다. Wi-Fi 전용 태블릿이나 모바일 데이터가 꺼진 기기는 셀룰러 인터넷을 사용할 수 없습니다.

문제 보고 시 **Connection → Copy diagnostics**로 현재 결과를 복사할 수 있습니다. 앱은 비밀번호·SSID·대시보드 콘텐츠를 진단에 넣지 않습니다. 기기 모델·OS·연결 모드·실패 절차·다른 앱 인터넷 여부도 함께 기록하십시오. VPN·Private DNS·절전 정책에 따라 결과가 달라질 수 있습니다. 모든 Android 기기의 동일 동작이나 무기한 백그라운드 연결은 보장하지 않습니다.


## 1.2.1 retry policy (supersedes earlier no-retry descriptions)

If Android reports onUnavailable during automatic Wi-Fi connection, the app immediately releases the old request and makes one fresh request. This applies to both launch auto-connect and the Connect button. Each sequence has at most two attempts. Each attempt has a 40-second overall deadline and a 35-second Android request timeout; the sequence may therefore take roughly 80 seconds. Other errors (including the overall deadline), manual Wi-Fi mode, and loss of an established connection do not automatically retry. Disconnect/close cancels the sequence. Android does not distinguish approval rejection from other unavailable reasons here, so dismissing the first system request can also cause the one retry and another approval prompt.
