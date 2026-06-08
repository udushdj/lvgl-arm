/**
 * @file linked_list.c
 * @brief 患者信息带头结点单向链表 —— 完整CRUD + 排序 + 筛选实现
 * @author [wz]
 * @date 2024-03-27
 *
 * 数据结构:
 *   List (管理结构) → head (哨兵结点) → node1 → node2 → ... → NULL
 *                                                ↑ tail 指向最后一个数据结点
 *
 * 设计要点:
 *   - 头结点不存储患者数据，简化边界条件处理
 *   - tail 指针支持 O(1) 尾部插入
 *   - size 字段 O(1) 获取链表长度
 *   - 排序使用归并排序 (O(n log n), 稳定)
 *   - kill_list_node 使用迭代而非递归，避免长链表栈溢出
 *   - 所有公共API对 NULL 参数进行了防御性检查
 *
 * 内存管理约定:
 *   - 调用者负责释放 list_pop_front / delete_list 返回的结点
 *   - kill_List 释放链表所有内存 (含头结点和管理结构)
 *   - list_filter 返回新链表，调用者负责释放
 */

#include "PatientManager/inc/linked_list.h"
#include <string.h>
#include <time.h>

/* ==================== 辅助函数 ==================== */

/**
 * @brief 默认患者相等比较: 按 Patient_ID 判断是否同一人
 */
bool equal(P_Node e1, P_Node e2) {
    if (e1 == NULL || e2 == NULL) return false;
    return e1->Patient_ID == e2->Patient_ID;
}

/* ==================== 链表基本操作 ==================== */

/**
 * @brief 初始化空链表 —— calloc 分配管理结构和头结点
 * @return 成功返回链表指针，失败返回 NULL
 *
 * 初始状态: head->next = NULL, tail = head, size = 0
 */
P_List InitList(void) {
    P_List list = (P_List)calloc(1, sizeof(List));
    if (list == NULL) {
        perror("list分配失败");
        return NULL;
    }

    list->head = (P_Node)calloc(1, sizeof(struct patient));
    if (list->head == NULL) {
        perror("list->head分配失败");
        free(list);
        return NULL;
    }

    list->tail = list->head;
    list->size = 0;
    list->head->next = NULL;

    return list;
}

/**
 * @brief 判断链表是否为空 (NULL或size为0均视为空)
 */
bool isempty(P_List list) {
    if (list == NULL) return true;
    return (list->size == 0);
}

/**
 * @brief 创建测试用新患者结点 (静态计数器生成递增ID)
 * @note 仅供开发调试使用，生产环境患者数据来自服务器JSON
 */
P_Node make_new_Node(void) {
    P_Node newNode = (P_Node)calloc(1, sizeof(struct patient));
    if (newNode == NULL) {
        perror("newNode分配失败");
        return NULL;
    }

    static long id_counter = 1000;
    newNode->Patient_ID = id_counter++;

    snprintf(newNode->name, sizeof(newNode->name), "Pat%ld", newNode->Patient_ID);
    snprintf(newNode->ID_number, sizeof(newNode->ID_number), "440101%010ld", newNode->Patient_ID);
    newNode->age = 20 + (rand() % 50);

    newNode->next = NULL;
    return newNode;
}

/**
 * @brief 尾插法 —— O(1) 时间复杂度
 *
 * 将 node 追加到链表末尾，更新 tail 指针。
 * node->next 由调用者设为 NULL (或由 calloc 保证)。
 */
void list_push_back(P_List list, P_Node node) {
    if (list == NULL || node == NULL) return;

    list->tail->next = node;
    list->tail = node;
    list->size++;
}

/**
 * @brief 头插法 —— O(1) 时间复杂度
 *
 * 将 node 插入到头结点之后、第一个数据结点之前。
 * 如果原链表为空，同时更新 tail 指针。
 */
void list_push_front(P_List list, P_Node node) {
    if (list == NULL || node == NULL) return;

    node->next = list->head->next;
    list->head->next = node;

    if (isempty(list)) {
        list->tail = node;
    }

    list->size++;
}

/* ==================== 归并排序 (按 Patient_ID 升序) ==================== */

/**
 * @brief 归并排序递归核心 —— 快慢指针找中点，分治合并
 *
 * 时间复杂度 O(n log n)，空间复杂度 O(log n) (递归栈)。
 * 使用 dummy 结点简化合并逻辑。
 */
P_Node mergeSort(P_Node node) {
    if (node == NULL || node->next == NULL) {
        return node; /* 递归基: 空或单结点已有序 */
    }

    /* 快慢指针找中点 (快指针每次走两步，慢指针走一步) */
    P_Node fast = node->next;
    P_Node slow = node;
    while (fast != NULL && fast->next != NULL) {
        fast = fast->next->next;
        slow = slow->next;
    }

    P_Node left_part = node;
    P_Node right_part = slow->next;
    slow->next = NULL;  /* 断开连接，分成两个子链表 */

    left_part = mergeSort(left_part);
    right_part = mergeSort(right_part);

    /* 合并两个有序子链表 */
    P_Node dummy = (P_Node)malloc(sizeof(struct patient));
    if (dummy == NULL) {
        perror("归并排序分配临时节点失败");
        return NULL;
    }

    P_Node current = dummy;
    while (left_part != NULL && right_part != NULL) {
        if (left_part->Patient_ID < right_part->Patient_ID) {
            current->next = left_part;
            left_part = left_part->next;
        } else {
            current->next = right_part;
            right_part = right_part->next;
        }
        current = current->next;
    }

    /* 连接剩余部分 (最多一个非空) */
    if (left_part != NULL) {
        current->next = left_part;
    } else {
        current->next = right_part;
    }

    P_Node result = dummy->next;
    free(dummy);
    return result;
}

/**
 * @brief 对链表进行归并排序 (按 Patient_ID 升序)
 *
 * 排序后重新定位 tail 指针 (遍历到末尾)。
 */
bool mergeSort_list(P_List list) {
    if (list == NULL) return false;

    if (list->head->next == NULL || list->head->next->next == NULL) {
        return true; /* 空或单结点无需排序 */
    }

    P_Node sorted_head = mergeSort(list->head->next);
    list->head->next = sorted_head;

    /* 重新定位尾指针 */
    if (sorted_head != NULL) {
        P_Node temp = sorted_head;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        list->tail = temp;
    } else {
        list->tail = list->head;
    }

    return true;
}

/* ==================== 链表遍历与查找 ==================== */

/**
 * @brief 打印链表全部内容 (调试用)
 */
void show_list(P_List list) {
    if (list == NULL) {
        printf("链表指针为空\n");
        return;
    }

    if (isempty(list)) {
        printf("链表为空\n");
        return;
    }

    printf("head ->\n");
    P_Node tmp = list->head->next;
    int index = 1;
    while (tmp != NULL) {
        printf("  [%d] ID:%ld  Name:%-6s  ID_Num:%s  Age:%d\n",
               index++, tmp->Patient_ID, tmp->name, tmp->ID_number, tmp->age);
        tmp = tmp->next;
    }
    printf("tail (链表长度: %d)\n", list->size);
}

/**
 * @brief 在链表中查找匹配结点
 * @param equal 比较函数指针 (NULL则使用默认按ID比较)
 * @return 找到返回结点指针，未找到返回 NULL
 */
P_Node find_node(P_List list, P_Node to_find_node_e, EqualFunc equal) {
    if (isempty(list) || equal == NULL || to_find_node_e == NULL) return NULL;

    P_Node tmp = list->head->next;
    while (tmp != NULL) {
        if (equal(to_find_node_e, tmp)) {
            return tmp;
        }
        tmp = tmp->next;
    }
    return NULL;
}

/**
 * @brief 删除链表中第一个匹配的结点
 * @return 被删除的结点指针 (调用者负责free)，未找到返回 NULL
 *
 * 正确处理了删除尾结点时更新 tail 指针的情况。
 * 只删除第一个匹配项，如需删除所有匹配项请多次调用。
 */
P_Node delete_list(P_List list, P_Node e, EqualFunc equal) {
    if (isempty(list) || equal == NULL || e == NULL) return NULL;

    P_Node frontnode = list->head;  /* 前驱结点 (初始为头结点) */
    P_Node target = NULL;

    P_Node tmp = list->head->next;
    while (tmp != NULL) {
        if (equal(e, tmp)) {
            target = tmp;
            frontnode->next = tmp->next;

            if (tmp == list->tail) {
                list->tail = frontnode;
            }

            list->size--;
            break;
        }
        frontnode = tmp;
        tmp = tmp->next;
    }

    return target;
}

/* ==================== 结点移动操作 ==================== */

/**
 * @brief 弹出并返回第一个数据结点 (叫号操作)
 * @return 弹出结点 (调用者负责free)，空链表返回 NULL
 *
 * 更新 head->next 跳过第一个结点，更新 tail 和 size。
 * 返回的结点 next 已置为 NULL，可直接 free。
 */
P_Node list_pop_front(P_List list) {
    if (isempty(list)) return NULL;
    P_Node first = list->head->next;
    list->head->next = first->next;
    if (list->tail == first) {
        list->tail = list->head; /* 弹出后变空链表 */
    }
    list->size--;
    first->next = NULL;
    return first;
}

/**
 * @brief 将 target 结点向前移动一位 (与前一结点交换位置)
 * @return 成功 true，target为空/已是第一个/未找到则 false
 */
bool list_move_up(P_List list, P_Node target) {
    if (isempty(list) || target == NULL) return false;

    /* 寻找 target 的前驱 prev */
    P_Node prev = list->head;
    while (prev->next != NULL && prev->next != target) {
        prev = prev->next;
    }
    if (prev->next != target) return false;
    if (prev == list->head) return false; /* 已是第一个 */

    /* 找到 prev 的前驱 pre_prev */
    P_Node pre_prev = list->head;
    while (pre_prev->next != prev) {
        pre_prev = pre_prev->next;
    }

    /* 交换 prev 和 target */
    prev->next = target->next;
    target->next = prev;
    pre_prev->next = target;

    if (list->tail == target) list->tail = prev;
    return true;
}

/**
 * @brief 将 target 结点向后移动一位 (与后一结点交换位置)
 */
bool list_move_down(P_List list, P_Node target) {
    if (isempty(list) || target == NULL) return false;
    if (target->next == NULL) return false; /* 已是最后一个 */

    P_Node after = target->next;

    /* 找 target 的前驱 */
    P_Node prev = list->head;
    while (prev->next != NULL && prev->next != target) {
        prev = prev->next;
    }
    if (prev->next != target) return false;

    /* 交换 target 和 after */
    prev->next = after;
    target->next = after->next;
    after->next = target;

    if (list->tail == after) list->tail = target;
    return true;
}

/**
 * @brief 将 target 移至链表头部 (头结点之后第一个位置)
 *
 * 用于患者管理界面的"置顶"功能。
 * 如果target已经是第一个数据结点则直接返回true。
 */
bool list_move_to_top(P_List list, P_Node target) {
    if (isempty(list) || target == NULL) return false;
    if (list->head->next == target) return true; /* 已在顶部 */

    /* 从原位移除 */
    P_Node prev = list->head;
    while (prev->next != NULL && prev->next != target) {
        prev = prev->next;
    }
    if (prev->next != target) return false;
    prev->next = target->next;
    if (list->tail == target) list->tail = prev;

    /* 插入头部 */
    target->next = list->head->next;
    list->head->next = target;
    if (list->size == 1) list->tail = target;
    return true;
}

/* ==================== 通用排序 (支持自定义比较函数) ==================== */

/**
 * @brief 通用归并排序核心 (支持自定义比较器)
 *
 * 与 mergeSort 逻辑相同，但接受 CompareFunc 参数代替硬编码的 ID 比较。
 * 比较器返回: <0 表示 a 在 b 前, 0 表示相等, >0 表示 a 在 b 后
 */
P_Node mergeSort_generic(P_Node node, CompareFunc cmp) {
    if (node == NULL || node->next == NULL) return node;

    /* 快慢指针找中点 */
    P_Node fast = node->next, slow = node;
    while (fast && fast->next) {
        fast = fast->next->next;
        slow = slow->next;
    }
    P_Node right = slow->next;
    slow->next = NULL;

    P_Node left = mergeSort_generic(node, cmp);
    right = mergeSort_generic(right, cmp);

    /* 合并 */
    P_Node dummy = (P_Node)malloc(sizeof(patient_t));
    if (!dummy) return NULL;
    P_Node cur = dummy;
    while (left && right) {
        if (cmp(left, right) <= 0) {
            cur->next = left;
            left = left->next;
        } else {
            cur->next = right;
            right = right->next;
        }
        cur = cur->next;
    }
    cur->next = left ? left : right;
    P_Node res = dummy->next;
    free(dummy);
    return res;
}

/**
 * @brief 对链表进行排序 (自定义比较器)
 *
 * 排序后自动更新 tail 指针。
 */
bool list_sort(P_List list, CompareFunc cmp) {
    if (!list || !list->head->next || !list->head->next->next) return true;
    P_Node sorted = mergeSort_generic(list->head->next, cmp);
    list->head->next = sorted;

    /* 重新定位尾指针 */
    P_Node tmp = sorted;
    while (tmp->next) tmp = tmp->next;
    list->tail = tmp;
    return true;
}

/* ==================== 预定义比较函数 ==================== */

int cmp_by_ID_desc(P_Node a, P_Node b) {
    return cmp_by_ID(b, a); /* 反转升序结果即降序 */
}

int cmp_by_ID(P_Node a, P_Node b) {
    /* 使用差值法避免分支 (a>b返回1, a<b返回-1, a==b返回0) */
    return (a->Patient_ID > b->Patient_ID) - (a->Patient_ID < b->Patient_ID);
}

int cmp_by_name(P_Node a, P_Node b) {
    return strcmp(a->name, b->name);
}

int cmp_by_age(P_Node a, P_Node b) {
    return a->age - b->age;
}

int cmp_by_ID_number(P_Node a, P_Node b) {
    return strcmp(a->ID_number, b->ID_number);
}

/* ==================== 筛选操作 ==================== */

/**
 * @brief 筛选满足 predicate 的结点，组成新链表返回
 *
 * 遍历原链表，对每个结点调用 predicate。
 * 满足条件的结点被 memcpy 复制到新链表。
 *
 * @param predicate 判断函数，返回 true 表示保留该结点
 * @return 新链表 (调用者负责 kill_List 释放)，失败或输入无效返回 NULL
 */
P_List list_filter(P_List list, bool (*predicate)(P_Node)) {
    if (!list || !predicate) return NULL;
    P_List newlist = InitList();
    if (!newlist) return NULL;
    P_Node cur = list->head->next;
    while (cur) {
        if (predicate(cur)) {
            P_Node copy = (P_Node)malloc(sizeof(patient_t));
            if (copy) {
                memcpy(copy, cur, sizeof(patient_t));
                copy->next = NULL;
                list_push_back(newlist, copy);
            }
        }
        cur = cur->next;
    }
    return newlist;
}

/* ==================== 内存释放 ==================== */

/**
 * @brief 迭代释放链表所有结点 (内部辅助函数)
 *
 * 使用迭代而非递归，避免长链表导致栈溢出。
 * ARM嵌入式系统默认栈空间有限，递归深度过大可能造成不可预期的崩溃。
 *
 * @param head 链表头结点 (含哨兵结点)
 */
void kill_list_node(P_Node head) {
    while (head != NULL) {
        P_Node next = head->next;
        free(head);
        head = next;
    }
}

/**
 * @brief 销毁整个链表 —— 释放所有结点 + 管理结构体
 *
 * 调用后 list 指针变为悬空指针，调用者应将其置为 NULL。
 */
void kill_List(P_List list) {
    if (list == NULL) return;
    kill_list_node(list->head); /* 释放所有结点 (含头结点) */
    free(list);                  /* 释放管理结构体 */
}
