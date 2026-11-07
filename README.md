# Air-conditioner-group2
中德空调改造项目-测试与开发小组2组仓库
小组成员：26自动化詹怡洁，26微电子程方志，25智感李慧仪；

## 项目进度
- 10.31 进行：初步协作，分支开发
- 11.3 进行：小项目人体感应空调节能控制系统

## 开发协作规则
1. main 分支是稳定主分支，**禁止直接 commit/push 到 main**
2. 新开发分支命名：`feature/xxx`；bug 修复分支：`bugfix/xxx`
3. Commit 格式规范：
   - feat: 新增功能
   - fix: 修复 bug
   - docs: 更新文档
4. 开发前先看对应 Issue，分支和 Issue 对应
5. 所有修改必须提交 Pull Request，至少 2 人评审通过才能合并；**不能合并自己的 PR**

## PR 交叉评审安排
- B 的 PR：组长 A + D 评审
- C 的 PR：组长 A + B 评审
- D 的 PR：组长 A + C 评审
- A 的 PR：B + C 评审，由 B 或 C 合并，A 不能合并自己代码

## 目录结构
```
Air-conditioner-group2/
├── firmware/        # ESP32 固件（雷达接收 / 空调控制 / 日志统计）
├── web-console/     # 嵌入式网页控制台
├── docs/            # 文档（接口约定 interface.md 等）
├── hardware/        # 硬件相关（原理图、BOM 等）
├── archive/         # 前期练习代码归档（cfz 时间估算 / tabby 温度判断）
├── README.md
└── .gitignore
```

> 模块间通信规则以 `docs/interface.md` 为准，改动必须更新文档并全员同步。
> 
# 智能家居联动固件

小组协作仓库：各模块位于 `firmware/`，通过功能分支 + Pull Request + 交叉评审合并到 `main`。

## 模块分工

| 组员 | 负责模块 | 主要文件 |
| --- | --- | --- |
| A | 人体检测信号 | `firmware/inc/human_sensor.h`、`firmware/src/human_sensor.c` |
| B | 红外空调控制（PR-01 已合并） | `firmware/inc/ir_driver.h`、`firmware/src/ir_driver.c`、`firmware/src/ac_control.c` |

## 构建与自测

- 工具链：GCC (MinGW-W64)、`mingw32-make`
- 一键自测：`powershell -ExecutionPolicy Bypass -File scripts/run-tests.ps1`
- 等价入口：`mingw32-make test`

## 红外空调控制模块（组员 B）

由分支 `feat/ir-control` 开发，经 PR-01 审核通过后合并入 `main`。
PR 描述见 `docs/pull_requests/PR-01-ir-control.md`，审核意见与复审结论见 `docs/reviews/review-PR-01.md`。

### 文件说明

| 文件 | 作用 |
| --- | --- |
| `firmware/inc/ir_driver.h`、`firmware/src/ir_driver.c` | 红外发射驱动：38kHz 载波、NEC 风格 32 位帧编码、空调开机码 `0x01` / 关机码 `0x02` |
| `firmware/hal/ir_capture.h`、`firmware/hal/ir_capture.c` | PC 仿真红外接收器：记录 mark/space 波形并解码还原地址与命令 |
| `firmware/inc/ac_control.h`、`firmware/src/ac_control.c` | 空调开关状态机与人体检测联动 |
| `firmware/src/main.c` | 有人 / 无人联动演示程序 |
| `tests/test_ir_transmit.c`、`tests/test_occupancy.c` | 红外发射单独自测、联动自测 |

### 红外协议

- 38kHz 载波；帧结构为 9000us 引导 mark + 4500us space + 32 位数据（地址低/高字节、命令、命令反码，LSB 优先）+ 560us 结束 mark；数据位用 560us / 1690us space 表示 0 / 1。
- 设备地址 `IR_AC_ADDRESS = 0x00FF` 为演练值，接入真实空调时替换为厂商地址码与完整码表。
- `ir_sink_t` 是硬件抽象层：真实设备上由 `carrier_on()` / `carrier_off()` 驱动 GPIO/PWM 与载波调制，驱动层无需改动。

### 联动策略（对接组员 A）

- 依赖 `firmware/inc/human_sensor.h` 的 `human_sensor_read()`；`ac_controller_poll()` 轮询 A 提供的 `human_detection_signal_t`，B 模块不修改 A 的实现。
- 有人 → 发送开机码；无人 → 启动计时，持续超过 `absent_delay_ms`（默认 300000ms）才发送关机码，期间有人立即取消计时。
- `confidence < 50` 的低置信度采样忽略；`min_command_interval_ms`（默认 1000ms）内的翻转指令返回 `IR_ERR_BUSY`，不发码、不切状态。
- 电源状态去重避免重复发射；时间差值用无符号减法，兼容 `uint32_t` 毫秒时钟回绕。
- 阈值可通过 `ac_controller_configure()` 注入，便于产测和自测。

### 自测

```text
powershell -ExecutionPolicy Bypass -File scripts/run-tests.ps1
mingw32-make test          # 或 mingw32-make test-ir / test-occupancy / demo
```

`test_ir_transmit` 单独验证可正常发出空调红外码：仿真接收器按脉冲时序解码，校验引导码、
67 个脉冲段、地址 `0x00FF`、命令 `0x01` / `0x02` 及命令反码。

## 协作流程

1. 从最新 `main` 建功能分支，命名 `feat/<module>`。
2. 本地自测通过后推送分支并发起 PR。
3. PR 需写明实现功能、自测情况、对接依赖，并指定审核人。
4. 审核通过后合并入 `main`，本地切回 `main` 拉取最新代码。

## 真机接入与后续 TODO

> 当前仓库是可在 PC 上编译运行的仿真版本；以下内容为迁移到真实硬件 / 平台时需要补齐的部分。

### 1. 真实空调码表与协议适配

- `firmware/src/ir_driver.c` 目前只实现 NEC 风格 32 位帧（地址低/高 + 命令 + 命令反码），命令仅 `0x01` 开机、`0x02` 关机。
- 主流空调（大金、格力、美的等）多为变长帧 + 自定义校验，与 NEC 不兼容。接入步骤：
  1. 用红外接收头录制原遥控器的开机 / 关机 / 温度 / 模式 / 风速 / 扫风码；
  2. 在 `firmware/inc/ir_driver.h` 扩充命令枚举与帧结构，或新增 `firmware/src/ir_protocol_<brand>.c` 适配层；
  3. 替换 `ir_driver_send_frame()` 的编码，并同步调整 `firmware/hal/ir_capture.c` 的解码与时序容差；
  4. 把 `IR_AC_ADDRESS` 换成厂商地址码，更新 `tests/test_ir_transmit.c` 的期望值。
- `IR_AC_ADDRESS = 0x00FF` 与 `0x01` / `0x02` 均为演练值，不可直接用于真机。

### 2. `ir_sink_t` 的 MCU 实现

- `firmware/hal/ir_capture.c` 只是 PC 仿真接收器（记录脉冲 + 解码），目标板编译时应排除。
- 需新增 `firmware/hal/ir_sink_mcu.c` 及对应头文件，实现 `carrier_on(ctx, duration_us)` / `carrier_off(ctx, duration_us)`：定时器输出 38kHz PWM（常用 1/3 占空比）、按微秒定时开关载波、驱动指定 GPIO 上的红外发射管（驱动管与限流电阻按硬件原理图配置）。
- 应用层只需在初始化时把该 sink 传入 `ac_controller_init()`，`ir_driver.c` / `ac_control.c` 无需改动。

### 3. 与组员 A 的联调契约

| 项目 | 约定 |
| --- | --- |
| 数据入口 | A 侧采样任务调用 `human_sensor_feed(state, confidence, timestamp_ms)`；B 侧用 `human_sensor_read()` / `ac_controller_poll()` 读取 |
| 时间戳 | 单调毫秒时钟，A/B 使用同一时基；计时用无符号差值，允许 `uint32_t` 回绕 |
| 轮询周期 | 建议 ≤ 1s（小于最小发送间隔 1000ms），由主循环或定时任务驱动 |
| 默认阈值 | 无人关机延时 300000ms、最小发送间隔 1000ms、最低置信度 50；用 `ac_controller_configure()` 按现场调整 |
| 异常语义 | 传感器无有效数据时保持现状、不发码；低置信度采样既不触发动作，也不启动 / 清除无人计时 |
| 待办 | A 的 `feat/human-detection` 去抖 PR 当前结论为「请求修改」（时间戳回绕导致去抖窗口失效），修复合并后再联调稳定状态，详见 `docs/reviews/review-PR-02-human-detection.md` |

### 4. 目标板构建与部署

- 现状：MinGW-W64 GCC + `mingw32-make`，产物为 Windows 可执行文件，用于 PC 仿真自测。
- 待补：确定目标 MCU 与交叉编译工具链（如 `arm-none-eabi-gcc`）、启动文件 / 链接脚本、烧录与调试方式。
- 待补：`Makefile` 增加目标板目标（如 `make mcu`），切换 `CC` / `CFLAGS`，排除 `firmware/src/main.c`（演示）与 `firmware/hal/ir_capture.c`（仿真接收器），接入第 2 项的 MCU sink；`scripts/run-tests.ps1` 保留为 PC 仿真自测入口。

### 5. 验收与排障

- 真机抓码：用逻辑分析仪或 VS1838B 等红外接收头采集发射波形，核对 9000us / 4500us 引导码、560us / 1690us 位时序、设备地址与命令反码；`ir_capture_decode_frame()` 的 ±120us 容差可作为参考。
- 常见故障：

| 现象 | 排查方向 |
| --- | --- |
| 完全无码 | sink 未接线 / 未启用定时器 PWM、GPIO 引脚或驱动管配置错误、载波未调制到 38kHz |
| 空调不响应或误码 | 协议与码表不匹配（见第 1 项）、脉冲时序偏差超出容差、命令反码校验错误 |
| 频繁开关机 | 无人延时或最小发送间隔配置过小、A 侧信号抖动或漏检、置信度阈值过低 |
| 联动不触发 | 传感器无有效采样、`confidence < 50`、状态已相同被去重、`IR_ERR_BUSY` 拦截 |

- 单项自测入口：`mingw32-make test-ir`（红外发射）、`mingw32-make test-occupancy`（联动）、`mingw32-make demo`（演示）。

### 6. 协作状态与后续事项

| 项目 | 状态 |
| --- | --- |
| B 的功能分支 / PR | `feat/ir-control`；PR-01 描述见 `docs/pull_requests/PR-01-ir-control.md`，审核人组员 A |
| 审核与合并 | 首轮「请求修改」→ 修复提交 `40625a4` → 复审通过 → 合并提交 `aea19ad`（`Reviewed-by: Member A`），评审记录 `docs/reviews/review-PR-01.md` |
| 模块文档 | `README.md` 由提交 `17ce2cb` 补充；本次补充真机接入与后续事项 |
| A 的 PR-02 | `feat/human-detection`，描述 `docs/pull_requests/PR-02-human-detection.md`，评审结论「请求修改」，**尚未合并** |
| 远程仓库 | 当前 `origin` 为本地裸仓库 `.remote/smart-ac.git`（演练用）；接入 GitHub/GitLab 需替换 `origin` 并复现 PR / 评审流程 |



日志存储模块：
功能：实现检测数据持久保存，日志字段与interface.md保持一致
配套文档：docs/日志格式说明文档
