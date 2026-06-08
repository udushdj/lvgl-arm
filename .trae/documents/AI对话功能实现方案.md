# Screen3 AI对话功能实现方案

## 概述

为Screen3添加AI对话功能，集成DeepSeek API实现智能医疗问答。用户在输入框输入问题后，系统通过HTTPS调用DeepSeek API获取回复，以气泡式聊天界面展示。

## 架构设计

```
用户输入 → on_screen3_send() → pthread后台线程 → ai_chat_send()
                                    ↓                    ↓
                              ai_poll_cb()轮询    OpenSSL HTTPS → DeepSeek API
                                    ↓                    ↓
                              更新LVGL聊天面板  ←  解析JSON响应
```

### 线程模型

- **主线程**：LVGL UI渲染 + 200ms定时器轮询AI完成状态
- **后台线程**：pthread执行`ai_chat_send()`（阻塞式网络请求，5-15秒）
- **通信机制**：`volatile int ai_busy`标志位 + 共享缓冲区`ai_resp_buf`

### 关键设计决策

1. **非阻塞UI**：AI请求在后台线程执行，不冻结LVGL界面
2. **气泡式布局**：用户消息右对齐绿色，AI消息左对齐灰色
3. **思考指示器**：请求期间显示"等待中..."提示
4. **生命周期管理**：Screen3销毁时自动清理定时器和指针

## 文件变更清单

### 新增文件

| 文件 | 说明 |
|------|------|
| `UIManager/ai_chat.h` | AI对话模块头文件（接口定义） |
| `UIManager/ai_chat.c` | AI对话核心实现（OpenSSL HTTPS + DeepSeek API + cJSON） |

### 修改文件

| 文件 | 变更内容 |
|------|----------|
| `UIManager/screens/ui_Screen3.c` | MiddlePanel改为可滚动flex列布局；顶栏标题改为"AI助手"；绑定SendBtn事件和Screen3删除回调 |
| `UIManager/ui_events.c` | 新增`add_chat_bubble()`/`on_screen3_send()`/`on_screen3_delete()`及AI线程逻辑 |
| `UIManager/ui_events.h` | 新增`on_screen3_send()`/`on_screen3_delete()`声明 |
| `UIManager/ui.h` | 新增`ui_event_Screen3SendBtn()`声明 |
| `UIManager/ui.c` | 新增`ui_event_Screen3SendBtn()`回调实现 |
| `CMakeLists.txt` | 添加`ssl crypto pthread`链接 |
| `main.c` | 添加`ai_chat_init()`调用 |

## 关键实现细节

### 1. ai_chat.c — OpenSSL HTTPS客户端

```c
#define AI_HOST     "api.deepseek.com"
#define AI_PORT     "443"
#define AI_PATH     "/chat/completions"
#define AI_MODEL    "deepseek-chat"
#define AI_KEY      "xxxxxxxxxxxxxxxxx"  // 需替换为实际API密钥
#define AI_TIMEOUT  15
```

- 非阻塞socket连接（`fcntl` + `select`超时）
- TLS客户端（`TLS_client_method()`）
- cJSON构建请求体，解析`choices[0].message.content`
- 系统提示词：`"你是一个医疗叫号系统的AI助手，帮助医护人员和患者回答医疗相关问题。请简洁回答。"`

### 2. 聊天气泡布局

```c
// 每条消息 = row容器(全宽flex-row) + bubble容器(自适应宽度)
// 用户消息：flex-align=END，绿色背景(#10b981)，白色文字
// AI消息：flex-align=START，灰色背景(#f1f5f9)，深色文字
// 气泡最大宽度540px，自动计算窄消息宽度
```

气泡宽度计算：`lv_txt_get_size()`测量文本像素宽度 → `min(文本宽度+24, 540)`

### 3. 发送流程

```
1. 用户点击"发送" → on_screen3_send()
2. 获取输入文本，添加用户气泡
3. 清空输入框，隐藏键盘
4. 添加"等待中..."标签
5. strdup复制消息 → pthread_create启动后台线程
6. 创建200ms LVGL定时器轮询ai_busy标志
7. 后台线程完成 → ai_busy=0
8. 定时器回调：删除"等待中..."，添加AI回复气泡
9. 滚动到底部
```

### 4. Screen3生命周期

```c
// 创建时：ui_Screen3_screen_init()中绑定LV_EVENT_DELETE回调
// 销毁时：on_screen3_delete()清空指针、删除定时器
```

## OpenSSL交叉编译（ARM）

### 步骤1：下载并编译OpenSSL

```bash
wget https://www.openssl.org/source/openssl-1.1.1w.tar.gz
tar xzf openssl-1.1.1w.tar.gz
cd openssl-1.1.1w

./Configure linux-armv4 \
  --prefix=/home/wz/usr/local/openssl-arm \
  --cross-compile-prefix=arm-linux-

make -j4
make install
```

### 步骤2：更新CMakeLists.txt

```cmake
# 在target_compile_definitions之后添加：
target_include_directories(${PROJECT_NAME} PRIVATE
    /home/wz/usr/local/openssl-arm/include
)
target_link_directories(${PROJECT_NAME} PRIVATE
    /home/wz/usr/local/openssl-arm/lib
)
target_link_libraries(${PROJECT_NAME} ssl crypto pthread)
```

### 步骤3：ARM板部署

将交叉编译的OpenSSL库复制到ARM板：

```bash
scp /home/wz/usr/local/openssl-arm/lib/libssl.so.1.1 root@<ARM_IP>:/usr/lib/
scp /home/wz/usr/local/openssl-arm/lib/libcrypto.so.1.1 root@<ARM_IP>:/usr/lib/
```

## 字体更新

新增中文字符需添加到字体生成命令的symbols中：

```
助手思考中出错对话问
```

完整字体重新生成命令：

```bash
npx lv_font_conv --font simfang.ttf --size 20 --bpp 4 --symbols "暂无候诊患者等待人数当前叫号下一个科室内科外科儿科皮肤骨科详情返回搜索排序编号降序置顶就诊记录耗时分钟近小（）请输入姓名或进行内容发送消息AI助手思考中出错对话问" --format lvgl -o my_font_cn.c
```

## 配置步骤

1. **设置API密钥**：编辑`UIManager/ai_chat.c`，将`AI_KEY`宏替换为实际DeepSeek API密钥
2. **交叉编译OpenSSL**：按上述步骤编译ARM版本
3. **更新CMakeLists.txt**：添加OpenSSL头文件路径和库路径
4. **重新生成字体**：添加新中文字符
5. **编译项目**：`cd build && cmake .. && make`
6. **部署**：将可执行文件和OpenSSL库复制到ARM板

## 注意事项

- `ai_chat_send()`为阻塞调用，必须在后台线程执行
- ARM板需要网络访问`api.deepseek.com:443`
- DeepSeek API有调用频率限制和计费，注意控制请求频率
- 如果网络不可用，会显示"请求出错"提示
- Screen3销毁时（用户返回主界面），进行中的AI请求结果会被丢弃
