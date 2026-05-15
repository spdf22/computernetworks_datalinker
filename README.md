# Computer Networks Lab 1: Data Link Layer Protocols

北京邮电大学计算机网络实验一：数据链路层滑动窗口协议设计与实现。

本仓库整理为小组协作与实验报告使用的代码仓库。默认分支展示最终实现版本：**Selective Repeat (SR) + piggybacked ACK + ACK timer + NAK**。历史分支保留了从原始停等协议到各个可选协议的演进过程。

## 版本结构

| 分支 / 标签 | 协议版本 | 说明 |
| --- | --- | --- |
| `version/00-stopwait-baseline` / `baseline-stopwait` | Stop-and-Wait | 实验包原始停等协议样例 |
| `version/01-gbn-basic` / `version-gbn-basic` | Go-Back-N | 基础滑动窗口、累计 ACK、超时回退重传 |
| `version/02-gbn-piggyback-acktimer` / `version-gbn-piggyback-acktimer` | GBN + piggyback + ACK timer | 在数据帧中捎带 ACK，并使用 ACK 定时器延迟独立确认 |
| `version/03-gbn-nak` / `version-gbn-nak` | GBN + NAK | 在误码或乱序时使用 NAK 加快恢复 |
| `version/04-sr-selective-repeat` / `version-sr-selective-repeat` | Selective Repeat | 最终版本，支持乱序缓存、选择确认、选择重传、NAK、ACK timer |

切换版本示例：

```powershell
git switch version/01-gbn-basic
git switch version/04-sr-selective-repeat
```

## 代码位置

主要工程保留 Visual Studio 2019 版本：

```text
Lab1-2024(Win+Linux)/Lab1-2024(Win+Linux)/Lab1-Windows-VS2019/
```

核心文件：

```text
datalink.c   协议主体实现
datalink.h   帧类型定义
protocol.h   实验框架接口
protocol.c   仿真网络层、物理层、信道、事件循环
crc32.c      CRC32 校验
lprintf.c    日志输出
```

本仓库已移除 VS2013、VS2017、Linux 示例和教师资料文档，避免 GitHub 页面被无关文件淹没。

## 最终 SR 设计摘要

最终版本使用选择重传协议，主要机制如下：

- 发送窗口大小 `NR_BUFS = 8`
- 序号空间 `0..127`，即 `MAX_SEQ = 127`
- 数据帧超时定时器 `DATA_TIMER = 1200 ms`
- ACK 延迟定时器 `ACK_TIMER = 280 ms`
- DATA 帧支持有效位 `ack_valid`，避免旧 ACK 在序号回绕后被误认为新确认
- 接收端缓存窗口内乱序帧，只有从 `frame_expected` 开始连续可交付时才调用 `put_packet`
- 收到坏帧或发现缺口时发送 NAK，请求对方重传指定序号
- 每个发送缓存槽和接收缓存槽记录实际序号，避免序号回绕后的旧帧/旧 ACK 污染当前窗口

窗口大小选择理由：

实验信道为 8000 bps，全双工，单向传播时延 270 ms，网络层分组长度 256 字节。数据帧约 260 字节，发送时间约 260 ms，往返传播时延约 540 ms，因此窗口至少约为 4 才能较好填满信道。最终选择窗口 8，给误码恢复和调度抖动留出余量；序号空间扩大到 128，是为了降低长时间运行时延迟重传帧与新窗口混淆的风险。

## 编译方法

使用 Visual Studio 2019/2022 打开：

```text
Lab1-2024(Win+Linux)/Lab1-2024(Win+Linux)/Lab1-Windows-VS2019/datalink.sln
```

若 Visual Studio 提示升级 Windows SDK 或平台工具集，可选择当前已安装版本，例如 Windows SDK 10.0、平台工具集 v143。

命令行构建示例：

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" `
  "Lab1-2024(Win+Linux)\Lab1-2024(Win+Linux)\Lab1-Windows-VS2019\datalink.sln" `
  /p:Configuration=Debug /p:Platform=Win32
```

生成的可执行文件通常位于：

```text
Lab1-2024(Win+Linux)/Lab1-2024(Win+Linux)/Lab1-Windows-VS2019/Win32-Debug/datalink.exe
```

## 运行与测试

进入可执行文件目录：

```powershell
cd "C:\Users\spdf\Desktop\daer2\计网\实验\Lab1-2024(Win+Linux)\Lab1-2024(Win+Linux)\Lab1-Windows-VS2019\Win32-Debug"
```

每次测试需要打开两个 PowerShell，先启动 A 站，再启动 B 站。

无误码洪水模式：

```powershell
.\datalink.exe -t 60 -u -f -d3 --log report-logs\manual-sr-u-f-A.log A
.\datalink.exe -t 60 -u -f -d3 --log report-logs\manual-sr-u-f-B.log B
```

默认误码率 `1e-5`：

```powershell
.\datalink.exe -t 60 -f -d3 --log report-logs\manual-sr-default-f-A-v2.log A
.\datalink.exe -t 60 -f -d3 --log report-logs\manual-sr-default-f-B-v2.log B
```

高误码率 `1e-4` 压力测试：

```powershell
.\datalink.exe -t 60 -f --ber 1e-4 -d3 --log report-logs\manual-sr-ber1e4-f-A-v2.log A
.\datalink.exe -t 60 -f --ber 1e-4 -d3 --log report-logs\manual-sr-ber1e4-f-B-v2.log B
```

检查是否发生错误交付或异常退出：

```powershell
Select-String report-logs\manual-sr-*.log -Pattern "FATAL","Abort","bad packet"
```

提取性能统计：

```powershell
Select-String report-logs\manual-sr-default-f-A-v2.log -Pattern "packets received" | Select-Object -Last 5
```

## 当前测试结果

| 测试场景 | 站点 | 最后统计样例 | 结论 |
| --- | --- | --- | --- |
| 无误码 + 洪水模式，60 秒 | A | `224 packets, 7726 bps, 96.57%, Err 0` | 稳定，接近满载 |
| 默认误码率 `1e-5` + 洪水模式，60 秒 | A | `131 packets, 6815 bps, 85.19%, Err 3` | 无 FATAL，持续传输 |
| 默认误码率 `1e-5` + 洪水模式，60 秒 | B | `144 packets, 5089 bps, 63.61%, Err 5` | 无 FATAL，存在方向波动 |
| 高误码率 `1e-4` + 洪水模式，60 秒 | A | `18 packets, 4423 bps, 55.28%, Err 7` | 无 FATAL，性能明显下降 |
| 高误码率 `1e-4` + 洪水模式，60 秒 | B | 无统计输出 | 压力场景下该方向吞吐接近停滞 |

高误码率测试用于说明协议在压力环境下不会错误交付，但 NAK、超时和重传会显著增加，双向吞吐可能不均衡。这一现象可以作为实验报告“存在问题与改进方向”的材料。

## 小组报告建议

推荐三人分工：

```text
成员 A：协议设计、帧格式、状态变量、主事件循环
成员 B：测试方法、日志提取、性能表格、运行截图
成员 C：窗口/定时器参数推导、CRC/NAK 分析、问题与改进
```

报告重点：

- 从 Stop-and-Wait 到 GBN，再到 SR 的版本演进
- 为什么窗口取 8、序号空间取 128
- CRC 检错后如何通过 NAK 或超时重传恢复
- ACK timer 和 piggybacked ACK 如何减少独立 ACK
- 高误码率下吞吐下降的原因与改进方向
