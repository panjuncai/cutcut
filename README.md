# CutCut

CutCut 是一个基于 Qt Widgets 的轻量桌面截图工具，常驻系统托盘，提供“框选截图复制”和“截图钉在桌面顶层”两个核心能力。

当前仓库主要面向 macOS 和 Windows，Linux 暂未实现全局热键注册逻辑。

## 功能特性

- `F1`：启动全屏选区截图
- `F3`：将最近一次截图，或当前选区，钉在桌面顶层
- 截图完成后自动复制到系统剪贴板
- 选区完成后显示悬浮工具栏，支持 `Copy` / `Pin`
- 钉图窗口支持拖动位置，双击关闭
- 托盘菜单支持手动触发截图、钉图和退出
- 支持多屏截图，按虚拟桌面区域一次性拼接抓取

## 交互说明

1. 启动应用后，程序会驻留在系统托盘，不主动显示主窗口。
2. 按 `F1` 进入截图模式，拖拽选择区域。
3. 松开鼠标后会出现操作条：
   - `Copy`：复制选区到剪贴板
   - `Pin`：复制并将图片钉在桌面顶层
4. 如果刚完成截图，也可以直接按 `F3` 将最近一次截图钉住。
5. 钉图窗口可拖动，双击即可关闭。

## 技术栈

- C++20
- CMake 3.21+
- Qt 5 / Qt 6
  - `Widgets`
  - `Gui`
- 平台相关能力
  - macOS：Carbon 全局热键
  - Windows：Win32 `RegisterHotKey`

## 项目结构

```text
.
├── CMakeLists.txt
├── src
│   ├── main.cpp
│   ├── appcontroller.{h,cpp}
│   ├── screenshotoverlay.{h,cpp}
│   ├── selectiontoolbar.{h,cpp}
│   └── pinnedshot.{h,cpp}
├── installer
│   ├── cutcut.ico
│   └── cutcut.wxs
└── .github
    └── workflows
        └── build.yml
```

各模块职责：

- `main.cpp`：应用入口，初始化 `QApplication` 与控制器
- `appcontroller`：托盘、全局热键、截图流程编排、剪贴板和钉图管理
- `screenshotoverlay`：截图遮罩层、选区绘制、键鼠交互
- `selectiontoolbar`：选区完成后的悬浮操作条
- `pinnedshot`：顶层钉图窗口

## 构建要求

### 通用

- CMake `>= 3.21`
- 支持 C++20 的编译器
- Qt 5 或 Qt 6，且包含 `Widgets` 与 `Gui`

### macOS

- 需要 Xcode Command Line Tools
- 构建时会链接 `Carbon.framework`
- 首次运行时通常需要授予“屏幕录制”权限，否则无法正确抓屏

### Windows

- 建议使用 MSVC + Ninja
- 打包运行时依赖时可使用 `windeployqt`

## 本地构建

### macOS / Linux

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Windows

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

生成的可执行文件位于 `build/` 目录下，名称为 `cutcut` 或 `cutcut.exe`。

## 运行

直接运行构建产物即可：

```bash
./build/cutcut
```

Windows 下运行：

```powershell
.\build\cutcut.exe
```

启动后程序会驻留在系统托盘。

## CI 与产物

仓库内置 GitHub Actions 构建流程：

- macOS：分别构建 `arm64` 和 `x86_64`，合并为通用二进制，并打包为 `.dmg`
- Windows：构建便携版，并打包为 `.zip`

相关配置见 [`.github/workflows/build.yml`](./.github/workflows/build.yml)。

## 平台支持说明

- macOS：已实现
- Windows：已实现
- Linux：当前 `registerHotkeys()` 未实现，程序初始化会失败，暂不支持

## 打包说明

- GitHub Actions 已覆盖 macOS 和 Windows 便携版产物输出
- `installer/cutcut.wxs` 提供了 WiX 安装器描述文件草稿，可作为 Windows 安装包方向的基础，但当前仓库默认发布流程仍以便携版 ZIP 为主

## 后续可扩展方向

- 自定义热键
- 截图保存到文件
- 标注、画框、文字等编辑能力
- 钉图缩放、透明度、锁定点击
- Linux 平台支持

## License

仓库当前未提供明确的开源许可证；如需公开分发，建议补充 `LICENSE` 文件。
