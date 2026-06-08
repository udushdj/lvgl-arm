<?php
/**
 * 医疗叫号系统 API
 * GET  /api.php              -> 返回患者队列JSON
 * POST /api.php              -> 接收患者队列JSON，写入 patient_queue.json
 * POST /api.php?target=stats -> 接收统计数据JSON，写入 stats.json
 */

$QUEUE_FILE = __DIR__ . '/patient_queue.json';
$STATS_FILE = __DIR__ . '/stats.json';

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    header('Access-Control-Allow-Origin: *');
    header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
    header('Access-Control-Allow-Headers: Content-Type');
    http_response_code(204);
    exit;
}

header('Content-Type: application/json; charset=utf-8');
header('Access-Control-Allow-Origin: *');

if ($_SERVER['REQUEST_METHOD'] === 'GET') {
    if (file_exists($QUEUE_FILE)) {
        readfile($QUEUE_FILE);
    } else {
        echo '[]';
    }
    exit;
}

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $target = isset($_GET['target']) ? $_GET['target'] : 'queue';
    $input = file_get_contents('php://input');
    $data = json_decode($input, true);

    if ($data === null) {
        http_response_code(400);
        echo json_encode(['error' => '无效的JSON格式'], JSON_UNESCAPED_UNICODE);
        exit;
    }

    if ($target === 'stats') {
        if (!isset($data['today_called']) || !isset($data['hourly_calls'])) {
            http_response_code(400);
            echo json_encode(['error' => '统计数据字段不完整'], JSON_UNESCAPED_UNICODE);
            exit;
        }
        $json = json_encode($data, JSON_UNESCAPED_UNICODE | JSON_PRETTY_PRINT);
        if (file_put_contents($STATS_FILE, $json) === false) {
            http_response_code(500);
            echo json_encode(['error' => 'stats.json 写入失败'], JSON_UNESCAPED_UNICODE);
            exit;
        }
        echo json_encode(['success' => true, 'target' => 'stats'], JSON_UNESCAPED_UNICODE);
        exit;
    }

    if (!is_array($data)) {
        http_response_code(400);
        echo json_encode(['error' => '患者队列应为JSON数组'], JSON_UNESCAPED_UNICODE);
        exit;
    }

    foreach ($data as $item) {
        if (!isset($item['Patient_ID']) || !isset($item['name']) ||
            !isset($item['ID_number']) || !isset($item['age'])) {
            http_response_code(400);
            echo json_encode(['error' => '患者数据字段不完整'], JSON_UNESCAPED_UNICODE);
            exit;
        }
    }

    $json = json_encode($data, JSON_UNESCAPED_UNICODE | JSON_PRETTY_PRINT);
    if (file_put_contents($QUEUE_FILE, $json) === false) {
        http_response_code(500);
        echo json_encode(['error' => '文件写入失败'], JSON_UNESCAPED_UNICODE);
        exit;
    }

    echo json_encode(['success' => true, 'count' => count($data)], JSON_UNESCAPED_UNICODE);
    exit;
}

http_response_code(405);
echo json_encode(['error' => '方法不允许'], JSON_UNESCAPED_UNICODE);
