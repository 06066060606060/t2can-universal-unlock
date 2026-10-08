#include "../can_usb_logger_pure.h"
#include <assert.h>
#include <string.h>

int main() {
  CanUsbFrame f = {};
  f.timestampUs = 4294967300ULL;
  f.session = 1; f.sequence = 2; f.id = 0x399; f.dlc = 3;
  f.data[0] = 0x01; f.data[1] = 0xAB; f.data[2] = 0xFF;
  char line[128] = {};
  assert(canUsbFormatFrame(line, sizeof(line), f) > 0);
  assert(strcmp(line, "@CAN,1,2,4294967300,A,0,399,3,01ABFF\n") == 0);
  f.bus = 1; f.flags = 3; f.id = 0x1FFFFFFF; f.dlc = 8;
  assert(canUsbFormatFrame(line, sizeof(line), f) > 0);
  assert(strcmp(line, "@CAN,1,2,4294967300,B,3,1FFFFFFF,8,\n") == 0);
  char small[8] = {};
  assert(canUsbFormatFrame(small, sizeof(small), f) == 0);
  f.flags = 0; // Standard IDs cannot carry 29-bit identifiers.
  assert(canUsbFormatFrame(line, sizeof(line), f) == 0);
  CanUsbQueue<2> queue;
  assert(!queue.observe(f));
  queue.start();
  assert(queue.observe(f)); assert(queue.observe(f));
  assert(!queue.observe(f));
  assert(queue.seen == 3 && queue.queued == 2 && queue.dropped == 1);
  CanUsbFrame received = {};
  assert(queue.pop(received) && received.sequence == 1 && received.session == 1);
  assert(queue.observe(f));
  assert(queue.pop(received) && received.sequence == 2);
  assert(queue.pop(received) && received.sequence == 4);
  assert(!queue.pop(received));
  queue.active = false; assert(!queue.observe(f));
  queue.start(); assert(queue.session == 2 && queue.seen == 0 && queue.count == 0);
}
