# 模块接口约定文档
> 本文档统一所有模块通信规则，所有人开发以此为准；改动必须更新文档 + 全员同步

## 1. TCP 雷达通信（A 模块输出人员状态）
- TCP 服务端端口：**8080**
- 报文格式：单次发送单个字符
  - `0`：无人
  - `1`：有人
- 说明：雷达连接 ESP32 TCP 服务端，持续上报 0 / 1

## 2. 固件内部人员状态变量（供 B 模块读取）
全局状态：`human_present`
- human_present = 1：有人
- human_present = 0：无人

## 3. HTTP 接口约定（C 模块开放，给 D 网页前端对接）
- HTTP 服务端端口：**80**（与 TCP 8080 区分，避免冲突）
- 数据格式：JSON，`Content-Type: application/json`，UTF-8 编码
- Base URL：`http://<ESP32_IP>/`
- 以下接口由 C 模块实现，D 模块按此对接：

| 方法 | 路径 | 说明 | 返回示例 |
|------|------|------|----------|
| GET | `/api/status` | 当前人员状态与基础信息 | `{"human_present":1,"online":true,"uptime_s":1234}` |
| GET | `/api/logs?limit=50` | 带时间戳日志列表（默认 50 条） | `{"logs":[{"ts":"2026-11-05T10:00:00","level":"INFO","msg":"..."}]}` |
| GET | `/api/stats` | 异常 / 事件统计 | `{"wifi_errors":0,"radar_errors":0,"auto_off_count":3}` |
| POST | `/api/simulate` | B 模块「有人/无人模拟开关」覆盖状态 | 请求 `{"human_present":1}`，返回 `{"ok":true}` |

> 字段约定：`human_present` 含义同第 2 节（1=有人 / 0=无人）；`ts` 时间格式 `YYYY-MM-DDTHH:MM:SS`。

## 4. B 模块空调控制约定（建议，待全员确认）
- B 读取全局变量 `human_present` 决定空调：
  - `human_present = 1`（有人）→ 开启空调（按设定逻辑运行）
  - `human_present = 0`（无人）→ 延时 N 秒后关闭空调（N 待定）
- B 提供「有人 / 无人模拟开关」：通过 `POST /api/simulate` 覆盖 `human_present`，方便无雷达时调试（见第 3 节）。
- 两套控制逻辑（具体策略）由 B 在 Issue 内细化，确定后回填此处。

## 5. 通用约定
| 项目 | 约定 |
|------|------|
| TCP 雷达端口 | 8080 |
| HTTP 接口端口 | 80 |
| 时间格式 | ISO8601 `YYYY-MM-DDTHH:MM:SS` |
| 文本编码 | UTF-8 |
| 状态值 | `human_present`：1=有人 / 0=无人 |

## 6. 变更记录
| 日期 | 修改人 | 内容 |
|------|--------|------|
| 2026-11-03 | A | 初版定稿：TCP 端口 8080，0/1 表示无/有人，human_present 全局变量 |
| 2026-11-05 | A | 补充 C 模块 HTTP 接口（端口 80、/api/*）、B 空调控制约定、通用约定表 |
