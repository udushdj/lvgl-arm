/**
 * @file ota_manager.h
 * @brief OTA自动更新 —— 非阻塞HTTP下载 + MD5校验 + 自动重启
 *
 * 与 sync_manager 共享服务器地址，使用相同的非阻塞socket + poll模式。
 * 检查间隔: 60秒 (OTA_CHECK_INTERVAL_SEC)
 *
 * 状态流转:
 *   OTA_IDLE → OTA_CHECK_* → 版本相同 → OTA_IDLE
 *                          → 版本不同 → OTA_DL_* → OTA_VERIFY → OTA_INSTALL
 *
 * 文件:
 *   服务器: /ota/version (格式: VERSION\nMD5\nSIZE)
 *           /ota/lvgl_fb  (ARM二进制)
 *   本地:   /wz/version   (当前版本)
 *           /wz/lvgl_fb   (当前二进制)
 *           /wz/lvgl_fb.new (下载临时文件)
 */

#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 触发一次版本检查 (仅在 OTA_IDLE 时有效)
 * @return 0=已触发, -1=正在忙或初始化失败
 */
int ota_check_trigger(void);

/**
 * @brief 推进OTA状态机 (主循环每帧调用，绝不阻塞)
 */
void ota_poll(void);

#ifdef __cplusplus
}
#endif

#endif
