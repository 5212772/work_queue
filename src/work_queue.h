/************************************************************************************************/
/* Copyright (C), 2016-2018  5212772@qq.com                                                     */
/************************************************************************************************/
/**
 * @file work_queue.h
 * @brief 工作队列。目的是降低系统的进程数量，从而降低系统开销。
 *        该工作队列主要用于对循环周期较长，一般大于400~500ms以上的周期任务执行。
 *        或者对实时性要求较低的任务单次处理，或延后处理。
 *        该工作队列的设计特点：
 *        1. 节省系统资源。通过设计上的处理，避免队列 manager 的负载轮询循环。
 *        2. 该功能主要包括两个主体： work_queue(manager) + task(任务)
 *        3. task(任务) 简便的添加删除，以及忽略挂起操作。
 *        4. work_queue 多实例创建，即可以创建多个work_queue(manager)，管理不同的task
 *        5. 同一个 work_queue 可以对其管理的task进行负载平衡。简单的说就是当一个work_queue管理下的
 *           task过多或处理时间过长，work_queue会视情况多增加或者删除，一个或多个线程对task进行处理。
 *           即平衡了task得到处理的响应时间，也动态的平衡了系统负载(增加线程造成)。
 *        6. 动态统计task的执行情况，如果出现某个task卡死或耗时异常。计划设计成:将该队列中的其他task
 *           移出到正常的work_queue(manager)中继续执行，避免正常的task被阻塞，同时向外界报错。
 *        7. work_queue可以动态显示执行情况和task执行情况，方便debug。至于如何启动打印，考虑两种方式
 *           进行. a: 通过函数API调用实现。方便整个系统的debug模块调用，做统一处理;  b: 通过在文件系
 *           统中生成管道，work_queue通过轮询管道获取的命令，来实现状态显示.
 *        8. task有多种运行模式，使用时，需要对task本身的处理函数做好响应处理，尤其是多次循环task的函
 *           数，以及单次task处理的时间不宜过长。
 * @author id: wangguixing
 * @Email: 5212772@qq.com
 * @version v0.1
 * @date 2018-04-14
 */

#ifndef _WORK_QUEUE_H_
#define _WORK_QUEUE_H_

#include <errno.h>
#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>

/************************************************************************************************/
/*                                      Include Files                                           */
/************************************************************************************************/
/* None */

/************************************************************************************************/
/*                                     Macros & Typedefs                                        */
/************************************************************************************************/


#ifdef INFO_DEBUG
#define WQ_INFO(fmt, args...)                                          \
    do {                                                                  \
        printf("WQ [FUN]%s [LINE]%d  " fmt, __FUNCTION__, __LINE__, ##args); \
    } while (0)
#else
#define WQ_INFO(fmt, args...)                                          \
    do {} while (0)
#endif

#ifdef ERR_DEBUG
#define WQ_ERR(fmt, args...)                                                                        \
    do {                                                                                                 \
        printf("\033[0;32;31m ERROR! [FUN]%s [LINE]%d  " fmt "\033[0m", __FUNCTION__, __LINE__, ##args); \
    } while (0)
#else
#define WQ_ERR(fmt, args...)                                          \
    do {} while (0)
#endif


#define ONE_QUEUE_MAX_TASK_NUM      32
#define MAX_WORK_QUEUE_NUM          16


typedef enum tag_WORK_QUEUE_STATUS_E
{
    WQ_UNINIT = 0,
    WQ_RUNNING,
    WQ_IDLE,
    WQ_STOP,
    WQ_STATUS_BOTTON,
} WORK_QUEUE_STATUS_E;


typedef enum tag_TASK_WORK_MODE_E
{
    TASK_MODE_LOOP = 0,
    TASK_MODE_ONCE,
    TASK_MODE_ONCE_DELAY,
    TASK_MODE_BOTTON,
} TASK_WORK_MODE_E;


typedef enum tag_TASK_STATUS_E
{
    TASK_RUNNING = 0,
    TASK_IDLE = 0,
    TASK_STOP = 0,
    TASK_STATUS_BOTTON,
} TASK_STATUS_E;

typedef int (*task_func)(void *param, void *data);


/************************************************************************************************/
/*                                    Structure Declarations                                    */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                      Global Variables                                        */
/************************************************************************************************/
/* None */

/************************************************************************************************/
/*                                    Function Declarations                                     */
/************************************************************************************************/

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/**
 * @brief For display work queue info
 * @param 
 * - poll_time  work queue basic poll time. unit: ms.
 * - wq_id      work queue ID number. Can be NULL.
 * @return
 *  - wq_id  >=0 (success)
 *  - fail    -1
 */
int create_work_queue(int poll_time, int *wq_id);
int destroy_work_queue(int wq_id);
int start_work_queue(int wq_id);
int stop_work_queue(int wq_id);

int set_work_queue_poll_time(int wq_id, int poll_time);
int get_work_queue_status(int wq_id, WORK_QUEUE_STATUS_E * status);
int get_all_work_queue_number(int * wq_number);
int get_all_work_queue_id(int id_array[MAX_WORK_QUEUE_NUM], int * wq_numer);
int get_all_task_number(int wq_id, int * task_number);


/**
 * 此处主要声明的是启/停工作队列宏函数。
 * 即 start_work_queue_2 和 stop_work_queue_2 
 * 此处主要声明的是启/停工作队列宏函数。这样定义的好处是知道整个系统
 * 运行过程中是哪个函数进行了工作队列的启停，方便后续调试跟踪。
 * 但这个两个函数的调用也不是必须是用的，根据实际情况使用即可。
**/
int _start_work_queue(int wq_id);
#define start_work_queue_2(wq_id) \
    do {  \
        _start_work_queue((wq_id), __FUNCTION__); \
    } while (0)

int _stop_work_queue(int wq_id);
#define stop_work_queue_2(wq_id) \
    do {  \
        _stop_work_queue((wq_id), __FUNCTION__); \
    } while (0)


/**
 * @brief add task to work_queue
 * @param 
 * - wq_id        work queue ID number.
 * - task_name    the task name.  Can be NULL
 * - mode         the task work mode. e.g once, loop...
 * - loop_time    the task loop time.  
 *                mode==once: time is invalid. 
 *                mode==loop: task running loop time
 *                mode==once_delay: is delay time. 
 * - handler      task handle function. Can't be NULL!
 * - param        task handler function input param.  Can be NULL.
 * - data         task handler function input or output param. Can be NULL
 * - task_id      if success, return task_id. If fail return NULL. Can be NULL
 * @return
 *  - task_id  >=0 (success)
 *  - fail    -1
 */
int add_task(int wq_id, char *task_name, TASK_WORK_MODE_E mode, int loop_time, 
             task_func handler, void *param, void *data, int *task_id);
int delete_task(int wq_id, int task_id);
int start_task(int wq_id, int task_id);
int stop_task(int wq_id, int task_id);
int get_task_looptime(int wq_id, int task_id, int *loop_time);
int set_task_looptime(int wq_id, int task_id, int loop_time);
int get_task_status(int wq_id, int task_id, TASK_STATUS_E *status);


/**
 * @brief For display work queue info
 * @param 
 * - wq_id   work queue ID number.
 * - times   Display work queue running status info times
 * @return
 *  - success  0
 *  - fail    -1
 */
int display_work_queue_info(int wq_id, int display_times);

//TODO: 其实我自己更倾向于通过在系统中建立有名管道来进行work_queue的状态打印调试的。

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif


#endif /* _WORK_QUEUE_H_ */
