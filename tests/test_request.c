#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "../src/request.h"

static void rejected(const char* text) { Request r; assert(!parse_request(text, strlen(text), &r)); }
int main(void) {
    Request r; int32_t amount = -1;
    const char* good = "# UTF-8 config\r\n id = camp-999_a \r\n target=999\r\n";
    assert(parse_request(good, strlen(good), &r)); assert(!strcmp(r.id, "camp-999_a")); assert(r.target == 999);
    { const char* bom = "\xef\xbb\xbfid=x\ntarget=80"; assert(parse_request(bom, strlen(bom), &r)); }
    rejected("target=999"); rejected("id=x"); rejected("id=../x\ntarget=999");
    rejected("id=a/b\ntarget=999"); rejected("id=x\ntarget=-1");
    rejected("id=x\ntarget=1000000"); rejected("id=x\ntarget=4294967295999");
    rejected("id=x\ntarget=1.5"); rejected("id=x\ntarget=1\ntarget=2");
    rejected("id=x\nid=y\ntarget=1"); rejected("id=x\ntarget=1\nunknown=1");
    rejected("id=abcdefghijklmnopqrstuvwxyz123456789\ntarget=999");
    assert(!parse_request("id=x\0\ntarget=999", 16, &r));
    assert(plan_target(0, 999, 999999, &amount) == PLAN_ADD && amount == 999);
    assert(plan_target(1, 999, 999999, &amount) == PLAN_ADD && amount == 998);
    assert(plan_target(80, 999, 999999, &amount) == PLAN_ADD && amount == 919);
    assert(plan_target(500, 999, 999999, &amount) == PLAN_ADD && amount == 499);
    assert(plan_target(999, 999, 999999, &amount) == PLAN_NOOP && amount == 0);
    assert(plan_target(1200, 999, 999999, &amount) == PLAN_NOOP && amount == 0);
    assert(plan_target(1, 0, 999999, &amount) == PLAN_NOOP && amount == 0);
    assert(plan_target(0, 999999, 999999, &amount) == PLAN_ADD && amount == 999999);
    assert(plan_target(-1, 999, 999999, &amount) == PLAN_INVALID);
    assert(plan_target(0, 1000, 999, &amount) == PLAN_INVALID);
    assert(plan_target(0, -1, 999999, &amount) == PLAN_INVALID);
    assert(plan_target(0, 999, 999999, NULL) == PLAN_INVALID);
    puts("Request parsing, bounds, and target-difference tests passed.");
    return 0;
}
