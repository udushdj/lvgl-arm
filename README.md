# 医疗叫号系统 - 函数功能描述文档

## 项目概述

该项目实现四个界面：
- **Screen1**（主叫号界面）：显示当前患者、等待队列、下一位患者
- **Screen2**（患者管理界面）：搜索、排序、置顶患者，查看就诊记录
- **Screen3**（消息发送界面）：通过DeepSeek AI API进行对话
- **Screen4**（统计看板）：今日叫号数、平均等待时间、近8小时流量柱状图

### 服务器对接

系统通过HTTP与 **tiny-httpd**（轻量化HTTP服务器CGI引擎）同步数据：
- `GET /patient_queue.json` → 患者队列数据
- `GET /stats.json` → 统计数据（叫号数、等待时间、就诊记录）
- `POST /cgi-bin/api` → 远程管理端写入患者数据
- 同步间隔：每5秒自动拉取一次

服务器项目地址：`\\wsl.localhost\Ubuntu-22.04\home\wz\codepath\轻量化http服务器CGI引擎`

项目启动逻辑：初始化板子配置 → HTTP同步患者队列 → 加载4个UI界面 → 进入主事件循环。

---
## 中文字体

### 运行时中文字符串清单

| 界面 | 字符串 | 用途 |
|------|--------|------|
| Screen1 | `"当前科室：B12-131"` | 顶栏科室信息 |
| Screen1 | `"暂无\n患者"` | 当前患者占位文字 |
| Screen1 | `"同步"` | 同步按钮 |
| Screen1 | `"患者详情"` | 详情按钮 |
| Screen1 | `"消息发送"` | 跳转Screen3按钮 |
| Screen1 | `"数据统计"` | 跳转Screen4按钮 |
| Screen1 | `"消息同步"` | 消息同步功能 |
| Screen1 | `"暂无候诊患者"` | 候诊队列为空时 |
| Screen1 | `"无当前患者"` | 无当前患者时 |
| Screen1 | `"等待人数：%d"` / `"等待人数：0"` | 等待人数显示 |
| Screen1 | `"无候诊患者"` | 下一位患者为空时 |
| Screen2 | `"请输入ID/姓名"` | 搜索框引导文字 |
| Screen2 | `"搜索"` | 搜索按钮 |
| Screen2 | `"排序"` | 排序按钮 |
| Screen2 | `"当前科室：B12-131"` | 顶栏科室信息 |
| Screen2 | `"返回"` | Screen2返回按钮 |
| Screen2 | `"置顶"` | 患者置顶按钮 |
| Screen2 | `"请输入姓名或编号进行搜索"` | 搜索提示 |
| Screen2 | `"精确匹配\|姓名：%s\|编号：%ld\|年龄：%d"` | 精确匹配结果 |
| Screen2 | `"姓名：%s\|编号：%ld\|年龄：%d"` | 模糊匹配结果 |
| Screen2 | `"未找到匹配的患者"` | 无搜索结果 |
| Screen2 | `"就诊记录"` | 就诊记录按钮 |
| Screen2 | `"暂无就诊记录"` | 就诊记录为空时 |
| Screen2 | `"%ld：%s（耗时 %d 分钟）"` | 就诊记录条目格式 |
| Screen3 | `"AI助手"` | 顶栏标题 |
| Screen3 | `"请输入内容..."` | 输入框引导文字 |
| Screen3 | `"发送"` | 发送按钮 |
| Screen3 | `"返回"` | Screen3返回按钮 |
| Screen4 | `"数据统计"` | 顶栏标题 |
| Screen4 | `"已就诊"` | 统计卡片1标题 |
| Screen4 | `"%d 人"` / `"0 人"` | 已就诊人数 |
| Screen4 | `"平均等待"` | 统计卡片2标题 |
| Screen4 | `"%d 分钟"` / `"0 分钟"` | 平均等待时间 |
| Screen4 | `"当前排队"` | 统计卡片3标题 |
| Screen4 | `"近8小时流量"` | 流量柱状图标题 |
| Screen4 | `"返回主界面"` | 返回按钮 |

### 字体文件

| 文件 | 字号 | 字模来源 | 格式 |
|------|------|----------|------|
| `my_font_cn.c` | 16px | 仿宋体 (simfang.ttf) | LVGL 4bpp, 无压缩 |
| `my_font_cn_48.c` | 48px | 仿宋体 (simfang.ttf) | LVGL 4bpp, 无压缩 |

### 字体生成步骤

#### 1. 准备工具

```bash
npm install -g lv_font_conv
```

或使用 LVGL 官方在线工具: https://lvgl.io/tools/fontconverter

#### 2. 确定需要的汉字集合

将所有运行时字符串中出现的汉字去重，加上英文字母、数字和常用符号，得到 symbols 列表。

当前项目两个字体共用同一套 symbols（含空格）：
```
当前科室患者详情下一个请输入姓名搜索正序倒排无匹配暂王李张刘陈杨黄赵吴周徐孙马胡朱郭何罗高林伟芳娜敏静涛文华强磊洋勇艳杰军秀英明丽平门诊急救等待叫号暂无候诊返回人数精确匹编号年龄未找到的或进行统计数据已就均队时段流量主界面钟分置顶内容发送消息同步记录耗近小（） AI助手请求出错思考中对话问答你好我是医疗帮助什么问题咨询建议预约医生科室时间连接失败同步成功数据更新网络超时服务器错误请重试开始结束日期 ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.,:;-()/%|：
```

> **注意**：`A` 前面有一个空格字符，必须包含。末尾新增 `记录耗近小（）` 用于"就诊记录"、"耗时"、"近8小时"、"（耗时 X 分钟）"等字符串。

#### 3. 生成字体命令

**16px 字体**:
```bash
lv_font_conv --bpp 4 --size 16 --no-compress --stride 1 --align 1 \
  --font simfang.ttf \
  --symbols "当前科室患者详情下一个请输入姓名搜索正序倒排无匹配暂王李张刘陈杨黄赵吴周徐孙马胡朱郭何罗高林伟芳娜敏静涛文华强磊洋勇艳杰军秀英明丽平门诊急救等待叫号暂无候诊返回人数精确匹编号年龄未找到的或进行统计数据已就均队时段流量主界面钟分置顶内容发送消息同步记录耗近小（） AI助手请求出错思考中对话问答你好我是医疗帮助什么问题咨询建议预约医生科室时间连接失败同步成功数据更新网络超时服务器错误请重试开始结束日期 ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.,:;-()/%|：" \
  --format lvgl -o my_font_cn.c
```

**48px 字体**:
```bash
lv_font_conv --bpp 4 --size 48 --no-compress --stride 1 --align 1 \
  --font simfang.ttf \
  --symbols "当前科室患者详情下一个请输入姓名搜索正序倒排无匹配暂王李张刘陈杨黄赵吴周徐孙马胡朱郭何罗高林伟芳娜敏静涛文华强磊洋勇艳杰军秀英明丽平门诊急救等待叫号暂无候诊返回人数精确匹编号年龄未找到的或进行统计数据已就均队时段流量主界面钟分置顶内容发送消息同步记录耗近小（） AI助手请求出错思考中对话问答你好我是医疗帮助什么问题咨询建议预约医生科室时间连接失败同步成功数据更新网络超时服务器错误请重试开始结束日期 ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.,:;-()/%|：" \
  --format lvgl -o my_font_cn_48.c
```

#### 4. 新增/修改汉字的步骤

1. 在 symbols 字符串末尾追加新的汉字
2. 重新执行上述 `lv_font_conv` 命令生成字体文件
3. 替换项目中的 `my_font_cn.c` 或 `my_font_cn_48.c`
4. 重新编译项目

> **关键约束**：UI 代码中 `lv_label_set_text()` 使用的每一个汉字都必须出现在字体文件的 symbols 中，否则该字会显示为空白方块。

### 项目字体使用总结

| 字体 | 声明位置 | 使用位置 |
|------|----------|----------|
| `my_font_cn` (16px) | `my_font_cn.c` 自动编译链接 | Screen1/Screen2/Screen3/Screen4 所有中文标签、按钮文字 |
| `my_font_cn_48` (48px) | `main.c` `extern const lv_font_t my_font_cn_48;` | Screen1 当前患者大标签 (`ui_CurrentPatientLabel`) |

## 核心业务函数

### Screen1（主叫号界面）

| 函数名 | 功能描述 | 参数说明 | 返回值 | 触发场景 |
|--------|----------|----------|--------|----------|
| `refresh_current_patient(e)` | 刷新当前患者显示标签（大字体） | `e`: LVGL事件对象 | 无 | 启动时、点击标签时、从Screen2返回时 |
| `refresh_waiting_count(e)` | 刷新等待人数显示 | `e`: LVGL事件对象 | 无 | 启动时、点击标签时、叫号后 |
| `refresh_next_patient(e)` | 刷新下一位患者信息 | `e`: LVGL事件对象 | 无 | 启动时、点击标签时、叫号后 |
| `call_next_patient(e)` | 叫号操作：弹出队首患者，更新UI，触发闪烁 | `e`: LVGL事件对象 | 无 | 点击"下一个"按钮时 |
| `refresh_waiting_queue(e)` | 重建候诊队列列表 | `e`: LVGL事件对象 | 无 | 启动时、叫号后、点击列表面板时 |

### Screen2（患者管理界面）

| 函数名 | 功能描述 | 参数说明 | 返回值 | 触发场景 |
|--------|----------|----------|--------|----------|
| `refresh_patient_list(e)` | 重建患者管理列表 | `e`: LVGL事件对象 | 无 | 进入Screen2时、排序后、置顶后 |
| `search_patient(e)` | 根据输入搜索患者 | `e`: LVGL事件对象 | 无 | 点击"搜索"按钮时 |
| `sort_patient_list(e)` | 根据下拉框选项排序患者列表 | `e`: LVGL事件对象 | 无 | 点击"排序"按钮时 |
| `refresh_event_list(e)` | 从stats.json的recent_events刷新就诊记录列表 | `e`: LVGL事件对象 | 无 | 点击"就诊记录"按钮时 |

> **注意**：Screen2患者列表不在自动同步时刷新，避免用户浏览时被后台打断。仅在进入界面、置顶、排序时刷新。


### Screen3（消息发送界面）

| 函数名 | 功能描述 | 参数说明 | 返回值 | 触发场景 |
|--------|----------|----------|--------|----------|
| `ui_event_Screen3Input(e)` | 输入框事件：点击/聚焦时弹出键盘，失焦时隐藏键盘 | `e`: LVGL事件对象 | 无 | 输入框获得/失去焦点时 |
| `ui_event_Screen3BackBtn(e)` | 返回主界面（Screen1），刷新队列数据 | `e`: LVGL事件对象 | 无 | 点击"返回"按钮 |
| `ui_event_Screen3NavBtn(e)` | 从Screen1跳转到消息发送界面（Screen3） | `e`: LVGL事件对象 | 无 | 点击"消息发送"按钮 |

### Screen4（统计看板界面）

| 函数名 | 功能描述 | 参数说明 | 返回值 | 触发场景 |
|--------|----------|----------|--------|----------|
| `refresh_screen4_stats()` | 刷新统计看板数据（已就诊、平均等待、当前排队、近8小时流量柱状图） | 无 | 无 | 进入Screen4时、每次同步后 |
| `screen4_draw_flow_bars()` | 绘制近8小时流量柱状图（含柱体上方数值+底部时间标签） | 无 | 无 | refresh_screen4_stats() 调用 |
| `ui_event_Screen4BackBtn(e)` | 返回主界面（Screen1），刷新队列数据 | `e`: LVGL事件对象 | 无 | 点击"返回主界面"按钮 |
| `ui_event_ReservedBtn(e)` | 从Screen1跳转到统计看板（Screen4）并刷新统计 | `e`: LVGL事件对象 | 无 | 点击"数据统计"按钮 |

### 统计模块（stats）

| 函数名 | 功能描述 | 参数说明 | 返回值 | 触发场景 |
|--------|----------|----------|--------|----------|
| `stats_init()` | 初始化统计变量（清零） | 无 | 无 | main() 启动时 |
| `stats_update_from_json(json)` | 解析服务器stats.json，更新today_called、hourly_calls、avg_wait_min、recent_events | `json`: JSON字符串 | 0=成功, -1=失败 | 每次同步时 sync_stats_from_server() 调用 |

**数据来源**：统计数据全部由服务器（网页 patient.html）计算，板端仅通过 HTTP GET `/stats.json` 接收并更新显示。事件队列逻辑在网页端维护，板端不再本地计算。

### 同步模块（sync_manager）

| 函数名 | 功能描述 | 参数说明 | 返回值 | 触发场景 |
|--------|----------|----------|--------|----------|
| `sync_from_server()` | HTTP GET 拉取 patient_queue.json → cJSON 解析 → 替换队列 → 刷新Screen1 UI → 拉取 stats.json → 刷新Screen4 | 无 | 0=成功, -1=网络失败, -2=解析失败 | 主循环每 5 秒自动调用 + 点击"同步"按钮 |
| `sync_stats_from_server()` | HTTP GET 拉取 stats.json → cJSON 解析 → 更新统计变量 | 无 | 0=成功, -1=网络失败, -2=解析失败 | sync_from_server() 内部调用 |

**同步流程**：
1. 裸 socket HTTP GET `http://192.168.171.73/patient_queue.json`
2. cJSON 解析 JSON 数组，提取每个患者的 `Patient_ID`、`name`、`ID_number`、`age`
3. 销毁旧队列 → 创建新队列 → 逐条填入
4. 依次调用 4 个 refresh 函数刷新 Screen1 UI（不刷新Screen2，避免打断用户操作）
5. HTTP GET `http://192.168.171.73/stats.json` 拉取统计数据
6. `stats_update_from_json()` 更新 today_called、hourly_calls（8小时）、avg_wait_min、recent_events
7. `refresh_screen4_stats()` 刷新Screen4卡片和柱状图
8. 主循环每 5 秒自动同步一次

**HTTP 网络问题排查记录**：详见 `tmp/HTTP网络问题排查记录.md`

### 整体项目检查结果

| 检查项 | 结果 | 备注 |
|--------|------|------|
| 未初始化变量 | ✅ 通过 | 所有 P_List/P_Node 均在使用前初始化 |
| 空指针解引用 | ✅ 已修复 | search_patient() 已添加 patient_queue 空指针守卫 |
| #include 完整性 | ✅ 通过 | 所有声明可从 include 链路追踪 |
| static 符号冲突 | ✅ 通过 | 无同名 static 函数/变量冲突 |
| printf 格式匹配 | ✅ 通过 | 所有 %d/%ld/%s 与参数类型一致 |
| socket 资源泄漏 | ✅ 通过 | http_get() 所有错误路径均 close(sock) |
| Screen4 背景重叠 | ✅ 已修复 | 添加 lv_obj_set_style_bg_opa(ui_Screen4, LV_OPA_COVER, 0) |

### tiny-httpd 服务器相关命令

| 操作 | 命令 |
|------|------|
| 启动服务器 | `./auto_build.sh run`（在服务器项目目录执行） |
| ARM 编译+部署 | `./auto_build.sh deploy`（在服务器项目目录执行） |
| 测试 JSON 接口 | `curl http://192.168.171.73:8080/patient_queue.json` |
| Web 管理页面 | 浏览器访问 `http://192.168.171.73:8080/patient.html` |

> **注意**：服务器端口为 8080（tiny-httpd默认），与Apache2的80端口不同。
> 服务器项目地址为 `~/codepath/轻量化http服务器CGI引擎/`。

---
## 内存配置（重要）

### LVGL内部内存池

`lv_conf.h:52` —— LVGL所有控件、样式、flex布局、事件回调都在这块池中分配。

| 配置项 | 旧值 | 新值 | 说明 |
|--------|------|------|------|
| `LV_MEM_SIZE` | **48KB** | **256KB** | 48KB导致碎片化后分配失败卡死 |

### 显示缓冲区

`main.c:47` —— LVGL渲染缓冲，单缓冲全屏384K像素即可。

| 配置项 | 旧值 | 新值 | 说明 |
|--------|------|------|------|
| `DISP_BUF_SIZE` | `800*480*4` (1536000像素=6MB) | `(800*480)` (384000像素=1.5MB) | 旧值浪费4.5MB |

### 内存总览

| 项目 | 大小 | 位置 |
|------|------|------|
| 显示缓冲 | 1.5 MB | BSS |
| 背景图片×3 (ARGB8888) | ~4.5 MB | rodata |
| 中文字体位图 (16px+48px) | ~710 KB | rodata |
| LVGL对象池 | 256 KB | BSS |
| 代码段+LVGL库 | ~500 KB | text |
| 栈 (默认8MB虚拟) | 8 MB | 虚拟地址 |
| 杂项 (sync_buf/cJSON等) | ~200 KB | BSS/data |

---
## 已修复的关键Bug

### 1. LV_MEM_SIZE过小导致UI卡死 (2026-06-08)

**现象**：点击"患者详情"切换屏幕时UI冻结，多次来回切换后必然复现。
不连接服务器（队列为空）不卡，服务器有数据（10个患者）后卡死。

**根因**：`LV_MEM_SIZE = 48KB`，4个屏幕预创建105个控件占用~25KB，每次刷新患者列表释放旧控件再创建40个新控件（~10KB）。经过几次分配-释放周期后池内碎片化，新的大块分配失败，`lv_mem_alloc`返回NULL，LVGL内部未处理 → 卡死。

**修复**：`LV_MEM_SIZE` 从48KB扩到256KB。

### 2. 显示缓冲浪费4.5MB (2026-06-08)

**现象**：`DISP_BUF_SIZE 800*480*4`，使BSS段分配了1536000像素（6MB），实际屏幕仅384000像素（1.5MB）。

**修复**：`DISP_BUF_SIZE` 改为 `(800*480)`。

### 3. HTTP同步socket阻塞主循环 (2026-06-08)

**现象**：tiny-httpd响应慢或网络波动时，`send()`/`recv()`阻塞导致整个UI不可用。

**修复**：同步状态机全程使用非阻塞socket + `poll()`超时检测 + EAGAIN重试。

### 4. 屏幕切换动画期间创建widget冲突 (2026-06-08)

**现象**：`lv_scr_load_anim`动画渲染与`refresh_patient_list`同步创建40+控件冲突。

**修复**：所有导航按钮只做屏幕切换，widget刷新通过`LV_EVENT_SCREEN_LOADED`在动画完成后触发。

### 5. kill_list_node递归栈溢出风险 (2026-06-08)

**修复**：改为迭代释放循环。

### 6. _ui_screen_delete空指针判断反转 (2026-06-08)

**修复**：`if(*target == NULL)` → `if(target != NULL && *target != NULL)`。

---
## Apache2 服务器（已弃用，改用tiny-httpd）

> 以下为旧版Apache2服务器配置，保留作参考。

| 操作 | 命令 |
|------|------|
| 启动 Apache | `sudo service apache2 start` |
| 停止 Apache | `sudo service apache2 stop` |
| 重启 Apache | `sudo service apache2 restart` |
| 查看状态 | `sudo service apache2 status` |
| 测试 JSON 接口 | `curl http://127.0.0.1/patient_queue.json` |
| Web 管理页面 | 浏览器访问 `http://<WSL-IP>/patient.html` |

> **注意**：WSL1 不需要 portproxy，Apache 监听 `0.0.0.0:80` 会自动暴露到 Windows 物理网卡。
> 如果外部设备无法访问 80 端口，执行 `netsh interface portproxy reset` 清除冲突的转发规则。



---
## UI控件变量

### Screen1 控件

| 变量名 | 控件类型 | 功能描述 | 位置 |
|--------|----------|----------|------|
| `ui_MainScreen` | Screen | 主叫号界面根容器 | 全屏 |
| `ui_DeptPanel` | Panel | 顶栏科室信息容器（蓝色渐变） | 顶部 |
| `ui_DeptLabel` | Label | 科室名称标签 | 顶栏内 |
| `ui_WaitingQueuePanel` | Panel | 候诊队列滚动面板 | 左侧 |
| `ui_CurrentPatientCard` | Panel | 当前患者信息卡片 | 中央 |
| `ui_CurrentPatientLabel` | Label | **当前患者ID/姓名（大字体，闪烁目标）** | 卡片中央 |
| `ui_WaitingCountLabel` | Label | 等待人数标签 | 卡片左下角 |
| `ui_NextPatientLabel` | Label | 下一位患者信息标签 | 卡片右下角 |
| `ui_CallNextBtn` | Button | "同步"按钮（蓝色渐变） | 下方左一 |
| `ui_PatientDetailBtn` | Button | "患者详情"按钮（灰色渐变） | 下方右一 |
| `ui_Screen3NavBtn` | Button | "消息发送"按钮（绿色渐变） | 下方左二 |
| `ui_ReservedBtn` | Button | "数据统计"按钮（橙色渐变） | 下方右二 |

### Screen2 控件

| 变量名 | 控件类型 | 功能描述 | 位置 |
|--------|----------|----------|------|
| `ui_PatientListScreen` | Screen | 患者管理界面根容器 | 全屏 |
| `ui_PatientListPanel` | Panel | 患者列表容器 | 中部 |
| `ui_SearchInput` | TextArea | 搜索输入框 | 顶部左侧 |
| `ui_SearchBtn` | Button | 搜索按钮（紫色） | 搜索框右侧 |
| `ui_SortDropdown` | Dropdown | 排序方式下拉框 | 顶部中间 |
| `ui_SortBtn` | Button | 排序按钮（浅紫色） | 下拉框左侧 |
| `ui_DeptPanel2` | Panel | 顶栏科室信息容器（紫色渐变） | 顶部 |
| `ui_BackBtn` | Button | 返回主界面按钮（蓝色） | 底部右侧 |
| `ui_EventListBtn` | Button | 就诊记录按钮（灰色） | 底部左侧 |
| `ui_ScreenKeyboard` | Keyboard | 软键盘 | 底部（隐藏） |

### Screen3 控件

| 变量名 | 控件类型 | 功能描述 | 位置 |
|--------|----------|----------|------|
| `ui_Screen3` | Screen | 消息/输入界面根容器 | 全屏 |
| `ui_Screen3Panel` | Panel | 全屏容器面板 | 填满屏幕 |
| `ui_Screen3Keyboard` | Keyboard | 软键盘（默认隐藏，显示时 lv_obj_move_foreground 提升Z层级） | 下部 |
| `ui_Screen3TopBar` | Panel | 顶栏容器（绿色渐变） | 顶部 |
| `ui_Screen3TopLabel` | Label | 顶栏标题标签 | 顶栏内 |
| `ui_Screen3Input` | TextArea | 消息输入框 | 顶部下方 |
| `ui_Screen3MiddlePanel` | Panel | 中间内容显示面板 | 中部 |
| `ui_Screen3SendBtn` | Button | 发送按钮（绿色） | 底部右侧 |
| `ui_Screen3BackBtn` | Button | 返回按钮（蓝色） | 底部左侧 |

### Screen4 控件

| 变量名 | 控件类型 | 功能描述 | 位置 |
|--------|----------|----------|------|
| `ui_Screen4` | Screen | 统计看板界面根容器 | 全屏 |
| `ui_Screen4TopBar` | Panel | 顶栏容器（橙色渐变） | 顶部 |
| `ui_Screen4TopLabel` | Label | 顶栏标题"数据统计" | 顶栏内 |
| `ui_Screen4StatCard1` | Panel | 已就诊统计卡片 | 上方左 |
| `ui_Screen4StatNum1` | Label | 已就诊人数数值 | 卡片1内 |
| `ui_Screen4StatCard2` | Panel | 平均等待统计卡片 | 上方中 |
| `ui_Screen4StatNum2` | Label | 平均等待时间数值 | 卡片2内 |
| `ui_Screen4StatCard3` | Panel | 当前排队统计卡片 | 上方右 |
| `ui_Screen4StatNum3` | Label | 当前排队人数数值 | 卡片3内 |
| `ui_Screen4FlowPanel` | Panel | 近8小时流量柱状图面板 | 中部 |
| `ui_Screen4BackBtn` | Button | 返回主界面按钮（蓝色） | 底部 |

---
## 界面颜色体系

| 界面 | 主题色 | 顶栏渐变 | 按钮颜色含义 |
|------|--------|----------|-------------|
| Screen1 | 蓝色 | `#0b5ed7 → #3b82f6` | 蓝色=本界面操作，绿色=跳转Screen3，橙色=跳转Screen4 |
| Screen2 | 紫色 | `#7c3aed → #6d28d9` | 紫色=本界面操作，蓝色=返回Screen1，灰色=就诊记录 |
| Screen3 | 绿色 | `#059669 → #10b981` | 绿色=本界面操作，蓝色=返回Screen1 |
| Screen4 | 橙色 | `#c2410c → #f97316` | 蓝色=返回Screen1 |

> 按钮颜色与目标界面颜色一致，方便用户识别跳转目标。

---

## 闪烁功能

| 属性 | 值 |
|------|-----|
| 闪烁目标 | `ui_CurrentPatientLabel`（当前患者大标签） |
| 闪烁次数 | 6次透明度切换（3个完整周期） |
| 切换间隔 | 300ms |
| 透明度过渡 | LV_OPA_COVER ↔ LV_OPA_30 |
| 触发时机 | `refresh_current_patient()` 和 `call_next_patient()` 执行时 |

---

## 数据结构

### 患者节点 (`P_Node`)

```c
struct patient {
    long Patient_ID;        // 患者编号
    char name[20];          // 患者姓名
    char ID_number[20];     // 身份证号
    int age;                // 年龄
    struct patient *next;   // 链表下一个节点
};
```

### 患者队列 (`P_List`)

```c
struct patient_list {
    P_Node head;            // 表头（哨兵节点）
    P_Node tail;            // 表尾
    int size;               // 队列大小
};
```

### 统计数据 (`stats`)

```c
#define HOURLY_SLOTS 8
int today_called;              // 今日已叫号总数（服务器计算）
int hourly_calls[HOURLY_SLOTS]; // 近8小时每小时叫号数（服务器计算）
int avg_wait_min;              // 平均等待分钟数（服务器计算）
int completed_count;           // 已完成就诊数

#define MAX_RECENT_EVENTS 50
typedef struct {
    long Patient_ID;
    char name[20];
    char start_time[6];    // 开始时间 "HH:MM"
    char end_time[6];      // 结束时间 "HH:MM"
    char date[11];         // 日期 "YYYY-MM-DD"
    int duration_min;      // 就诊时长（分钟）
} recent_event_t;

recent_event_t recent_events[MAX_RECENT_EVENTS];
int recent_event_count;        // 最近已完成就诊事件列表
```

### stats.json 格式（服务器提供）

```json
{
  "today_called": 23,
  "avg_wait_min": 15,
  "hourly_calls": [0, 2, 4, 5, 3, 1, 3, 2],
  "recent_events": [
    {"id": 1008, "name": "周八", "start_time": "09:30", "end_time": "09:42", "date": "2026-06-01", "duration_min": 12},
    {"id": 1007, "name": "吴七", "start_time": "09:15", "end_time": "09:23", "date": "2026-06-01", "duration_min": 8}
  ]
}
```

---

## 修改记录

| 日期 | 修改内容 | 修改人 |
|------|----------|--------|
| 2026-05-18 | 初始化项目结构，添加闪烁功能 | 开发 |
| 2026-05-19 | 统一命名规范，清理死代码 | 开发 |
| 2026-05-29 | 集成Screen3参考界面（文本输入+软键盘+发送按钮） | 开发 |
| 2026-05-29 | 添加sync_manager同步模块（HTTP+cJSON自动同步患者队列） | 开发 |
| 2026-05-30 | 添加Screen4统计看板（橙色主题，已就诊/平均等待/当前排队/时段流量柱状图） | 开发 |
| 2026-05-30 | 添加stats统计模块（today_called, hourly_calls, stats_on_call等） | 开发 |
| 2026-05-30 | 界面颜色体系：按钮颜色=目标界面颜色，Screen2紫色/Screen3绿色/Screen4橙色 | 开发 |
| 2026-05-30 | 修复Screen4与Screen1重叠问题（添加bg_opa=LV_OPA_COVER） | 开发 |
| 2026-05-30 | 修复HTTP网络问题（IP网段+portproxy冲突） | 开发 |
| 2026-05-30 | 重构事件队列：移除本地event_queue，统计由服务器（网页）计算，板端仅接收JSON更新 | 开发 |
| 2026-05-30 | Screen2新增"就诊记录"按钮（x=-110，灰色），BackBtn移至x=+110对称布局 | 开发 |
| 2026-05-30 | 所有业务函数统一收拢至ui_events.c，Screen4函数移入，简化main.c | 开发 |
| 2026-05-30 | Screen4柱状图改为近8小时（当前小时-7到当前小时），动态横坐标标签 | 开发 |
| 2026-05-30 | sync_manager新增sync_stats_from_server，同步拉取/stats.json | 开发 |
| 2026-05-30 | stats模块重构：HOURLY_SLOTS=8，新增avg_wait_min、recent_events、stats_update_from_json | 开发 |
| 2026-05-30 | api.php新增POST ?target=stats写入stats.json，patient.html添加事件队列跟踪+自动推送统计 | 开发 |
| 2026-05-30 | Screen2修复：添加LV_EVENT_DELETE回调清空ui_PatientListPanel指针，解决切换界面后列表不刷新 | 开发 |
| 2026-05-30 | Screen4柱状图上方添加数值标签显示人数 | 开发 |
| 2026-05-30 | 自动同步不再刷新Screen2列表，避免用户浏览时被后台打断 | 开发 |
| 2026-06-08 | 修复LV_MEM_SIZE=48KB导致UI卡死（改为256KB） | 开发 |
| 2026-06-08 | 修复显示缓冲DISP_BUF_SIZE浪费4.5MB（改为单屏384Kpx） | 开发 |
| 2026-06-08 | 重写sync_manager为全程非阻塞socket+poll超时，消除阻塞风险 | 开发 |
| 2026-06-08 | 修复屏幕切换动画与widget创建冲突：改用LV_EVENT_SCREEN_LOADED延迟刷新 | 开发 |
| 2026-06-08 | 修复kill_list_node递归改为迭代，消除栈溢出风险 | 开发 |
| 2026-06-08 | 修复_ui_screen_delete空指针判断反转 | 开发 |
| 2026-06-08 | 服务器从Apache2迁移至tiny-httpd（端口8080） | 开发 |
| 2026-06-08 | 补充全部核心源码的中文注释 | 开发 |

---

## 需求描述格式

当你需要修改或添加功能时，可以按以下格式描述：

```
【需求】[功能简述]

【详细描述】
- 功能说明：...
- 涉及文件：...（如 ui_events.c, main.c）
- 涉及函数：...（如 call_next_patient）
- 预期行为：...

【参考示例】
（如果有参考代码或截图请在此说明）
```

---

## 示例需求

```
【需求】添加叫号成功提示音

【详细描述】
- 功能说明：叫号成功后播放一段提示音
- 涉及文件：ui_events.c
- 涉及函数：call_next_patient()
- 预期行为：点击"下一个"按钮后，除了文字闪烁外，还播放提示音

【参考示例】
提示音时长：1秒
```
