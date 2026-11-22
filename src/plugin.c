/* SPDX-License-Identifier: MIT */
#include "runtime.h"
#include "game.h"
#include "request.h"

static char request_path[1100], report_path[1100], receipt_path[1100];
static uint64_t next_poll, deadline;
static int ready, verifying;
static Request active;
static int32_t amount;
static int64_t before_vit, before_gold;
static char last_id[REQUEST_ID_MAX + 1];

static int report_open(void) {
    report = CreateFileA(report_path, FILE_APPEND_DATA, FILE_SHARE_READ, 0,
                         OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    return report != INVALID_HANDLE_VALUE;
}
static void report_close(void) {
    if (report != INVALID_HANDLE_VALUE) {
        FlushFileBuffers(report); CloseHandle(report); report = INVALID_HANDLE_VALUE;
    }
}
static void log_request(const char* state, int64_t current, int64_t gold) {
    if (!report_open()) return;
    out(state); out(" id="); out(active.id); out(" current="); num(current);
    out(" target="); num(active.target); out(" amount="); num(amount);
    out(" G="); num(gold); out("\r\n"); report_close();
}
static int read_request(Request* r) {
    char text[513]; DWORD n = 0; HANDLE file;
    file = CreateFileA(request_path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE |
                       FILE_SHARE_DELETE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE) return 0;
    if (!ReadFile(file, text, sizeof(text), &n, 0)) { CloseHandle(file); return 0; }
    CloseHandle(file);
    return n <= 512 && parse_request(text, n, r);
}
static int reserve_request(void) {
    HANDLE file; DWORD written = 0;
    static const char receipt[] = "Reserved before native call. Do not reuse this request ID.\r\n";
    copy(receipt_path, dir, sizeof(receipt_path)); append(receipt_path, "/attempt-", sizeof(receipt_path));
    append(receipt_path, active.id, sizeof(receipt_path)); append(receipt_path, ".txt", sizeof(receipt_path));
    file = CreateFileA(receipt_path, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_NEW,
                       FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE) return 0;
    if (!WriteFile(file, receipt, sizeof(receipt) - 1, &written, 0) ||
        written != sizeof(receipt) - 1 || !FlushFileBuffers(file)) {
        CloseHandle(file); return 0;
    }
    CloseHandle(file); return 1;
}
static void on_tick(void* user, float dt) {
    uint64_t now = GetTickCount64(); cps_state state = {0};
    int32_t actor_id; int64_t vc, gold; Request candidate; enum Plan plan;
    (void)user; (void)dt;
    if (now < next_poll) return;
    next_poll = now + 1000;
    state.size = sizeof(state);
    if (!a->state(&state) || !state.in_gameplay || !a->character(0)) return;
    if (!ready) {
        if (!identify_item() || !resolve_native() || !inventory(&actor_id, &vc, &gold, 0)) return;
        ready = 1;
        a->log(self, CPS_LOG_INFO, "Ready in read-only mode. Create request.ini to raise Vitcoin to a target total.");
    }
    if (!inventory(&actor_id, &vc, &gold, 0)) return;
    if (verifying) {
        if (vc == active.target && gold == before_gold) {
            log_request("VERIFIED", vc, gold); verifying = 0;
            a->log(self, CPS_LOG_INFO, "Target total verified; gold unchanged. Request ID consumed permanently.");
        } else if (now >= deadline) {
            log_request("UNCONFIRMED_NO_RETRY", vc, gold); verifying = 0;
            a->log(self, CPS_LOG_WARNING, "Result not verified within 15 seconds. No automatic retry; inspect status.txt and inventory.");
        }
        return;
    }
    if (!read_request(&candidate) || equal(candidate.id, last_id)) return;
    active = candidate; copy(last_id, active.id, sizeof(last_id));
    plan = plan_target(vc, active.target, item_max_amount, &amount);
    if (plan == PLAN_INVALID) { log_request("INVALID_TARGET_NO_WRITE", vc, gold); return; }
    if (!reserve_request()) { log_request("ALREADY_RESERVED_OR_IO_ERROR_NO_WRITE", vc, gold); return; }
    if (plan == PLAN_NOOP) { log_request("ALREADY_AT_OR_ABOVE_TARGET_NO_WRITE", vc, gold); return; }
    before_vit = vc; before_gold = gold;
    log_request("ONE_NATIVE_REQUEST", before_vit, before_gold);
    verifying = 1; deadline = now + 15000;
    ((AddRequest)native_add)(0, 1, actor_id, &vit_name, amount, 1,
                           &empty_delegate, &empty_delegate, 0, 0, 0);
}
__declspec(dllexport) int cps_plugin_load(const cps_api* host, const cps_plugin* plugin) {
    const cps_engine* engine;
    a = host; self = plugin->client; copy(dir, plugin->config_dir, sizeof(dir));
    engine = a->engine(); if (!engine) return 0;
    pool = engine->name_pool; objects = engine->objects;
    copy(request_path, dir, sizeof(request_path)); append(request_path, "/request.ini", sizeof(request_path));
    copy(report_path, dir, sizeof(report_path)); append(report_path, "/status.txt", sizeof(report_path));
    a->log(self, CPS_LOG_INFO, "Headless Vitcoin loaded. No request file means no currency changes.");
    return a->on_game_tick(self, on_tick, 0);
}
__declspec(dllexport) void cps_plugin_unload(void) {}
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    (void)instance; (void)reason; (void)reserved; return TRUE;
}
