// T-2CAN V1.0 bench bring-up / read-only bus checker.
//
// This firmware is deliberately LISTEN-ONLY on both buses:
//   CAN A (MCP2515, 16 MHz, P2 connector) : listen-only, 500 kbit/s
//   CAN B (ESP32 TWAI, P1 connector)      : listen-only, 500 kbit/s
// It never transmits CAN frames and never ACKs, so it is suitable for a first
// board/bus check on an isolated bench.
//
// Pins follow the official LILYGO T-2CAN V1.0 pin map:
//   MCP2515: CS 10, RST 9, SCK 12, MISO 13, MOSI 11
//   TWAI:    TX 7, RX 6
//
// Wi-Fi AP: T2CAN-BRINGUP / 12345678  -> http://192.168.4.1

#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <mcp2515.h>
#include "driver/twai.h"

static const int MCP_CS = 10;
static const int MCP_RST = 9;
static const int SPI_SCK = 12;
static const int SPI_MISO = 13;
static const int SPI_MOSI = 11;
static const int TWAI_TX = 7;
static const int TWAI_RX = 6;

MCP2515 mcp(MCP_CS, 10000000, &SPI);
WebServer server(80);

static bool mcpReady = false;
static bool twaiReady = false;
static uint32_t countA = 0;
static uint32_t countB = 0;
static uint32_t lastA = 0;
static uint32_t lastB = 0;
static uint32_t overflowA = 0;
static uint32_t overflowB = 0;

struct FrameRec {
  uint32_t ms;
  uint32_t id;
  bool ext;
  uint8_t dlc;
  uint8_t data[8];
};

static const uint8_t RING_SIZE = 16;
static FrameRec ringA[RING_SIZE];
static FrameRec ringB[RING_SIZE];
static uint8_t headA = 0, headB = 0;
static uint8_t sizeA = 0, sizeB = 0;

static void pushFrame(FrameRec *ring, uint8_t &head, uint8_t &size,
                      uint32_t now, uint32_t id, bool ext, uint8_t dlc,
                      const uint8_t *data) {
  FrameRec &r = ring[head];
  r.ms = now;
  r.id = id;
  r.ext = ext;
  r.dlc = dlc > 8 ? 8 : dlc;
  for (uint8_t i = 0; i < 8; i++) r.data[i] = i < r.dlc ? data[i] : 0;
  head = (head + 1) % RING_SIZE;
  if (size < RING_SIZE) size++;
}

static String hexData(const uint8_t *d, uint8_t n) {
  String s;
  for (uint8_t i = 0; i < n; i++) {
    if (i) s += ' ';
    char b[3];
    snprintf(b, sizeof(b), "%02X", d[i]);
    s += b;
  }
  return s;
}

static String framesJson(const FrameRec *ring, uint8_t head, uint8_t size,
                         uint32_t now) {
  String s = "[";
  for (uint8_t n = 0; n < size; n++) {
    uint8_t idx = (uint8_t)((head + RING_SIZE - 1 - n) % RING_SIZE);
    const FrameRec &r = ring[idx];
    if (n) s += ',';
    s += "{\"ageMs\":" + String(now - r.ms) +
         ",\"id\":\"0x" + String(r.id, HEX) + "\"" +
         ",\"ext\":" + String(r.ext ? "true" : "false") +
         ",\"dlc\":" + String(r.dlc) +
         ",\"data\":\"" + hexData(r.data, r.dlc) + "\"}";
  }
  s += "]";
  return s;
}

static String statusJson() {
  uint32_t now = millis();
  String s = "{";
  s += "\"mcpReady\":" + String(mcpReady ? "true" : "false");
  s += ",\"twaiReady\":" + String(twaiReady ? "true" : "false");
  s += ",\"countA\":" + String(countA);
  s += ",\"countB\":" + String(countB);
  s += ",\"lastAAgeMs\":" + String(countA ? (now - lastA) : 0xFFFFFFFFu);
  s += ",\"lastBAgeMs\":" + String(countB ? (now - lastB) : 0xFFFFFFFFu);
  s += ",\"overflowA\":" + String(overflowA);
  s += ",\"overflowB\":" + String(overflowB);
  s += ",\"framesA\":" + framesJson(ringA, headA, sizeA, now);
  s += ",\"framesB\":" + framesJson(ringB, headB, sizeB, now);
  s += "}";
  return s;
}

static const char PAGE[] PROGMEM = R"HTML(<!doctype html>
<html lang="zh-CN"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">
<meta name="color-scheme" content="dark"><title>T-2CAN 板级自检</title>
<style>
*{box-sizing:border-box}body{margin:0;background:#0d131a;color:#eef4f7;font:15px/1.5 system-ui,-apple-system,"Microsoft YaHei",sans-serif}
main{max-width:760px;margin:auto;padding:22px 16px 40px}.sub{color:#93a5b3;font-size:13px;margin:4px 0 18px}
.badge{display:inline-block;padding:4px 10px;border-radius:999px;font-size:12px;background:#29323b;color:#aeb9c6;margin-left:6px}
.ok{background:#1f3a29;color:#baf5c8}.bad{background:#3a2323;color:#f5b8b8}
.card{background:#151f29;border:1px solid #293744;border-radius:14px;padding:14px;margin-bottom:12px}
h1{font-size:24px;margin:0 0 2px}.meta{font-size:12px;color:#8fa2b0;margin-top:6px}
table{width:100%;border-collapse:collapse;font-size:12px;margin-top:8px}
th,td{padding:5px 4px;border-bottom:1px solid #26313c;text-align:left;white-space:nowrap}
th{color:#8fa2b0;font-weight:600}td{font-family:ui-monospace,monospace;color:#c9d5df}
.note{color:#8fa2b0;font-size:12px;margin-top:8px}
</style></head><body><main>
<h1>T-2CAN 板级自检</h1>
<div class="sub">只监听 · 500 kbit/s · 不发送任何 CAN 帧</div>
<div class="card"><div>CAN A (MCP2515 / P2) <span id="ba" class="badge">—</span></div>
<div class="meta" id="ma">—</div><table><thead><tr><th>ID</th><th>DLC</th><th>数据</th><th>时间</th></tr></thead><tbody id="ta"></tbody></table></div>
<div class="card"><div>CAN B (TWAI / P1) <span id="bb" class="badge">—</span></div>
<div class="meta" id="mb">—</div><table><thead><tr><th>ID</th><th>DLC</th><th>数据</th><th>时间</th></tr></thead><tbody id="tb"></tbody></table></div>
<div class="card"><div>说明</div><p class="note">P2 = CAN A（MCP2515，16 MHz），P1 = CAN B（TWAI）。<br>
本固件仅用于台架/板级验证：不注入、不 ACK、不发送。<br>
如果某一路上电后一直无帧，检查接线、终端电阻（总线两端各 120 Ω）和 TWAI 的 RX 引脚标注。</p></div>
<p class="note" id="err"></p>
</main><script>
const $=id=>document.getElementById(id);
function row(f){return '<tr><td>'+f.id+'</td><td>'+f.dlc+'</td><td>'+f.data+'</td><td>'+f.ageMs+' ms</td></tr>'}
function render(s){
  $('ba').textContent=s.mcpReady?'已初始化':'初始化失败'; $('ba').className='badge '+(s.mcpReady?'ok':'bad');
  $('bb').textContent=s.twaiReady?'已初始化':'初始化失败'; $('bb').className='badge '+(s.twaiReady?'ok':'bad');
  $('ma').textContent='收到 '+s.countA+' 帧 · 最后 '+(s.countA? s.lastAAgeMs+' ms 前':'无');
  $('mb').textContent='收到 '+s.countB+' 帧 · 最后 '+(s.countB? s.lastBAgeMs+' ms 前':'无');
  $('ta').innerHTML=s.framesA.map(row).join('')||'<tr><td colspan="4">等待 CAN A 数据…</td></tr>';
  $('tb').innerHTML=s.framesB.map(row).join('')||'<tr><td colspan="4">等待 CAN B 数据…</td></tr>';
}
async function poll(){try{const r=await fetch('/api/status',{cache:'no-store'});if(!r.ok)throw Error();render(await r.json());$('err').textContent=''}catch(e){$('err').textContent='连接中断，正在重试…'}}
poll();setInterval(poll,1000);
</script></body></html>)HTML";

static void initCanA() {
  pinMode(MCP_RST, OUTPUT);
  digitalWrite(MCP_RST, LOW);
  delay(10);
  digitalWrite(MCP_RST, HIGH);
  delay(20);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, MCP_CS);
  mcpReady = mcp.reset() == MCP2515::ERROR_OK &&
             mcp.setBitrate(CAN_500KBPS, MCP_16MHZ) == MCP2515::ERROR_OK &&
             mcp.setListenOnlyMode() == MCP2515::ERROR_OK;
}

static void initCanB() {
  twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(
      (gpio_num_t)TWAI_TX, (gpio_num_t)TWAI_RX, TWAI_MODE_LISTEN_ONLY);
  g.tx_queue_len = 0;
  g.rx_queue_len = 64;
  twai_timing_config_t t = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  twaiReady = twai_driver_install(&g, &t, &f) == ESP_OK &&
              twai_start() == ESP_OK;
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== T-2CAN V1.0 bench bring-up (listen-only) ===");

  initCanA();
  initCanB();
  Serial.printf("CAN A MCP2515 (16MHz listen-only): %s\n", mcpReady ? "OK" : "FAIL");
  Serial.printf("CAN B TWAI (listen-only):          %s\n", twaiReady ? "OK" : "FAIL");

  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP("T2CAN-BRINGUP", "12345678")) Serial.println("AP start failed");
  server.on("/", HTTP_GET, [] {
    server.sendHeader("Cache-Control", "no-store");
    server.send_P(200, "text/html; charset=utf-8", PAGE);
  });
  server.on("/api/status", HTTP_GET, [] {
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", statusJson());
  });
  server.begin();
  Serial.printf("Dashboard: http://%s/\n", WiFi.softAPIP().toString().c_str());
}

void loop() {
  server.handleClient();

  struct can_frame f;
  if (mcpReady) {
    for (int i = 0; i < 32; i++) {
      if (mcp.readMessage(&f) != MCP2515::ERROR_OK) break;
      const uint32_t now = millis();
      countA++;
      lastA = now;
      const bool ext = (f.can_id & CAN_EFF_FLAG) != 0;
      pushFrame(ringA, headA, sizeA, now, f.can_id & 0x1FFFFFFF, ext, f.can_dlc, f.data);
      Serial.printf("A 0x%03lX dlc=%u %s\n", (unsigned long)(f.can_id & 0x1FFFFFFF),
                    f.can_dlc, hexData(f.data, f.can_dlc).c_str());
    }
  }

  twai_message_t m;
  if (twaiReady) {
    for (int i = 0; i < 32; i++) {
      if (twai_receive(&m, 0) != ESP_OK) break;
      const uint32_t now = millis();
      countB++;
      lastB = now;
      pushFrame(ringB, headB, sizeB, now, m.identifier, m.extd, m.data_length_code, m.data);
      Serial.printf("B 0x%03lX dlc=%u %s\n", (unsigned long)m.identifier,
                    m.data_length_code, hexData(m.data, m.data_length_code).c_str());
    }
  }
  delay(1);
}
