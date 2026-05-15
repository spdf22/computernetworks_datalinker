# 计算机网络实验一：数据链路层协议

本仓库用于保存数据链路层实验的代码、版本演进和测试说明。当前主版本实现的是：

```text
Selective Repeat + 捎带确认 + ACK 定时器 + NAK
```

`main` 分支等同于 `04` 分支，也就是最终验收版本。

## 版本结构

| 分支 | 协议版本 | 窗口设置 | 说明 |
| --- | --- | --- | --- |
| `00` | 停等协议 Stop-and-Wait | 发送窗口 1，接收窗口 1 | 基础可靠传输版本 |
| `01` | 基础 Go-Back-N | `MAX_SEQ=7`，发送窗口 `NR_BUFS=4`，接收窗口 1 | GBN 基础版本 |
| `02` | GBN + 捎带确认 + ACK 定时器 | `MAX_SEQ=7`，发送窗口 `NR_BUFS=4`，接收窗口 1 | 增加 piggyback ACK 和 ACK timer |
| `03` | GBN + NAK | `MAX_SEQ=15`，发送窗口 `NR_BUFS=8`，接收窗口 1 | 最终 GBN 对比版本 |
| `04` | Selective Repeat + 捎带确认 + ACK 定时器 + NAK | `MAX_SEQ=15`，发送窗口 `NR_BUFS=8`，接收窗口 8 | 最终 SR 验收版本 |

标签也提供了两套入口：

```text
v00, v01, v02, v03, v04
版本00-停等协议
版本01-基础GBN
版本02-GBN捎带确认
版本03-GBN最终对比
版本04-SR最终验收
```

## 直接阅读代码

如果只是阅读各版本代码，不想频繁切换分支，可以打开：

```text
版本代码对照/
```

其中包含每个版本的 `datalink.c` 快照：

```text
00-停等协议-datalink.c
01-基础GBN-datalink.c
02-GBN捎带确认ACK定时器-datalink.c
03-GBN最终对比NAK窗口8-datalink.c
04-SR最终验收选择重传-datalink.c
```

注意：这些文件只用于阅读。真正编译运行时，仍然使用工程目录中的：

```text
Lab1-2024(Win+Linux)/Lab1-2024(Win+Linux)/Lab1-Windows-VS2019/datalink.c
```

## 代码位置

主要工程目录：

```text
Lab1-2024(Win+Linux)/Lab1-2024(Win+Linux)/Lab1-Windows-VS2019/
```

核心文件：

```text
datalink.c   协议主体实现
datalink.h   帧类型定义
protocol.h   实验框架接口
protocol.c   网络层、物理层、信道和事件循环模拟
crc32.c      CRC32 校验
lprintf.c    日志输出
```

## 切换版本

进入仓库根目录：

```powershell
cd "C:\Users\spdf\Desktop\daer2\计网\实验"
```

查看当前版本：

```powershell
git branch --show-current
```

切换到最终 GBN 对比版本：

```powershell
git checkout 03
```

切换到最终 SR 验收版本：

```powershell
git checkout 04
```

每次切换版本后，都需要重新编译。

## 编译方法

使用 Visual Studio 2019/2022 打开：

```text
Lab1-2024(Win+Linux)/Lab1-2024(Win+Linux)/Lab1-Windows-VS2019/datalink.sln
```

也可以用命令行编译：

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "Lab1-2024(Win+Linux)\Lab1-2024(Win+Linux)\Lab1-Windows-VS2019\datalink.sln" /p:Configuration=Debug /p:Platform=Win32 /m
```

生成的可执行文件通常位于：

```text
Lab1-2024(Win+Linux)/Lab1-2024(Win+Linux)/Lab1-Windows-VS2019/Win32-Debug/datalink.exe
```

## 运行方法

进入可执行文件目录：

```powershell
cd "C:\Users\spdf\Desktop\daer2\计网\实验\Lab1-2024(Win+Linux)\Lab1-2024(Win+Linux)\Lab1-Windows-VS2019\Win32-Debug"
```

每次测试打开两个 PowerShell 窗口，先运行 A 端，再运行 B 端。

无误码，120 秒：

```powershell
.\datalink.exe -t 120 -u -f -d0 --log final-v04-u120-A.log A
.\datalink.exe -t 120 -u -f -d0 --log final-v04-u120-B.log B
```

正常误码，300 秒：

```powershell
.\datalink.exe -t 300 -f -d0 --log final-v04-default300-A.log A
.\datalink.exe -t 300 -f -d0 --log final-v04-default300-B.log B
```

高误码，180 秒：

```powershell
.\datalink.exe -t 180 -f --ber 1e-4 -d0 --log final-v04-ber1e4-180-A.log A
.\datalink.exe -t 180 -f --ber 1e-4 -d0 --log final-v04-ber1e4-180-B.log B
```

检查是否出现异常：

```powershell
Select-String final-v04-*.log -Pattern "FATAL","Abort","bad packet"
```

查看最后几条性能统计：

```powershell
Select-String final-v04-u120-A.log -Pattern "packets received" | Select-Object -Last 5
Select-String final-v04-u120-B.log -Pattern "packets received" | Select-Object -Last 5
```

## 参数说明

| 参数 | 含义 |
| --- | --- |
| `A` / `B` | 指定运行站点 A 或站点 B |
| `-t 120` | 运行 120 秒 |
| `-u` | 无误码信道，也就是 utopia 模式 |
| `-f` | flood 模式，网络层持续产生分组，用于测试吞吐率 |
| `--ber 1e-4` | 指定误码率为 `10^-4` |
| `-d0` | 关闭详细调试输出，适合性能测试 |
| `-d3` | 输出 DATA/ACK/NAK 等调试日志，适合观察协议过程 |
| `--log xxx.log` | 指定日志文件名 |

## 最终 SR 设计摘要

最终 SR 版本位于 `04` 分支，主要机制如下：

- 使用 CRC32 检测帧错误。
- 发送窗口大小为 8。
- 接收窗口大小为 8。
- 序号空间为 `0..15`，即 `MAX_SEQ=15`。
- 数据帧支持捎带 ACK。
- 使用 ACK 定时器延迟独立 ACK，尽量等待数据帧捎带确认。
- 发现坏帧或缺口时发送 NAK，加快恢复。
- 接收方缓存窗口内乱序帧，只有从 `frame_expected` 开始连续到达时才交付网络层。
- 发送方只重传未确认的指定帧，避免 Go-Back-N 的批量回退浪费。

SR 的窗口大小选择满足：

```text
发送窗口 <= 序号空间 / 2
```

因此 `MAX_SEQ=15` 时，序号空间大小为 16，SR 最大安全窗口为 8。

## GBN 与 SR 对比说明

最终对比时主要使用：

```text
03：最终 GBN 对比版本
04：最终 SR 验收版本
```

两者都采用发送窗口 8，便于公平比较。

GBN 接收窗口为 1，乱序帧会被丢弃；遇到错误后可能回退重传多个帧。SR 接收窗口为 8，能够缓存乱序帧，只重传出错或丢失的帧，因此在有误码场景下通常利用率更高。

## 推荐验收讲法

可以按这个顺序介绍：

1. `00` 实现停等协议，作为可靠传输基础。
2. `01` 实现基础 GBN，引入滑动窗口和累计确认。
3. `02` 在 GBN 上加入捎带确认和 ACK 定时器，减少独立 ACK。
4. `03` 在 GBN 上加入 NAK，并把发送窗口调整为 8，用于最终 GBN 性能对比。
5. `04` 实现 SR，支持乱序缓存、选择确认、选择重传、NAK 和 ACK timer，是最终验收版本。

测试时重点说明：

- 无误码下，窗口协议能显著提高信道利用率。
- 正常误码下，SR 能通过选择重传减少不必要重传。
- 高误码下，GBN 会因为回退重传浪费较多，SR 性能通常更稳定。
