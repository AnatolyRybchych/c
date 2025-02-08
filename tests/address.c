#include "test.h"

#include <string.h>

TEST(address_to_string, MC_Address addr, const char *res){
    char buf[64];
    size_t written;

    ASSERT(mc_address_to_string(&addr, sizeof buf, buf, &written) == MCE_OK);
    ASSERT(strlen(buf) == written);
    ASSERT(strcmp(buf, res) == 0);
}