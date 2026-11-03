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
