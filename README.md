# JackalExtensionSDK

JackalClient 扩展开发 SDK。用 C/C++ 编写原生模块，或参考客户端接口设计 Lua、Jackal 批处理扩展，实现模块联动、状态查询、网络请求和 Custom HUD。

[完整教程与接口说明](https://doc.wormwake.com/extension-sdk.html)

## 1. 选择开发方式

| 方式 | 适合的功能 | 准备内容 |
| --- | --- | --- |
| Lua | 状态观察、模块联动、HTTP、HUD | 支持脚本扩展的 JackalClient、文本编辑器 |
| Jackal 批处理 | 顺序命令、延时操作、简单接口调用 | 支持脚本扩展的 JackalClient、文本编辑器 |
| C/C++ | 原生功能、现有 C/C++ 库集成 | Windows、Visual Studio C++、CMake |

Lua 使用客户端内置的全局 `jackal` 表；批处理通过 `/api` 调用同一套客户端接口。本仓库的两个头文件供原生扩展使用。

## 2. 仓库内容

| 文件 | 用途 |
| --- | --- |
| `jackal_extension.h` | C ABI：宿主结构、查询与异步操作函数表、生命周期函数类型 |
| `jackal_extension.hpp` | C++17 封装：`jackal::Host`，依赖 nlohmann-json |
| `README.md` | 开发入门、接口示例与发布说明 |

SDK 为头文件形式，ABI/API 当前均为版本 1。把此目录加入编译器的 include 路径即可使用。

## 3. 接入 C++ 工程

提前下载 SDK，将 `JackalExtensionSDK` 与扩展目录放在同一父目录下。三个示例共用这一份 SDK：

```text
workspace/
├─ JackalExtensionSDK/
│  ├─ jackal_extension.h
│  └─ jackal_extension.hpp
├─ example-module/
│  ├─ CMakeLists.txt
│  ├─ example.cpp
│  ├─ example.def
│  └─ extension.json
├─ example-lua/
└─ example-batch/
```

原生示例的 CMake 使用 `../JackalExtensionSDK/` 作为头文件搜索路径。在上述父目录执行：

```powershell
cmake -S ./example-module -B ./build/example-x64 -A x64 -DCMAKE_PREFIX_PATH="$env:VCPKG_ROOT/installed/x64-windows-static"
cmake --build ./build/example-x64 --config Release
```

把 vcpkg 前缀替换为自己的位置。安装依赖可运行 `vcpkg install nlohmann-json:x64-windows-static nlohmann-json:x86-windows-static`。构建 x86 时使用独立构建目录、`-A Win32` 和 `x86-windows-static` 前缀。原生示例在扩展目录中直接输出两种架构的 DLL。

自建 CMake 工程时，设置 C++17、链接 `nlohmann_json::nlohmann_json`，并使用：

```cmake
target_include_directories(your-extension PRIVATE ../JackalExtensionSDK)
```

源码使用 `#include "jackal_extension.hpp"`，也可以直接按相对路径写成 `#include "../JackalExtensionSDK/jackal_extension.hpp"`。

## 4. 使用客户端接口

在 `JcExtensionInitialize` 保存宿主接口，随后在模块生命周期回调中查询状态：

```cpp
#include "jackal_extension.hpp"

jackal::Host host;

extern "C" int __cdecl JcExtensionInitialize(const JcExtensionHostV1* environment) {
    return host.initialize(environment) ? JC_EXTENSION_OK : JC_EXTENSION_ERROR;
}
```

完整模块还需要版本导出、启用和停用函数，以及清单中对应的 `behaviors`。可从 `example-module` 的源码、`.def` 和清单开始修改；x86 使用 `.def` 导出未修饰的 C 函数名。导出函数应在内部处理异常并返回结果码。

常用查询示例：

```cpp
const std::string fps = host.variable("fps");
const bool hasClock = host.moduleExists("Clock");
const bool shortcut = host.keyDown(std::vector<int>{0x11, 0x4B});
const auto permissions = host.permissions();
const auto windows = host.privateWindows();
const auto elements = host.hudElements();
```

模块名称使用客户端原始英文名，按键使用 Windows 虚拟键值。查询在客户端生命周期回调中执行。

通知、执行命令、切换模块、修改配置、HTTP 和 HUD 写入采用异步操作。保存返回的票据，在后续 update 中读取结果：

```cpp
const uint64_t ticket = host.notify("Hello from my extension", "success");
```

```cpp
const auto reply = host.requestResult(ticket);
if (reply.value("ready", false)) {
    const auto& result = reply.at("result");
    if (result.value("ok", false)) {
        const auto value = result.at("value");
    } else {
        const std::string error = result.at("error").get<std::string>();
    }
}
```

操作完成后清除自己保存的票据。`host.info()` 的 `capabilities` 列出当前客户端支持的查询和操作；`host.get(request)`、`host.post(request)` 可调用完整的 JSON 接口。

## 5. 从脚本开始

Lua 示例 `main.lua` 的启用回调可以直接读取 FPS 并发送通知：

```lua
function on_enable(config, context)
    jackal.notify("FPS: " .. jackal.variables.get("fps"), "success", context.module)
end

function on_disable(config, context)
    jackal.notify("Stopped", "info", context.module)
end
```

在清单中使用 `schemaVersion: 2`，设置 `script.language` 为 `lua`、`script.entry` 为脚本的文件逻辑键，`behaviors` 关联上述函数名。

Jackal 批处理设置 `script.language` 为 `jbat`，`behaviors` 关联 `.jbat` 的文件逻辑键：

```text
/api {"method":"variable.get","name":"fps"}
/api {"method":"notify","text":{"$ref":"/last"},"title":"FPS"}
```

可直接从 `example-lua` 或 `example-batch` 的完整清单开始。脚本使用 UTF-8，配置默认值写在模块的 `defaults` 中；扩展自有翻译写入 `Dictionary.txt` 并通过 `contributes.Dictionary` 登记。

## 6. 安装与发布

在客户端 **扩展 (Extensions) → 导入本地 (Import local...)** 选择包含 `extension.json` 的扩展目录或 ZIP。首次安装后，新增模块出现在清单指定的分类中，手动开启即可使用。更新、禁用和删除扩展在重启客户端后生效；模块设置保存在当前配置档案。

本地开发时，清单可省略各文件的 `size`、`sha256`，导入时由客户端计算；已有校验值需要与当前文件一致。发布前，为每个 `files` 条目填写实际字节数和 SHA-256，例如：

```powershell
(Get-Item -LiteralPath ./main.lua).Length
(Get-FileHash -LiteralPath ./main.lua -Algorithm SHA256).Hash.ToLowerInvariant()
```

将文件名换成清单登记的每一个文件。发布包根目录包含 `extension.json` 及其登记的全部文件；原生示例同时登记 x86、x64，因此两份 DLL 均需提供。

