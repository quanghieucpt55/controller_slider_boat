#include "gps_rmc.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

/* ====================== cấu hình ====================== */
#ifndef GPS_RMC_MAX_LINE
#define GPS_RMC_MAX_LINE  128
#endif

/* ====================== tiện ích ====================== */

static int hex2nibble(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    c = (char)toupper((unsigned char)c);
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

static int parse_u16_2digits(const char *s, uint8_t *out)
{
    if (!isdigit((unsigned char)s[0]) || !isdigit((unsigned char)s[1])) return 0;
    *out = (uint8_t)((s[0]-'0')*10 + (s[1]-'0'));
    return 1;
}

/** Đổi ddmm.mmmm/dddmm.mmmm -> abs(deg)*1e7. Trả 0 nếu lỗi. */
static int ddmm_to_deg1e7(const char *ddmm, uint32_t *out_deg1e7)
{
    if (!ddmm || !*ddmm) return 0;

    // tìm dấu '.'
    const char *dot = strchr(ddmm, '.');
    size_t len = strlen(ddmm);
    if (len < 3) return 0;

    // phần trước '.' (hoặc toàn bộ nếu không có)
    size_t int_len = dot ? (size_t)(dot - ddmm) : len;
    if (int_len < 3) return 0; // tối thiểu dmm

    // minutes: 2 chữ số trước '.', degrees: phần còn lại
    int min_tens = ddmm[int_len-2];
    int min_ones = ddmm[int_len-1];
    if (!isdigit((unsigned char)min_tens) || !isdigit((unsigned char)min_ones)) return 0;

    // parse degrees (dd hoặc ddd)
    char deg_str[4] = {0};
    size_t deg_len = int_len - 2;
    if (deg_len < 1 || deg_len > 3) return 0;
    for (size_t i=0; i<deg_len; i++) {
        if (!isdigit((unsigned char)ddmm[i])) return 0;
        deg_str[i] = ddmm[i];
    }
    int deg = atoi(deg_str);

    // parse minutes theo fixed-point *1e7, không dùng float
    char min_str[16] = {0};
    // copy từ minutes tới hết chuỗi
    size_t copy_len = len - (int_len-2);
    if (copy_len >= sizeof(min_str)) copy_len = sizeof(min_str)-1;
    memcpy(min_str, ddmm + (int_len-2), copy_len);
    min_str[copy_len] = '\0';

    // min_str: "MM[.ffff...]"
    uint64_t min_int = 0;
    uint64_t frac = 0;
    const char *p = min_str;
    // phần nguyên
    while (*p && *p != '.') {
        if (!isdigit((unsigned char)*p)) return 0;
        min_int = min_int*10 + (uint64_t)(*p - '0');
        p++;
    }
    if (*p == '.') p++;
    // phần lẻ tối đa 7 chữ số
    int frac_digits = 0;
    while (*p && frac_digits < 7) {
        if (!isdigit((unsigned char)*p)) break;
        frac = frac*10 + (uint64_t)(*p - '0');
        frac_digits++;
        p++;
    }
    // scale phần lẻ lên 1e7
    uint64_t frac_to_1e7 = frac;
    while (frac_digits < 7) { frac_to_1e7 *= 10; frac_digits++; }
    uint64_t min_scaled_1e7 = min_int * 10000000ULL + frac_to_1e7;

    uint64_t deg_scaled_1e7 = (uint64_t)deg * 10000000ULL;
    // deg + minutes/60
    uint64_t add = min_scaled_1e7 / 60ULL;

    uint64_t result = deg_scaled_1e7 + add;
    if (result > 0xFFFFFFFFu) result = 0xFFFFFFFFu;
    *out_deg1e7 = (uint32_t)result;
    return 1;
}

/** Đổi số thập phân -> *100 (lấy 2 chữ số thập phân). */
static int dec_to_x100(const char *s, uint32_t *out)
{
    if (!s || !*s) return 0;
    uint64_t ip = 0;
    int seen_digit = 0;

    while (*s && *s != '.') {
        if (!isdigit((unsigned char)*s)) return 0;
        seen_digit = 1;
        ip = ip*10 + (uint64_t)(*s - '0');
        s++;
    }
    if (!seen_digit) return 0;

    uint64_t frac2 = 0;
    if (*s == '.') {
        s++;
        // lấy tối đa 2 digit, thiếu thì coi là 0
        uint8_t d1 = (uint8_t)(isdigit((unsigned char)s[0]) ? (s[0] - '0') : 0);
        uint8_t d2 = (uint8_t)(isdigit((unsigned char)s[0]) && isdigit((unsigned char)s[1]) ? (s[1] - '0') : 0);
        frac2 = (uint64_t)d1 * 10ULL + (uint64_t)d2;
    }

    uint64_t val = ip*100ULL + frac2;
    if (val > 0xFFFFFFFFu) val = 0xFFFFFFFFu;
    *out = (uint32_t)val;
    return 1;
}

/** Đổi knots -> km/h*100 (có làm tròn). */
static int knots_to_kmh_x100(const char *knots_str, uint32_t *out_kmh_x100)
{
    if (!knots_str || !*knots_str) return 0;

    // parse knots dạng *1000 để tăng độ phân giải
    uint64_t ip = 0;
    uint64_t frac = 0;
    uint64_t frac_scale = 1;
    int seen_digit = 0;

    const char *s = knots_str;
    while (*s && *s != '.') {
        if (!isdigit((unsigned char)*s)) return 0;
        seen_digit = 1;
        ip = ip*10 + (uint64_t)(*s - '0');
        s++;
    }
    if (!seen_digit) return 0;

    if (*s == '.') {
        s++;
        int frac_digits = 0;
        while (*s && frac_digits < 3) {
            if (!isdigit((unsigned char)*s)) break;
            frac = frac*10 + (uint64_t)(*s - '0');
            frac_scale *= 10;
            frac_digits++;
            s++;
        }
        while (frac_scale < 1000) { frac_scale *= 10; frac *= 10; }
    } else {
        frac_scale = 1000;
    }

    uint64_t knots_x1000 = ip*1000ULL + (frac_scale ? (frac * (1000ULL/frac_scale)) : 0ULL);

    // km/h = knots*1.852
    // knots_x1000 * 1852 / 1000 = km/h * 1000  => chia 10 để ra km/h*100
    uint64_t kmh_x100 = (knots_x1000 * 1852ULL + 5ULL) / 10ULL; // +5 for rounding /10
    if (kmh_x100 > 0xFFFFFFFFu) kmh_x100 = 0xFFFFFFFFu;
    *out_kmh_x100 = (uint32_t)kmh_x100;
    return 1;
}

/* ====================== lõi parser ====================== */

// Double-buffer: IRQ chỉ gom câu, main mới parse
static char     s_lines[2][GPS_RMC_MAX_LINE];
static volatile uint8_t s_full[2] = {0, 0}; // 1: buffer có 1 line hoàn chỉnh (không gồm \n)
static uint8_t  s_write_idx = 0;
static uint16_t s_len = 0;
static uint8_t  s_in_sentence = 0;

static gnss_data_t s_last;
static volatile uint8_t s_has_new = 0;

void gps_rmc_init(void)
{
    memset(s_lines, 0, sizeof(s_lines));
    s_full[0] = 0;
    s_full[1] = 0;
    s_write_idx = 0;
    s_len = 0;
    s_in_sentence = 0;
    memset(&s_last, 0, sizeof(s_last));
    s_has_new = 0;
}

static int nmea_checksum_ok(const char *line)
{
    // line bắt đầu '$', không gồm CR/LF, dạng: $.....*HH
    const char *star = strchr(line, '*');
    if (!star) return 0;
    if ((star - line) < 2) return 0;

    int h1 = star[1] ? hex2nibble(star[1]) : -1;
    int h2 = star[2] ? hex2nibble(star[2]) : -1;
    if (h1 < 0 || h2 < 0) return 0;
    uint8_t expected = (uint8_t)((h1 << 4) | h2);

    uint8_t x = 0;
    for (const char *p = line + 1; p < star; p++) x ^= (uint8_t)(*p);

    return x == expected;
}

static int is_rmc_sentence(const char *line)
{
    // Chấp nhận $GNRMC, $GPRMC,... miễn là ..RMC
    if (!line || line[0] != '$') return 0;
    // $xxRMC,...
    return (strlen(line) >= 6 && line[3] == 'R' && line[4] == 'M' && line[5] == 'C');
}

static int utc_datetime_valid(const realtime_t *t)
{
    if (t == NULL) return 0;

    // kiểm tra nhanh thời gian UTC
    if (t->year < 2000u || t->year > 2099u) return 0;
    if (t->month < 1u || t->month > 12u) return 0;
    if (t->day < 1u || t->day > 31u) return 0;
    if (t->hour > 23u) return 0;
    if (t->minute > 59u) return 0;
    if (t->sec > 59u) return 0;
    // msec không bắt buộc
    return 1;
}

static int split_fields(char *s, char *fields[], int max_fields)
{
    int n = 0;
    fields[n++] = s;
    for (char *p = s; *p && n < max_fields; p++) {
        if (*p == ',') {
            *p = '\0';
            fields[n++] = p + 1;
        } else if (*p == '*') {
            *p = '\0';
            // bỏ qua phần checksum
            break;
        }
    }
    return n;
}

static int parse_rmc_to_struct(const char *line, gnss_data_t *out)
{
    // copy ra buffer để tách field
    char buf[GPS_RMC_MAX_LINE];
    strncpy(buf, line, sizeof(buf)-1);
    buf[sizeof(buf)-1] = '\0';

    // tách field theo ',' và '*'
    char *fields[16] = {0};
    int n = split_fields(buf, fields, 16);
    if (n < 10) return 0;

    // RMC: time, status, lat, N/S, lon, E/W, speed, COG, date...

    gnss_data_t g;
    memset(&g, 0, sizeof(g));

    // time
    if (fields[1] && strlen(fields[1]) >= 6) {
        uint8_t hh=0, mm=0, ss=0;
        if (parse_u16_2digits(&fields[1][0], &hh) &&
            parse_u16_2digits(&fields[1][2], &mm) &&
            parse_u16_2digits(&fields[1][4], &ss)) {
            g.dateTime.hour = hh;
            g.dateTime.minute = mm;
            g.dateTime.sec = ss;
            // msec
            // const char *dot = strchr(fields[1], '.');
            // if (dot && isdigit((unsigned char)dot[1])) {
            //     int ms = (dot[1]-'0')*100;
            //     if (isdigit((unsigned char)dot[2])) ms += (dot[2]-'0')*10;
            //     if (isdigit((unsigned char)dot[3])) ms += (dot[3]-'0');
            //     g.dateTime.msec = (uint16_t)ms;
            // } else {
            //     g.dateTime.msec = 0;
            // }
        }
    }

    // status A/V
    char status = (fields[2] && fields[2][0]) ? fields[2][0] : 'V';
    g.haveData = (status == 'A') ? 1u : 0u;
    // state: ACTIVE / TIME / RECV_DATA
    if (g.haveData) {
        g.state = (uint8_t)GNSS_STATE_3_ACTIVE;
    } else if (utc_datetime_valid(&g.dateTime)) {
        g.state = (uint8_t)GNSS_STATE_2_TIME;
    } else {
        g.state = (uint8_t)GNSS_STATE_1_RECV_DATA;
    }

    // lat/lon và bán cầu
    uint32_t lat1e7=0, lon1e7=0;
    if (ddmm_to_deg1e7(fields[3], &lat1e7)) {
        g.latitude = lat1e7;
        g.northSouth = (fields[4] && (fields[4][0]=='N' || fields[4][0]=='S')) ? (uint8_t)fields[4][0] : 0;
    }
    if (ddmm_to_deg1e7(fields[5], &lon1e7)) {
        g.longitude = lon1e7;
        g.eastWest = (fields[6] && (fields[6][0]=='E' || fields[6][0]=='W')) ? (uint8_t)fields[6][0] : 0;
    }

    // speed: knots -> km/h*100
    uint32_t spx100=0;
    if (knots_to_kmh_x100(fields[7], &spx100)) g.speed = spx100;

    // COG: độ*100
    uint32_t cogx100=0;
    if (dec_to_x100(fields[8], &cogx100)) g.cog = cogx100;

    // date: ddmmyy
    if (fields[9] && strlen(fields[9]) >= 6) {
        uint8_t dd=0, mo=0, yy=0;
        if (parse_u16_2digits(&fields[9][0], &dd) &&
            parse_u16_2digits(&fields[9][2], &mo) &&
            parse_u16_2digits(&fields[9][4], &yy)) {
            g.dateTime.day = dd;
            g.dateTime.month = mo;
            g.dateTime.year = (uint16_t)(2000u + yy); // giả định 20xx
        }
    }

    *out = g;
    return 1;
}

static int handle_line(const char *line)
{
    if (!is_rmc_sentence(line)) return 0;
    if (!nmea_checksum_ok(line)) return 0;

    gnss_data_t g;
    if (!parse_rmc_to_struct(line, &g)) return 0;

    s_last = g;
    s_has_new = 1;
    return 1;
}

int gps_rmc_feed_byte(uint8_t b)
{
    if (b == '$') {
        // nếu buffer đang full thì thử chuyển sang buffer còn trống
        if (s_full[s_write_idx]) {
            uint8_t other = (uint8_t)(s_write_idx ^ 1u);
            if (!s_full[other]) {
                s_write_idx = other;
            } else {
                // cả 2 buffer đều full -> bỏ qua
                s_in_sentence = 0;
                s_len = 0;
                return 0;
            }
        }

        s_in_sentence = 1;
        s_len = 0;
        s_lines[s_write_idx][s_len++] = (char)b;
        return 0;
    }
    if (!s_in_sentence) return 0;

    // kết thúc khi gặp '\n' (chấp nhận \r\n)
    if (b == '\n') {
        s_lines[s_write_idx][s_len] = '\0';
        s_in_sentence = 0;
        // bỏ '\r' nếu có
        if (s_len > 0 && s_lines[s_write_idx][s_len-1] == '\r') s_lines[s_write_idx][s_len-1] = '\0';

        // đánh dấu buffer đầy (nếu đã đầy thì drop)
        if (!s_full[s_write_idx]) {
            s_full[s_write_idx] = 1;
        }

        // chuyển sang buffer trống để ghi câu tiếp theo
        {
            uint8_t other = (uint8_t)(s_write_idx ^ 1u);
            if (!s_full[other]) {
                s_write_idx = other;
            }
        }

        s_len = 0;
        return 1;
    }

    if (s_len < (GPS_RMC_MAX_LINE - 1)) {
        s_lines[s_write_idx][s_len++] = (char)b;
    } else {
        // tràn buffer -> reset
        s_in_sentence = 0;
        s_len = 0;
    }
    return 0;
}

int gps_rmc_process(void)
{
    int got_valid = 0;

    // xử lý tối đa 2 line đang chờ
    for (int pass = 0; pass < 2; pass++) {
        uint8_t idx = 0xFFu;
        if (s_full[0]) idx = 0;
        else if (s_full[1]) idx = 1;
        else break;

        // parse line ở buffer idx
        if (handle_line(s_lines[idx])) got_valid = 1;

        // đã xử lý xong
        s_full[idx] = 0;
        s_lines[idx][0] = '\0';
    }

    return got_valid;
}

int gps_rmc_pop(gnss_data_t *out)
{
    if (!out) return 0;
    if (!s_has_new) return 0;
    *out = s_last;
    s_has_new = 0;
    return 1;
}

void gps_rmc_clear_new(void)
{
    s_has_new = 0;
}
