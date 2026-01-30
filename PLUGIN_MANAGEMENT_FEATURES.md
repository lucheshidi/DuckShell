# DuckShell 插件管理功能增强说明

## 新增功能

本次更新为DuckShell添加了以下插件管理命令：

### 1. remove 命令
**功能**：完全删除插件（包括从列表中移除和删除物理文件）
**用法**：`plugin remove <plugin_name>`
**示例**：
```
plugin remove HelloWorld
plugin remove myplugin
```

### 2. enable 命令  
**功能**：启用已安装但被禁用的插件
**用法**：`plugin enable <plugin_name>`
**示例**：
```
plugin enable HelloWorld
plugin enable myplugin
```

### 3. disable 命令
**功能**：禁用已安装的插件（插件文件仍存在，但不会被加载）
**用法**：`plugin disable <plugin_name>`
**示例**：
```
plugin disable HelloWorld
plugin disable myplugin
```

### 4. 改进的 uninstall 命令
**功能**：卸载插件（从插件列表中移除，但保留物理文件）
**改进**：现在支持不带扩展名的插件名输入
**用法**：`plugin uninstall <plugin_name>`
**示例**：
```
# 现在两种方式都可以工作
plugin uninstall HelloWorld      # 推荐方式
plugin uninstall HelloWorld.dll  # 旧方式仍然支持
```

## 命令对比

| 命令 | 功能 | 文件操作 | 列表操作 |
|------|------|----------|----------|
| `install` | 安装插件 | 无 | 添加到列表 |
| `uninstall` | 卸载插件 | 无 | 从列表移除 |
| `remove` | 删除插件 | 删除文件 | 从列表移除 |
| `enable` | 启用插件 | 无 | 修改状态 |
| `disable` | 禁用插件 | 无 | 修改状态 |

## 使用示例

```
# 查看帮助
plugin

# 列出所有已安装插件及其状态
plugin list

# 安装新插件
plugin install MyPlugin

# 禁用插件（临时停用）
plugin disable MyPlugin

# 启用插件
plugin enable MyPlugin

# 卸载插件（保留文件）
plugin uninstall MyPlugin

# 完全删除插件
plugin remove MyPlugin

# 查看可用插件
plugin available

# 设置插件仓库
plugin repo https://my-repo.example.com
```

## 技术实现细节

### 文件结构
- 插件列表文件：`~/duckshell/plugins.ls`
- 插件存储目录：`~/duckshell/plugins/`

### 插件列表格式
```
plugin_name1:enabled
plugin_name2:disabled
plugin_name3:enabled
```

### 跨平台支持
所有新功能都支持Windows和Unix/Linux平台：
- Windows: 使用`.dll`扩展名
- Linux/macOS: 使用`.so`扩展名

### 错误处理
- 插件不存在时会显示友好提示
- 文件操作失败时会显示具体错误信息
- 支持自动解析插件名称（带或不带扩展名）

## 注意事项

1. **权限要求**：删除插件文件需要相应目录的写权限
2. **依赖关系**：删除插件前请确认没有其他插件依赖它
3. **备份建议**：重要插件建议先备份再删除
4. **状态同步**：启用/禁用操作会立即生效，无需重启Shell

## 故障排除

如果遇到问题，请检查：
1. 插件文件是否存在且可访问
2. 插件列表文件权限是否正确
3. 输入的插件名称是否正确
4. 是否有足够的权限执行操作