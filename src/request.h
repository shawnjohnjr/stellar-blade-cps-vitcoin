/* Pure C request parsing and target planning; shared by the DLL and host tests. */
#ifndef VITCOIN_REQUEST_H
#define VITCOIN_REQUEST_H
#include <stdint.h>
#include <stddef.h>

#define REQUEST_ID_MAX 32
typedef struct { char id[REQUEST_ID_MAX + 1]; int32_t target; } Request;
enum Plan { PLAN_INVALID = -1, PLAN_NOOP = 0, PLAN_ADD = 1 };

static int request_id_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_' || c == '-';
}
static int request_space(char c) { return c == ' ' || c == '\t' || c == '\r'; }
static int request_key(const char* p, size_t n, const char* expected) {
    size_t i = 0;
    for (; i < n && expected[i]; ++i) if (p[i] != expected[i]) return 0;
    return i == n && !expected[i];
}
static int parse_request(const char* text, size_t length, Request* result) {
    size_t pos = 0; int have_id = 0, have_target = 0;
    Request r = {{0}, 0};
    if (!text || !result || length > 512) return 0;
    if (length >= 3 && (unsigned char)text[0] == 0xef &&
        (unsigned char)text[1] == 0xbb && (unsigned char)text[2] == 0xbf) pos = 3;
    while (pos < length) {
        size_t start = pos, end, eq, key_end, value;
        while (pos < length && text[pos] != '\n') { if (!text[pos]) return 0; ++pos; }
        end = pos; if (pos < length) ++pos;
        while (start < end && request_space(text[start])) ++start;
        while (end > start && request_space(text[end - 1])) --end;
        if (start == end || text[start] == '#' || text[start] == ';') continue;
        eq = start; while (eq < end && text[eq] != '=') ++eq;
        if (eq == end) return 0;
        key_end = eq; while (key_end > start && request_space(text[key_end - 1])) --key_end;
        value = eq + 1; while (value < end && request_space(text[value])) ++value;
        if (request_key(text + start, key_end - start, "id")) {
            size_t n = end - value, i;
            if (have_id || n < 1 || n > REQUEST_ID_MAX) return 0;
            for (i = 0; i < n; ++i) { if (!request_id_char(text[value + i])) return 0; r.id[i] = text[value + i]; }
            r.id[n] = 0; have_id = 1;
        } else if (request_key(text + start, key_end - start, "target")) {
            uint32_t total = 0; size_t i;
            if (have_target || value == end) return 0;
            for (i = value; i < end; ++i) {
                unsigned digit = (unsigned)(text[i] - '0');
                if (digit > 9 || total > (999999u - digit) / 10u) return 0;
                total = total * 10u + digit;
            }
            r.target = (int32_t)total; have_target = 1;
        } else return 0;
    }
    if (!have_id || !have_target) return 0;
    *result = r; return 1;
}
static enum Plan plan_target(int64_t current, int32_t target, int32_t item_max, int32_t* amount) {
    if (!amount) return PLAN_INVALID;
    *amount = 0;
    if (current < 0 || current > 10000000 || target < 0 || item_max < 1 ||
        target > item_max || target > 999999) return PLAN_INVALID;
    if (current >= target) return PLAN_NOOP;
    *amount = (int32_t)((int64_t)target - current);
    return PLAN_ADD;
}
#endif
