# CutCut

CutCut 是一个基于 Qt Widgets 的轻量桌面截图工具，常驻系统托盘，支持截图、钉图，以及常见截图标注能力。

当前仓库主要面向 macOS 和 Windows，Linux 暂未实现全局热键注册逻辑。

## 功能特性

- `F1`：启动全屏选区截图
- `F3`：将最近一次截图，或当前选区，钉在桌面顶层
- 截图完成后自动复制到系统剪贴板
- 支持多屏截图，按虚拟桌面区域一次性拼接抓取
- 已选区支持整体拖动和四边/四角缩放
- 截图后显示底部悬浮工具条，支持拖动
- 标注工具：
  - 矩形框
  - 箭头指示
  - 文字输入
  - 马赛克
- 文字工具支持：
  - 字号选择
  - 字体颜色选择
- 支持撤回最近一次标注
- 钉图窗口支持拖动位置，双击关闭
- 托盘菜单支持手动触发截图、钉图和退出

## 交互说明

1. 启动应用后，程序会驻留在系统托盘，不主动显示主窗口。
2. 按 `F1` 进入截图模式，拖拽选择区域。
3. 松开鼠标后会出现悬浮工具条：
   - 矩形：在截图区域内拖出红色矩形框
   - 箭头：拖出红色箭头用于指示
   - 文字：点击截图区域输入文字，并可设置字号和颜色
   - 马赛克：拖出区域进行像素化遮挡
   - 撤回：按顺序撤销最近一次标注
   - Pin：将当前截图钉在桌面顶层
   - Copy：复制当前截图到剪贴板
   - X：取消当前截图
4. 如果刚完成截图，也可以直接按 `F3` 将最近一次截图钉住。
5. 所有标注都会一并进入复制结果和钉图结果。
6. 钉图窗口可拖动，双击即可关闭。

## 技术栈

- C++20
- CMake 3.21+
- Qt 5 / Qt 6
  - `Widgets`
  - `Gui`
  - `Svg`
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
- `screenshotoverlay`：截图遮罩层、选区绘制、标注、键鼠交互
- `selectiontoolbar`：截图后悬浮工具条、文字设置条和图标按钮
- `pinnedshot`：顶层钉图窗口

## 构建要求

### 通用

- CMake `>= 3.21`
- 支持 C++20 的编译器
- Qt 5 或 Qt 6，且包含：
  - `Widgets`
  - `Gui`
  - `Svg`

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

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

生成的可执行文件位于 `build/` 目录下，名称为 `cutcut` 或 `cutcut.exe`。

## 运行

macOS / Linux：

```bash
./build/cutcut
```

Windows：

```powershell
.\build\cutcut.exe
```

启动后程序会驻留在系统托盘。

## 调试预览

推荐直接本地运行可执行文件后，通过热键预览交互：

- 按 `F1` 开始截图
- 拖选区域
- 测试移动和缩放选区
- 测试矩形、箭头、文字、马赛克和撤回
- 点 `Copy` / `Pin` 查看结果是否符合预期

如果你在 macOS 上调试：

- 第一次运行通常需要给终端或应用授予“屏幕录制”权限
- `F1` / `F3` 如果被系统功能键占用，可能需要配合 `fn`

## CI 与产物

仓库内置 GitHub Actions 构建流程：

- macOS：分别构建 `arm64` 和 `x86_64`，合并为通用二进制，并打包为 `.dmg`
- Windows：构建便携版，并打包为 `.zip`
- Tag 发布：推送 `v*` tag 后自动创建 GitHub Release

相关配置见 [`.github/workflows/build.yml`](./.github/workflows/build.yml)。

## 发布

当前仓库已经配置好 tag 发布流程。发布正式版时，通常执行：

```bash
git add .
git commit -m "release: v1.0.0"
git push origin main
git tag v1.0.0
git push origin v1.0.0
```

推送 `v1.0.0` tag 后，GitHub Actions 会自动构建并发布：

- `CutCut.dmg`
- `CutCut-portable.zip`

## 平台支持说明

- macOS：已实现
- Windows：已实现
- Linux：当前 `registerHotkeys()` 未实现，程序初始化会失败，暂不支持

## 打包说明

- GitHub Actions 已覆盖 macOS 和 Windows 便携版产物输出
- `installer/cutcut.wxs` 提供了 WiX 安装器描述文件，可作为 Windows 安装包方向的基础

## 后续可扩展方向

- 更多标注工具
- 标注对象二次编辑
- 自定义热键
- 截图保存到文件
- 钉图缩放、透明度、锁定点击
- Linux 平台支持

## License

本项目基于 [MIT License](./LICENSE) 开源。
