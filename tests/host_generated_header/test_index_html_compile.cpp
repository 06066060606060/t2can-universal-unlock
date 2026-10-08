#include "index_html.h"

static_assert(INDEX_HTML_GZ_LEN > 0, "embedded dashboard must not be empty");
static_assert(sizeof(INDEX_HTML_GZ) == INDEX_HTML_GZ_LEN,
              "generated length must match the byte array");

int main() {
  return INDEX_HTML_GZ[0] == 0x1f && INDEX_HTML_GZ[1] == 0x8b ? 0 : 1;
}
