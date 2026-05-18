# Tiled 地图编辑器集成 — 设计规格

## 目标

以 Tiled 作为 ESP32 GameLib 项目的可视化地图编辑器，实现从 `.tmx`/`.tsx` 到 C 代码的完整转换管线，支持图层数据、tile 属性（自动发现）、对象层。

## 核心原则

- **属性自动发现**：转换器扫描 Tiled 中的所有自定义属性名，自动生成 C 数据结构，新增属性无需修改转换器代码
- **零运行时解析**：所有数据在编译时转为 `const` 数组，ESP32 端无需 XML/JSON 解析
- **多图集兼容**：支持一个地图引用多个 tileset

## 数据流

```
Tiled 编辑器 ──(.tmx/.tsx)──→ tiled2c.py ──→ map_data.h / map_data.c ──→ ESP32 编译
```

## 转换器设计 (tiled2c.py)

### 输入

- `map.tmx` — 地图主文件（XML），包含图层数据、对象层、tileset 引用、导出设置
- `tilemap.tsx` — 图集定义（XML），包含 tile 属性、图片路径、列数、tilecount

### 提取的信息

| TMX/TSX 字段 | 用途 |
|:---|:---|
| `map.width/height` | 地图尺寸 #define |
| `map.tilewidth/tileheight` | 图块尺寸 #define |
| `map.orientation` | 地图方向（固定 orthogonal） |
| `layer.data` (CSV) | 每层 tile GID 数组 |
| `layer.name` | 图层变量名 |
| `tileset.tilecount` | tileset 总 tile 数 |
| `tileset.columns` | 图集列数（计算 tile 在图集中的行列） |
| `tileset.image.source/width/height` | 图集图片信息 |
| `tileset.tile[].properties` | **所有** tile 自定义属性（自动发现属性名） |
| `objectgroup.objects` | 对象层实体 |

### 输出

生成 `map_data.h` 和 `map_data.c`，内容见下方。

---

## 生成的数据格式

### 1. 地图元数据

```c
#define MAP_WIDTH        30
#define MAP_HEIGHT       20
#define MAP_TILE_WIDTH   16
#define MAP_TILE_HEIGHT  16
#define MAP_TILESET_FIRSTGID  1
```

### 2. 图层数据

```c
extern const int16_t map_layer_<layername>[MAP_WIDTH * MAP_HEIGHT];
```

每个图层生成一个 `int16_t[]` 数组，tile GID 从 CSV 直接填入，空 tile 为 0。

### 3. Tile 属性 (按类型 + 属性名自动生成)

三种类型各有不同的存储格式：

**Bool 属性 → uint32_t 位掩码**

```c
#define TILEPROP_<propname>_EXISTS  1
#define TILEPROP_<propname>_WORDS   <ceil(tilecount/32)>
extern const uint32_t tileprop_<propname>[TILEPROP_<propname>_WORDS];

// 注意: tile_idx 是 0-based 局部编号, 需从图层 GID 减 firstgid 得到
#define TILE_HAS_<PROPNAME>(tile_idx) \
    ((tileprop_<propname>[(tile_idx) >> 5] >> ((tile_idx) & 0x1F)) & 1)
```

**Int 属性 → int32_t 数组**

```c
#define TILEPROP_<propname>_EXISTS  1
extern const int32_t tileprop_<propname>[<tilecount>];

#define TILE_GET_<PROPNAME>(tile_idx)  (tileprop_<propname>[tile_idx])
```

**Float 属性 → float 数组**

```c
#define TILEPROP_<propname>_EXISTS  1
extern const float tileprop_<propname>[<tilecount>];

#define TILE_GET_<PROPNAME>(tile_idx)  (tileprop_<propname>[tile_idx])
```

- `gid` 是 tile 在图集内的局部 ID (0-based)，使用时需减 `firstgid`
- 未设置属性的 tile 默认值：bool→false, int→0, float→0.0f

### 4. 对象层数据

按对象 `type` 字段分组，每组生成结构体数组：

```c
#define MAP_OBJ_<TYPE>_COUNT  <N>
extern const map_obj_t map_obj_<type>[MAP_OBJ_<TYPE>_COUNT];
```

`map_obj_t` 结构：

```c
typedef struct {
    int   id;
    float x, y;
    float w, h;
    char  name[32];
} map_obj_t;
```

对象属性同样按类型自动生成：

```c
extern const int32_t obj_<type>_<propname>[MAP_OBJ_<TYPE>_COUNT];
extern const float  obj_<type>_<propname>[MAP_OBJ_<TYPE>_COUNT];
// bool 同理
```

---

## 运行时 API (gamelib_core 新增)

### gamelib.h 新增声明

```c
// === Tile 属性查询（运行时动态查询） ===
bool  gamelib_tile_prop_bool (int firstgid, const char *name, int tile_gid);
int   gamelib_tile_prop_int  (int firstgid, const char *name, int tile_gid);
float gamelib_tile_prop_float(int firstgid, const char *name, int tile_gid);
```

### 使用方式

- **快速访问**：直接使用生成的 `TILE_HAS_OBSTACLE(tile_idx)` / `TILE_GET_DAMAGE(tile_idx)` 宏，编译时确定属性名，零开销
- **动态查询**：当属性名是运行时变量时（如脚本系统），使用 `gamelib_tile_prop_xxx()` 函数，内部通过字符串匹配查找属性数组
- **对象遍历**：直接使用生成的 `MAP_OBJ_<TYPE>_COUNT` 和 `map_obj_<type>[]` 数组，`for` 循环即可，无需迭代器 API

### 实现要点

- 属性名到数组的映射通过 `map_data.c` 中以属性名为 key 的静态查找表实现
- 对象数据为 `const` 静态数组，直接通过生成的数组名访问

---

## 工作流

### 日常使用

1. Tiled 中编辑地图、标记属性、放置对象
2. 保存 `.tmx`
3. 运行：`python tiled2c.py map.tmx map_data.c map_data.h`
4. `idf.py build && idf.py flash`

### Tiled 自定义命令（可选，一键导出）

Tiled → 编辑 → 命令 → 添加：

| 字段 | 值 |
|:---|:---|
| 可执行文件 | `python` |
| 参数 | `tiled2c.py %mapfile map_data.c map_data.h` |
| 工作目录 | `%mappath` |
| 快捷键 | `Ctrl+Shift+E` |

### demo 集成

`main/CMakeLists.txt` 需将 `map_data.c` 加入编译源文件列表（或 demo 中 `#include "map_data.c"` 方式内联）。

---

## 限制与约定

1. **不支持 Tiled 的 `infinite` 地图** — 仅支持固定尺寸地图
2. **仅支持 CSV 编码的图层数据** — base64/zlib 编码不支持
3. **对象 type 名长度限制** — 实际受 C 标识符长度限制，建议不超过 31 字符
4. **对象 name 字段** — 固定 32 字节，超出截断
5. **Tile 属性仅支持 bool/int/float** — string/color/file/object 类型暂不支持（可在后续版本添加）
6. **CSV 中连续逗号视为0** — Tiled 导出时空 tile 为 0

---

## 涉及文件

| 文件 | 操作 |
|:---|:---|
| `tools/tiled2c.py` | **重写** — 新的通用转换器 |
| `components/gamelib_core/gamelib.h` | **修改** — 新增属性/对象 API 声明 |
| `components/gamelib_core/gamelib.c` | **修改** — 新增属性/对象 API 实现 |
| `main/demos/demo_17_rpg_demo.c` | **修改** — 使用新 API 加载 tilemap |
| `main/CMakeLists.txt` | **修改** — 添加 map_data.c 到编译 |
| `main/demos/*/map_data.h` | **生成** — 转换器输出 |
| `main/demos/*/map_data.c` | **生成** — 转换器输出 |
