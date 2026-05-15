# Go-Back-N 验收版

这个文件夹是独立可编译工程，供验收和队友直接阅读使用，不需要切换 Git 分支。

## 使用方法

1. 用 Visual Studio 打开本文件夹中的 datalink.sln。
2. 选择 Debug | Win32 编译。
3. 编译后进入本文件夹下的 Win32-Debug 运行 datalink.exe。

命令行编译示例：

`powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "datalink.sln" /p:Configuration=Debug /p:Platform=Win32 /m
`

运行时仍然需要两个 PowerShell 窗口，先 A 后 B。
