#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>          // 为可能的字符串比较提供声明

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 患者节点与链表结构 ========== */
typedef struct patient {
    char ID_number[19];      /**< 身份证号 */
    char name[6];            /**< 姓名 */
    long Patient_ID;         /**< 患者唯一编号 */
    int age;                 /**< 年龄 */
    struct patient *next;    /**< 指向下一个节点 */
} patient_t, *P_Node;

typedef struct Patient_list {
    int size;                /**< 节点数量（不含头结点） */
    P_Node head;             /**< 头结点 */
    P_Node tail;             /**< 尾节点 */
} List, *P_List;

/* ========== 比较函数指针类型 ========== */
typedef bool (*EqualFunc)(P_Node, P_Node);       // 相等判断（查找/删除）
typedef int  (*CompareFunc)(P_Node, P_Node);     // 排序比较，返回值 <0, =0, >0

/* ========== 辅助函数 ========== */
bool equal(P_Node e1, P_Node e2);                // 默认相等比较（按 Patient_ID）

/* ========== 基本操作 ========== */
P_List InitList(void);                           // 初始化链表（含头结点）
bool isempty(P_List list);                       // 链表是否为空
P_Node make_new_Node(void);                      // 创建测试节点（可删除）
void list_push_back(P_List list, P_Node node);   // 尾插
void list_push_front(P_List list, P_Node node);  // 头插

/* ========== 队列操作 ========== */
P_Node list_pop_front(P_List list);              // 弹出第一个节点（叫号）

/* ========== 节点移动 ========== */
bool list_move_up(P_List list, P_Node target);        // 上移一位
bool list_move_down(P_List list, P_Node target);      // 下移一位
bool list_move_to_top(P_List list, P_Node target);    // 置顶

/* ========== 排序 ========== */
P_Node mergeSort_generic(P_Node node, CompareFunc cmp);  // 通用归并排序（内部使用）
bool list_sort(P_List list, CompareFunc cmp);            // 对链表排序
// 预定义比较函数
int cmp_by_ID_desc(P_Node a, P_Node b);                  // 按 Patient_ID 降序
int cmp_by_ID(P_Node a, P_Node b);                       // 按 Patient_ID 升序
int cmp_by_name(P_Node a, P_Node b);                     // 按姓名升序
int cmp_by_age(P_Node a, P_Node b);                      // 按年龄升序
int cmp_by_ID_number(P_Node a, P_Node b);                // 按身份证号升序

/* ========== 查找与删除 ========== */
P_Node find_node(P_List list, P_Node to_find_node_e, EqualFunc equal);
P_Node delete_list(P_List list, P_Node e, EqualFunc equal);   // 删除并返回节点，调用者需释放

/* ========== 条件筛选 ========== */
P_List list_filter(P_List list, bool (*predicate)(P_Node));    // 返回新链表（需自行销毁）

/* ========== 显示与销毁 ========== */
void show_list(P_List list);
void kill_list_node(P_Node head);   // 递归释放节点
void kill_List(P_List list);        // 销毁整个链表

#ifdef __cplusplus
}
#endif

#endif /* LINKED_LIST_H */