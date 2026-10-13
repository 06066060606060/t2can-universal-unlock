# Home dashboard fixes — 13 October 2026

The reported symptoms were reproduced in a browser with mocked firmware API
responses: the supported Nag Killer switch became disabled after loading its
configuration, all three CAN buses showed WAITING despite fresh traffic, and
PARTY occupied an implicit grid cell below the other cards.

- Profile initialization writes `window.__tmrNagSupported`, while the old NAG
  renderer read an undefined `window.__t2NagSupported`. Capability readers now
  use the initialized TMR variables. Both NAG switches keep the same server
  state; a failed update restores the displayed configuration and reports the
  error. If configuration has not loaded, a user change first reloads it.
- HOME now reads the firmware's `bodyTrafficSeen`, `bodyTrafficOnline`,
  `bodyTrafficAgeMs` fields and their CHASSIS/PARTY equivalents. The compact API
  also exposes normalized controller state/readiness for all three badges.
- The CSS had a nested `<style>` tag and a final grid definition that omitted
  the `canc` area. Style elements are closed correctly, the three CAN cards
  share one row, and R79 spans the complete row beneath them.
- GET requests have a four-second timeout covering headers and JSON parsing.
  Connectivity is updated after a complete valid JSON response. A stalled
  snapshot releases its busy guard so subsequent polling can resume.
- NAG TSL9 route labels use the API's `tsl9EffectiveRoute`; related capability
  readers and the Legacy HW3 display recognize the current fixed topology.

`index_html.h` was regenerated from the editable dashboard. Its gzip payload is
78,784 bytes and was verified against the current minified source.

Validation:

- `node tests/dashboard_request_test.js`
- `node tests/can_performance_dashboard_test.js`
- `node tests/dashboard_home_browser_test.js`
- `node tests/dashboard_home_browser_test.js --embedded`

The browser test requires Playwright and a local Edge installation (or a
compatible channel selected through `DASHBOARD_BROWSER_CHANNEL`). It mocks all
device requests and checks the complete page scripts, Home/Settings switches,
failed saves, missing configuration, LIVE/NO DATA/WAITING, controller badges,
320/360/390/520/800 px layouts, and recovery after a stalled snapshot.

Screenshots and the corrected `-Os` build are stored under
`.build/dashboard-fix/`. This build uses the same generic ESP32-S3 / QIO / 16 MB
flash / OPI PSRAM / 3 MB application partition profile as the earlier compiler
comparison. It is not hardware detection. The earlier comparison binaries
predate these dashboard fixes.

The corrected build passed with Arduino-ESP32 3.3.12: 1,529,383 bytes program
storage and 73,696 bytes static RAM. The sketch and local driver's `-Os` flags
were verified, and the staged firmware sources matched the workspace after
compilation. The application image is `Os/TMR_Dashboard.ino.bin`; build details
and its SHA-256 hash are in `build-report.json`. Only the application image is
intended for dashboard OTA.

The physical board could not be reached from this PC. No firmware was uploaded
and actual CAN activity or wireless reconnection was not measured.
