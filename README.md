# GameLib for ESP32 (GameLib_MCU)

## 简介
gamelib_for_esp32 是一个专为 ESP32 (重点支持 ESP32-S3 等 MCU) 平台打造的轻量级 2D 游戏开发框架。
项目深受 [Skywind3000 的开源项目 GameLib](https://github.com/skywind3000/GameLib) 启发，保留了极其简单的开发体验，并将其底层完全使用 **纯 C 语言** 重写与解耦。基于 ESP-IDF 框架构建，致力于让开发者能够在嵌入式单片机（MCU）上极速开发 2D 游戏、图形演示或者互动程序。

## 核心特性
* **纯 C 语言构建**：极低的性能开销与内存占用，专为资源受限的 MCU 单片机环境优化。
* **硬件抽象层（HAL）架构**：核心游戏逻辑与硬件彻底分离。不仅能平滑运行支持 ESP32-S3 等芯片，也使移植到 STM32 等其他 MCU 或 PC 上位机变得轻而易举。
* **完善的 2D 游戏组件引擎**：
  * **图形渲染**：基础几何图形的描边与填充绘制。
  * **精灵系统 (Sprite)**：内建 PNG/BMP 图像解析加载、缩放渲染。
  * **动画模块 (Animator)**：基于时间戳 (ms) 控制的多帧动画状态机配置，简化角色行为与动画匹配逻辑。
  * **游戏摄像机 (Camera)**：提供 2D 游戏世界的视口管理与目标跟随裁切引擎，轻松实现大地图卷轴漫游。
  * **碰撞检测系统 (Collision)**：针对多层 Tilemap 的物理边界计算以及 AABB 矩形碰撞判定。
  * **瓦片地图 (Tilemap)**：支持多图层的高效背景瓦片地图渲染系统。
  * **输入处理控制**：封装了按键的 Down / Pressed / Released 边缘检测逻辑。

## 目录结构
```text
.
├── components/
│   ├── gamelib_core/        # 游戏库独立核心层（动画, 摄像机, 碰撞, 精灵, 渲染控制，硬件无关）
│   ├── gamelib_hal/         # 硬件抽象层接口标准申明 (显示、输入、文件系统接口)
│   └── gamelib_port_esp32s3/# 针对 ESP32-S3 平台的物理驱动实现（LCD 驱动刷屏、引脚按钮扫描等）
├── main/
│   ├── main.c               # ESP-IDF 项目应用主入口
│   └── demos/               # 丰富的实战游戏示例源码 (例如: RPG演示 demo_17_rpg_demo)
├── tools/                   # 辅助开发工具集
│   ├── tiled2c.py           # 可将 Tiled 地图导出数据转为 C 数组的 Python 脚本
│   ├── texture_packer.html  # 网页版: 精灵图集拼接打包工具
│   ├── sprite-splitter.html # 网页版: 图集切割拆分工具
│   └── image_scaler.html    # 网页版: 像素图像纯净缩放工具
├── GameLib/                 # 上位机原版 C++ GameLib 库 (用作对照、参考或文档指引)
└── README.md                # 本说明文件
```

## 实战示例演示
在 main/demos/ 目录下提供了多个由浅入深的演示代码，可直接以此为模板二次开发：
* 例如 **demo_17_rpg_demo.c** 完整演示了一个带有物理摄像机跟随追踪、多方向攻击巡逻精灵帧动画状态机、以及跨层地形阻挡判定检测的 Action-RPG （动作角色扮演）游戏骨骼逻辑。

## 快速上手与编译 (基于 ESP-IDF)

### 1. 环境准备
* 推荐安装 [ESP-IDF](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/get-started/) 开发环境 (推荐 v5.x 版本)。
* 开发板要求：包含可用彩色 LCD 屏幕与按键模块的 ESP32 开发板。请视具体硬件情况在 gamelib_port_esp32s3 内修改对应屏幕 SPI/I80 驱动及物理 IO 引脚映射。

### 2. 编译与烧录
在集成好 ESP-IDF 环境的终端（或开启 IDF 插件的 VS Code）中执行：

# 设定目标平台芯片 (如 esp32s3)
idf.py set-target esp32s3

# 编译项目
idf.py build

# 烧录到您的开发板并通过串口查看日志
idf.py flash monitor

*(在 VS Code 环境中，您也可以直接使用左下角状态栏的 ESP-IDF 快捷按钮一键执行 Build、Flash 与 Monitor)*

## 辅助开发工具箱
根目录的 	tools/ 附带了一套对于像素以及嵌入式游戏十分实用的离线工具包：
* 各类 .html 文件为纯前端独立工具应用，双击以浏览器打开，即可极速实现美术素材的等比放大、贴图重组与切分处理。

## 致谢
* 本项目 API 接口逻辑设计与极简体验理念深刻源于 [skywind3000/GameLib](https://github.com/skywind3000/GameLib)。在此向原作者表示感谢与敬意！

## 许可协议
本项目秉承原版精神，基于 [MIT 协议](./GameLib/LICENSE) 开源。
允许自由分发、修改，并广泛支持各种商业与非商用硬件项目产品。

