#include <cassert>
#include <cstring>
#include "../nag_mode_h_variant_pure.h"
int main() {
 assert(nagModeHDefaultVariantPure()==1u);
 for(unsigned v=0;v<256;v++) assert(nagModeHVariantValidPure(v)==(v==1));
 assert(strcmp(nagModeHVariantLabelPure(1),"Mode H")==0);
}
