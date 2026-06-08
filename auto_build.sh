#!/bin/bash
#
# 医疗叫号系统 - WSL交叉编译运行脚本
# 用法: 在 WSL 原生路径下运行！
#       cd /home/wz/codepath/LVGL/lvgl-arm
#       bash run.sh
#
# 注意: 不要在 /mnt/c/... 路径下运行，CMake 不支持跨路径缓存！

set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
EXEC_NAME="lvgl_fb"

echo "========================================="
echo "  医疗叫号系统 - 编译运行脚本"
echo "  项目路径: $PROJECT_DIR"
echo "========================================="

if [[ "$PROJECT_DIR" == /mnt/* ]]; then
    echo "错误: 请在 WSL 原生 Linux 路径下运行本脚本！"
    echo "      请将项目克隆到 /home/wz/codepath/LVGL/lvgl-arm"
    echo "      不要从 /mnt/c/ 路径运行"
    exit 1
fi

cd "$PROJECT_DIR"

echo "[1/4] 清理旧编译缓存..."
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

echo "[2/4] 执行 CMake 配置与编译..."
cd "$BUILD_DIR"
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

if [ ! -f "$PROJECT_DIR/$EXEC_NAME" ]; then
    echo "错误: 编译产物 $EXEC_NAME 未找到！"
    echo "查找: $PROJECT_DIR/$EXEC_NAME"
    ls -la "$PROJECT_DIR/$EXEC_NAME" 2>/dev/null || echo "(文件不存在)"
    exit 1
fi

echo "[3/4] 编译成功，可执行文件: $PROJECT_DIR/$EXEC_NAME"

echo "[4/4] 部署到开发板 root@192.168.171.74:/wz/ ..."
scp "$PROJECT_DIR/$EXEC_NAME" root@192.168.171.74:/wz/

if [ $? -eq 0 ]; then
    # 同时推送到OTA服务器供开发板自动更新
    OTA_DIR="/home/wz/codepath/轻量化http服务器CGI引擎/www/ota"
    if [ -d "$OTA_DIR" ]; then
        echo "[OTA] 同步二进制到OTA服务器..."
        cp "$PROJECT_DIR/$EXEC_NAME" "$OTA_DIR/"
        VER=$(date +%Y%m%d%H%M)
        MD5=$(md5sum "$PROJECT_DIR/$EXEC_NAME" | awk '{print $1}')
        SIZE=$(stat -c%s "$PROJECT_DIR/$EXEC_NAME")
        printf "%s\n%s\n%s\n" "$VER" "$MD5" "$SIZE" > "$OTA_DIR/version"
        echo "[OTA] 版本: $VER  大小: $SIZE  MD5: $MD5"
        echo "[OTA] version文件已更新: $OTA_DIR/version"
    else
        echo "[OTA] 注意: OTA目录不存在 ($OTA_DIR)，跳过OTA推送"
        echo "[OTA] 请先创建: mkdir -p $OTA_DIR"
    fi

    echo ""
    echo "========================================="
    echo "  编译 & 部署完成！"
    echo "  可执行文件: $PROJECT_DIR/$EXEC_NAME"
    echo "  已部署到: root@192.168.171.74:/wz/$EXEC_NAME"
    echo "  OTA版本: $(cat "$OTA_DIR/version" 2>/dev/null | head -1 || echo N/A)"
    echo "  SSH到设备后执行: ./lvgl_fb"
    echo "========================================="
else
    echo ""
    echo "========================================="
    echo "  编译成功但部署失败！"
    echo "  可执行文件: $PROJECT_DIR/$EXEC_NAME"
    echo "  请手动部署: scp $PROJECT_DIR/$EXEC_NAME root@192.168.171.74:/wz/"
    echo "========================================="
    exit 1
fi