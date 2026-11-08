/*
 * 红外空调控制模块 —— 精简整合版（组员 B）
 *
 * 与 ir_ac_all_in_one.c 相比：去掉完整脉冲解码器、重复断言、演示程序和配置接口，
 * 只保留完成分工任务所需的最小代码：红外发射、发射码校验、空调开关、有人/无人联动。
 *
 * 编译：gcc -std=c11 -Wall -Wextra -Werror -pedantic ir_ac_minimal.c -o ir_ac_minimal.exe
 * 运行：ir_ac_minimal.exe
 *
 * 真机接入：把 sink_t 的两个函数换成 GPIO/PWM 载波实现；把 feed/read_signal 换成组员 A 的驱动。
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* ---------------- 配置 ---------------- */
#define CARRIER_HZ       38000U   /* 38kHz 载波 */
#define LEADER_MARK      9000U
#define LEADER_SPACE     4500U
#define BIT_MARK          560U
#define ZERO_SPACE        560U
#define ONE_SPACE        1690U
#define STOP_MARK         560U
#define AC_ADDR         0x00FFU   /* 演练地址，真机替换为厂商码 */
#define CMD_ON           0x01U
#define CMD_OFF          0x02U
#define ABSENT_DELAY     5000U    /* 无人多久后关机；真机改 300000U（5 分钟） */
#define MIN_INTERVAL     1000U    /* 两次发码最小间隔 */
#define MIN_CONFIDENCE     50U    /* 低于此置信度的信号忽略 */

#define CHECK(cond, msg)                            \
    do {                                            \
        if (!(cond)) {                              \
            fprintf(stderr, "[FAIL] %s\n", (msg));  \
            return 1;                               \
        }                                           \
    } while (0)

/* ---------------- 1. 组员 A：人体检测信号 ---------------- */
typedef enum { ABSENT = 0, PRESENT = 1 } human_t;

typedef struct {
    human_t state;
    uint8_t confidence;
    uint32_t ts;
} signal_t;

static signal_t s_signal;
static bool s_valid;

static void feed(human_t state, uint8_t confidence, uint32_t ts)
{
    s_signal.state = state;
    s_signal.confidence = confidence;
    s_signal.ts = ts;
    s_valid = true;
}

static int read_signal(signal_t *out)
{
    if (!s_valid) {
        return -1;
    }
    *out = s_signal;
    return 0;
}

/* ---------------- 2. 红外发射驱动 ---------------- */
typedef void (*pulse_fn)(void *ctx, uint32_t us);

typedef struct {
    pulse_fn on;   /* 38kHz 载波发射 */
    pulse_fn off;  /* 静默间隔 */
    void *ctx;
} sink_t;

static void send_byte(const sink_t *s, uint8_t byte)
{
    uint32_t i;
    for (i = 0U; i < 8U; ++i) {
        s->on(s->ctx, BIT_MARK);
        s->off(s->ctx, ((byte >> i) & 1U) ? ONE_SPACE : ZERO_SPACE);
    }
}

/* 一帧 = 引导码 + 地址低/高 + 命令 + 命令反码 + 结束位（LSB 优先） */
static void send_power(const sink_t *s, uint8_t cmd)
{
    s->on(s->ctx, LEADER_MARK);
    s->off(s->ctx, LEADER_SPACE);
    send_byte(s, (uint8_t)(AC_ADDR & 0xFFU));
    send_byte(s, (uint8_t)(AC_ADDR >> 8));
    send_byte(s, cmd);
    send_byte(s, (uint8_t)~cmd);
    s->on(s->ctx, STOP_MARK);
}

/* ---------------- 3. 极简仿真接收器：还原数据位并校验反码 ---------------- */
typedef struct {
    uint8_t bytes[4];
    uint32_t bits;
    bool bit_next;
} capture_t;

static void cap_on(void *ctx, uint32_t us)
{
    capture_t *c = (capture_t *)ctx;
    c->bit_next = (us == BIT_MARK);
}

static void cap_off(void *ctx, uint32_t us)
{
    capture_t *c = (capture_t *)ctx;
    if (!c->bit_next) {
        return;
    }
    c->bit_next = false;
    if (c->bits < 32U) {
        if (us >= ONE_SPACE - 200U) {
            c->bytes[c->bits / 8U] |= (uint8_t)(1U << (c->bits % 8U));
        }
    }
    c->bits++;
}

/* 返回解码出的命令；0 表示帧无效 */
static uint8_t capture_command(const capture_t *c)
{
    if (c->bits != 32U || c->bytes[0] != (uint8_t)(AC_ADDR & 0xFFU) ||
        c->bytes[1] != (uint8_t)(AC_ADDR >> 8) ||
        c->bytes[3] != (uint8_t)~c->bytes[2]) {
        return 0U;
    }
    return c->bytes[2];
}

/* ---------------- 4. 空调状态机与联动 ---------------- */
typedef struct {
    bool on;
    const sink_t *sink;
    uint32_t last_send;
    uint32_t absent_since;
    bool absent_pending;
    uint32_t frames;
} ac_t;

static void set_power(ac_t *ac, bool on, uint32_t now)
{
    if (ac->on == on) {                                  /* 状态去重 */
        return;
    }
    if (ac->frames > 0U && (uint32_t)(now - ac->last_send) < MIN_INTERVAL) {
        return;                                          /* 最小发送间隔，差值法兼容时钟回绕 */
    }
    send_power(ac->sink, on ? CMD_ON : CMD_OFF);
    ac->on = on;
    ac->frames++;
    ac->last_send = now;
}

static void on_signal(ac_t *ac, const signal_t *sig)
{
    if (sig->confidence < MIN_CONFIDENCE) {              /* 低置信度忽略 */
        return;
    }
    if (sig->state == PRESENT) {
        ac->absent_pending = false;                      /* 有人立即取消无人计时 */
        set_power(ac, true, sig->ts);
        return;
    }
    if (!ac->absent_pending) {
        ac->absent_pending = true;
        ac->absent_since = sig->ts;
    }
    if ((uint32_t)(sig->ts - ac->absent_since) >= ABSENT_DELAY) {
        set_power(ac, false, sig->ts);
    }
}

static void poll(ac_t *ac)
{
    signal_t sig;
    if (read_signal(&sig) == 0) {
        on_signal(ac, &sig);
    }
}

/* ---------------- 5. 自测：红外码发射 + 有人/无人联动 ---------------- */
int main(void)
{
    capture_t cap;
    sink_t sink;
    ac_t ac;

    memset(&cap, 0, sizeof(cap));
    sink.on = cap_on;
    sink.off = cap_off;
    sink.ctx = &cap;

    /* (1) 单独测试红外发射：发出的空调开机 / 关机码可被解码还原 */
    send_power(&sink, CMD_ON);
    CHECK(capture_command(&cap) == CMD_ON, "POWER_ON infrared code not emitted");
    memset(&cap, 0, sizeof(cap));
    send_power(&sink, CMD_OFF);
    CHECK(capture_command(&cap) == CMD_OFF, "POWER_OFF infrared code not emitted");
    printf("[PASS] ir_transmit: carrier=%luHz, NEC 32-bit frame, addr=0x%04X, cmd=0x01/0x02, checksum OK\n",
           (unsigned long)CARRIER_HZ, (unsigned)AC_ADDR);

    /* (2) 对接 A 的信号：有人开机、无人延时关机、去重与取消计时 */
    memset(&cap, 0, sizeof(cap));
    memset(&ac, 0, sizeof(ac));
    ac.sink = &sink;

    feed(PRESENT, 90U, 1000U);
    poll(&ac);
    CHECK(ac.on && ac.frames == 1U, "presence must turn AC on");

    feed(ABSENT, 90U, 2000U);
    poll(&ac);
    CHECK(ac.on, "AC must stay on inside absence delay window");

    feed(PRESENT, 90U, 4500U);
    poll(&ac);
    CHECK(ac.on && ac.frames == 1U, "re-detection cancels timer without re-transmitting");

    feed(ABSENT, 90U, 6000U);
    poll(&ac);
    memset(&cap, 0, sizeof(cap));                 /* 只捕获超时后的关机码 */
    feed(ABSENT, 90U, 11000U);
    poll(&ac);
    CHECK(!ac.on && ac.frames == 2U, "absence timeout must turn AC off");
    CHECK(capture_command(&cap) == CMD_OFF, "POWER_OFF code not emitted on timeout");
    printf("[PASS] occupancy: PRESENT -> ON, ABSENT delay -> OFF, de-dup / cancel timer OK\n");

    /* (3) 低置信度信号不改变状态 */
    feed(ABSENT, 40U, 12000U);
    poll(&ac);
    CHECK(!ac.on && ac.frames == 2U, "low-confidence sample must be ignored");
    printf("[PASS] occupancy: confidence<50 ignored\n");

    printf("ALL TESTS PASSED\n");
    return 0;
}
