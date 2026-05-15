# 版本代码对照

这个文件夹用于直接阅读各个协议版本的 `datalink.c`，不用频繁切换 Git 分支。

这些文件是从对应分支导出的代码快照，只用于对照阅读和讨论；真正编译运行时，仍然使用工程目录中的：

```text
Lab1-2024(Win+Linux)/Lab1-2024(Win+Linux)/Lab1-Windows-VS2019/datalink.c
```

## 文件对应关系

| 文件 | 对应分支 | 协议内容 |
| --- | --- | --- |
| `00-停等协议-datalink.c` | `00` | 停等协议 Stop-and-Wait |
| `01-基础GBN-datalink.c` | `01` | 基础 Go-Back-N |
| `02-GBN捎带确认ACK定时器-datalink.c` | `02` | GBN + 捎带确认 + ACK 定时器 |
| `03-GBN最终对比NAK窗口8-datalink.c` | `03` | GBN + NAK，发送窗口为 8 |
| `04-SR最终验收选择重传-datalink.c` | `04` | Selective Repeat + 捎带确认 + ACK 定时器 + NAK |

## 推荐阅读顺序

1. 先看 `00-停等协议-datalink.c`，理解最基本的发送、接收、超时重传。
2. 再看 `01-基础GBN-datalink.c`，关注滑动窗口、累计确认、批量重传。
3. 再看 `02-GBN捎带确认ACK定时器-datalink.c`，关注 ACK 如何被捎带，以及 ACK timer 为什么存在。
4. 再看 `03-GBN最终对比NAK窗口8-datalink.c`，关注 NAK 如何让 GBN 更快发现丢帧或错帧。
5. 最后看 `04-SR最终验收选择重传-datalink.c`，关注接收窗口、乱序缓存、选择确认和选择重传。

## 编译提醒

不要直接编译本文件夹里的代码快照。

如果要运行某个版本，请回到仓库根目录切换分支，例如：

```powershell
git checkout 03
```

然后重新编译 Visual Studio 工程。
