/************************************************************************************************/
/* Copyright (C), 2016-2019     5212772@qq.com                                                  */
/************************************************************************************************/
/**
 * @file work_queue.c
 * @brief : work queue src file.
 * @author id: wangguixing
 * @version v0.1 create
 * @date 2018-04-14
 */

/************************************************************************************************/
/*                                      Include Files                                           */
/************************************************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <sys/prctl.h>

#include "work_queue.h"

/************************************************************************************************/
/*                                     Macros & Typedefs                                        */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                    Structure Declarations                                    */
/************************************************************************************************/
typedef struct tag_WORK_TASK_S {
    char                task_name[128];
    int                 task_id;
    TASK_WORK_MODE_E    work_mode;
    long                loop_time;         /* This task execution cycle.  Unit:ms */
    task_func           handler;           /* work_queue task handle function  */
    void               *param;             /* task function input param     */
    void               *data;              /* task function I/O data        */

    long                next_start_time;   /* The next task start time.   Unit:ms */
    long                run_time;          /* last run time.              Unit:ms */
    long                max_run_time;      /* The Most task run time.     Unit:ms */
    long                average_run_time;  /* This task average run time. Unit:ms */
    long                run_number;        /* This task total run numbers   */
    TASK_STATUS_E       run_flag;          /* Task control flag. only RUNNING and STOP is valid */
    TASK_STATUS_E       status;

    struct tag_WORK_TASK_S  *next_task;
    struct tag_WORK_TASK_S  *prev_task;
} WORK_TASK_S, *P_WORK_TASK_S;


typedef struct tag_WORK_QUEUE_S {
    int                 group_id;
    int                 task_num;
    long                cur_time;
    unsigned int        idle_num;
    unsigned int        unit_time;
    unsigned int        poll_time;
    pthread_mutex_t     operate_lock;
    pthread_t           thread_id;
    sem_t               sem;
    WORK_QUEUE_STATUS_E run_flag;
    WORK_QUEUE_STATUS_E status;

    P_WORK_TASK_S       task_head;
    P_WORK_TASK_S       task_current;

    struct tag_WORK_QUEUE_S  *next_wq;
} WORK_QUEUE_S, *P_WORK_QUEUE_S;


/************************************************************************************************/
/*                                      Global Variables                                        */
/************************************************************************************************/
static WORK_QUEUE_S g_wq_array[MAX_WORK_QUEUE_NUM] = {};

static int              g_wq_init_flag      = 0;
static int              g_manage_run_flag   = 0;
static pthread_t        g_manage_thread_id  = 0;
static pthread_mutex_t  g_operate_lock      = PTHREAD_MUTEX_INITIALIZER;



/************************************************************************************************/
/*                                    Function Declarations                                     */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                     Function Definitions                                     */
/************************************************************************************************/

static int get_valid_wq_id(int *wq_id)
{
    int cnt = 0;
    for (cnt = 0; cnt < MAX_WORK_QUEUE_NUM; cnt++) {
        if (-1 == g_wq_array[cnt].group_id || WQ_UNINIT == g_wq_array[cnt].status)
            break;
    }

    if (cnt < MAX_WORK_QUEUE_NUM) {
        if (wq_id)
            *wq_id = cnt;
        return cnt;
    } else {
        if (wq_id)
            *wq_id = -1;
        return -1;
    }
}

static int get_valid_task_id(int wq_id, int *task_id)
{
    int id = 0;
    P_WORK_TASK_S p_task = NULL;

    if (wq_id >= MAX_WORK_QUEUE_NUM) {
        WQ_ERR("Input wq_id error! wq_id:%d  MAX_ID:%d\n", wq_id, MAX_WORK_QUEUE_NUM);
        return -1;
    }

    if (WQ_UNINIT == g_wq_array[wq_id].status || -1 == g_wq_array[wq_id].group_id) {
        WQ_ERR("The wq_id:%d work_queue don't init! \n", wq_id);
        return -1;
    }

    id = 0;
    p_task = g_wq_array[wq_id].task_head;
    while(1) {
        if (NULL == p_task)
            break;

        if (p_task->task_id > id)
            id = p_task->task_id;
        p_task = p_task->next_task;
    }
    id++;

    if (task_id)
        *task_id = id;

    return id;
}


static int check_wq_id(int wq_id)
{
    if (wq_id >= MAX_WORK_QUEUE_NUM) {
        WQ_ERR("Input wq_id error! wq_id:%d  MAX_ID:%d\n", wq_id, MAX_WORK_QUEUE_NUM);
        return -1;
    }

    if(pthread_mutex_lock(&g_wq_array[wq_id].operate_lock) != 0) {
        WQ_ERR("The id:%d work_queue don't init! \n", wq_id);
        return -1;
    }

    if (WQ_UNINIT == g_wq_array[wq_id].status || \
        -1 == g_wq_array[wq_id].group_id || \
        WQ_STATUS_BOTTON == g_wq_array[wq_id].status) {
        WQ_ERR("The id:%d work_queue don't init! \n", wq_id);
        pthread_mutex_unlock(&g_wq_array[wq_id].operate_lock);
        return -1;
    }

    pthread_mutex_unlock(&g_wq_array[wq_id].operate_lock);
    return 0;
}


static void * work_queue_manager(void *para)
{
    int  ret = 0;

    prctl(PR_SET_NAME, "WQ_manager", 0, 0, 0);

    while (g_manage_run_flag) {

        //WQ_INFO("  ============= guixing === \n"); sleep(3);
        usleep(50 * 1000);  /* 50ms */
    }

    return NULL;
}


static void * work_queue_proccess(void *para)
{
    int  ret   = 0;
    int  wq_id = 0;
    char name[64] = {0};

    if (NULL == para) {
        WQ_ERR("Input para wq_id is NULL!!!\n");
        return NULL;
    }

    wq_id = *(int*)para;
    if (wq_id >= MAX_WORK_QUEUE_NUM) {
        WQ_ERR("Input wq_id:%d error!!!\n", wq_id);
        return NULL;
    }

    snprintf(name, 63, "WQ_proc_ID_%d", wq_id);
    prctl(PR_SET_NAME, name, 0, 0, 0);
    WQ_INFO(" The %s proccess have create.\n", name);

    while (1) {
        /* This part is run control function */
        usleep(g_wq_array[wq_id].unit_time * 1000);  /* 50ms */



        usleep(300 * 1000);

        pthread_mutex_lock(&g_wq_array[wq_id].operate_lock);
        if (WQ_EXIT == g_wq_array[wq_id].run_flag || WQ_UNINIT == g_wq_array[wq_id].run_flag) {
            WQ_INFO(" The ID:%d work_queue proccess exit! \n", wq_id);
            pthread_mutex_unlock(&g_wq_array[wq_id].operate_lock);
            break;
        } else if (WQ_STOP == g_wq_array[wq_id].run_flag) {
            pthread_mutex_unlock(&g_wq_array[wq_id].operate_lock);
            continue;
        }

        //TODO samething!
        WQ_INFO("  ... ID:%d proccess ... \n", wq_id); usleep(800 * 1000);

        pthread_mutex_unlock(&g_wq_array[wq_id].operate_lock);
    }

    //WQ_INFO("  ======ID:%d proccess === guixing === Bybte ! \n", wq_id); usleep(200 * 1000);

    return NULL;
}



//static int init_work_queue(void)
int init_work_queue(void)
{
    int            ret = 0;
    int            idx = 0;
    pthread_attr_t attr;

    if (g_wq_init_flag) {
        return 0;
    }

    memset(g_wq_array, 0, sizeof(WORK_QUEUE_S) * MAX_WORK_QUEUE_NUM);
    for (idx = 0; idx < MAX_WORK_QUEUE_NUM; idx++) {
        g_wq_array[idx].group_id = -1;
        g_wq_array[idx].run_flag = WQ_UNINIT;
        g_wq_array[idx].status   = WQ_UNINIT;
        pthread_mutex_init(&g_wq_array[idx].operate_lock, NULL);
        #if 0
        if (sem_init(&g_wq_array[idx].sem, 0, 0) == -1) {
            WQ_ERR("Do sem_init fail! errno:%d %s \n", errno, perror(errno));
        }
        #endif
    }

    g_manage_run_flag = 1;
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&g_manage_thread_id, &attr, work_queue_manager, NULL);
    usleep(10);
    pthread_attr_destroy(&attr);
    if (ret) {
        WQ_ERR("Create pthread fail! ret:%d  error:%s\n", ret, strerror(errno));
		return ret;
    }

    g_wq_init_flag = 1;

    return 0;
}

//static int exit_work_queue(void)
int exit_work_queue(void)
{
    int idx = 0;

    g_manage_run_flag = 0;
    g_wq_init_flag    = 0;
    memset(g_wq_array,   0, sizeof(WORK_QUEUE_S) * MAX_WORK_QUEUE_NUM);
    for (idx = 0; idx < MAX_WORK_QUEUE_NUM; idx++) {
        g_wq_array[idx].group_id = -1;
        g_wq_array[idx].status   = WQ_UNINIT;
        g_wq_array[idx].run_flag = WQ_UNINIT;
    }

    WQ_INFO("All of work queue haved destroy. So exit work queue and destroy manager thread.\n");
    //pthread_join(g_manage_thread_id, NULL);

    return 0;
}


/**
 * @brief For display work queue info
 * @param
 * - poll_time  work queue basic poll time. unit: ms.
 * - wq_id      work queue ID number. Can be NULL.
 * @return
 *  - wq_id  >=0 (success)
 *  - fail    -1
 */
int create_work_queue(int poll_time, int *wq_id)
{
    int ret = 0;
    int id  = 0;
    pthread_attr_t attr;

    pthread_mutex_lock(&g_operate_lock);
    if (0 == g_wq_init_flag) {
        ret = init_work_queue();
        if (ret) {
            pthread_mutex_unlock(&g_operate_lock);
            WQ_ERR("Do init_work_queue fail! ret:%d", ret);
            return -1;
        }
    }

    ret = get_valid_wq_id(&id);
    if (ret < 0) {
        pthread_mutex_unlock(&g_operate_lock);
        WQ_ERR("Get valid wq_id fail! ret:%d", ret);
        return -1;
    }

    g_wq_array[id].group_id     = id;
    g_wq_array[id].task_num     = 0;
    g_wq_array[id].cur_time     = 0;
    g_wq_array[id].idle_num     = 0;
    g_wq_array[id].unit_time    = poll_time;
    g_wq_array[id].poll_time    = poll_time;
    g_wq_array[id].task_head    = NULL;
    g_wq_array[id].task_current = NULL;
    g_wq_array[id].next_wq      = NULL;

    #if 0
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&g_wq_array[id].thread_id, &attr, work_queue_proccess, (void *)&g_wq_array[id].group_id);
    usleep(10);
    pthread_attr_destroy(&attr);
    if (ret) {
        WQ_ERR ("Create pthread fail! ret:%d  error:%s\n", ret, strerror(errno));
		return ret;
    }
    #else
    ret = pthread_create(&g_wq_array[id].thread_id, NULL, work_queue_proccess, (void *)&g_wq_array[id].group_id);
    if (ret) {
        WQ_ERR ("Create pthread fail! ret:%d  error:%s\n", ret, strerror(errno));
		return ret;
    }
    usleep(10);
    #endif

    g_wq_array[id].run_flag = WQ_RUNNING;
    g_wq_array[id].status   = WQ_IDLE;

    pthread_mutex_unlock(&g_operate_lock);

    if (NULL != wq_id) {
        *wq_id = id;
    }

    return id;
}


int destroy_work_queue(int wq_id)
{
    int ret = 0, cnt = 0;
    int exit_flag = 0;
    P_WORK_TASK_S p_task = NULL, p_next = NULL;

    pthread_mutex_lock(&g_operate_lock);

    ret = check_wq_id(wq_id);
    if (ret) {
        pthread_mutex_unlock(&g_operate_lock);
        WQ_ERR("Input wq_id error! wq_id:%d  MAX_ID:%d\n", wq_id, MAX_WORK_QUEUE_NUM);
        return -1;
    }

    g_wq_array[wq_id].run_flag = WQ_EXIT;

    #if 1
    pthread_join(g_wq_array[wq_id].thread_id, NULL);
    WQ_INFO("  Join the wq_id:%d exit. And destroy wq!\n", wq_id);
    #else
    ret = sem_wait(&g_wq_array[wq_id].sem);
    if (ret) {
        WQ_ERR("Do sem_wait fail! errno:%d %s \n", errno, perror(errno));
    }
    #endif

    /* release all task */
    p_task = g_wq_array[wq_id].task_head;
    while (1) {
        if (NULL == p_task) {
            break;
        }
        p_next = p_task->next_task;
        p_task->run_flag = TASK_STOP;
        WQ_ERR("Free wq_id:%d  task_ID:%d\n", wq_id, p_task->task_id);
        free(p_task);
        p_task = NULL;
        p_task = p_next;
    }
    g_wq_array[wq_id].group_id     = -1;
    g_wq_array[wq_id].task_num     = 0;
    g_wq_array[wq_id].cur_time     = 0;
    g_wq_array[wq_id].idle_num     = 0;
    g_wq_array[wq_id].unit_time    = 0;
    g_wq_array[wq_id].poll_time    = 0;
    g_wq_array[wq_id].task_head    = NULL;
    g_wq_array[wq_id].task_current = NULL;
    g_wq_array[wq_id].next_wq      = NULL;
    g_wq_array[wq_id].status       = WQ_UNINIT;
    g_wq_array[wq_id].run_flag     = WQ_UNINIT;

    for (cnt = 0; cnt < MAX_WORK_QUEUE_NUM; cnt++) {
        if (-1 != g_wq_array[cnt].group_id || WQ_UNINIT != g_wq_array[cnt].status)
            break;
    }

    if (cnt >= MAX_WORK_QUEUE_NUM) {
        if ((ret = exit_work_queue()))
            WQ_ERR("Do exit_work_queue fail! ret:%d\n", ret);
    }

    pthread_mutex_unlock(&g_operate_lock);

    return 0;
}


int start_work_queue(int wq_id)
{
    int ret = 0;

    if (wq_id >= MAX_WORK_QUEUE_NUM) {
        WQ_ERR("Input wq_id error! wq_id:%d  MAX_ID:%d\n", wq_id, MAX_WORK_QUEUE_NUM);
        return -1;
    }

    if(pthread_mutex_lock(&g_wq_array[wq_id].operate_lock) != 0) {
        WQ_ERR("The id:%d work_queue don't init! \n", wq_id);
        return -1;
    }

    if (WQ_UNINIT == g_wq_array[wq_id].status || -1 == g_wq_array[wq_id].group_id) {
        WQ_ERR("The id:%d work_queue don't init! \n", wq_id);
        pthread_mutex_unlock(&g_wq_array[wq_id].operate_lock);
        return -1;
    }

    pthread_mutex_lock(&g_operate_lock);
    g_wq_array[wq_id].run_flag = WQ_RUNNING;
    pthread_mutex_unlock(&g_operate_lock);

    pthread_mutex_unlock(&g_wq_array[wq_id].operate_lock);

    WQ_INFO("ssssss wq_id:%d \n", wq_id);
    return 0;
}


int stop_work_queue(int wq_id)
{
    int ret = 0;

    if (wq_id >= MAX_WORK_QUEUE_NUM) {
        WQ_ERR("Input wq_id error! wq_id:%d  MAX_ID:%d\n", wq_id, MAX_WORK_QUEUE_NUM);
        return -1;
    }

    if(pthread_mutex_lock(&g_wq_array[wq_id].operate_lock) != 0) {
        WQ_ERR("The id:%d work_queue don't init! \n", wq_id);
        return -1;
    }

    if (WQ_UNINIT == g_wq_array[wq_id].status || -1 == g_wq_array[wq_id].group_id) {
        WQ_ERR("The id:%d work_queue don't init! \n", wq_id);
        pthread_mutex_unlock(&g_wq_array[wq_id].operate_lock);
        return -1;
    }

    pthread_mutex_lock(&g_operate_lock);
    g_wq_array[wq_id].run_flag = WQ_STOP;
    pthread_mutex_unlock(&g_operate_lock);

    pthread_mutex_unlock(&g_wq_array[wq_id].operate_lock);

    WQ_INFO("*** wq_id:%d \n", wq_id);

    return 0;
}


int set_work_queue_poll_time(int wq_id, int poll_time)
{
    int ret = 0;

    if (wq_id >= MAX_WORK_QUEUE_NUM) {
        WQ_ERR("Input wq_id error! wq_id:%d  MAX_ID:%d\n", wq_id, MAX_WORK_QUEUE_NUM);
        return -1;
    }

    if (poll_time < 20 || poll_time > 1000) {
        WQ_ERR("Input poll_time:%d outof limite[20ms < pool < 1000ms], so fix poll_time = 50ms \n", poll_time);
        poll_time = 50;
        ret = -1;
    }
    g_wq_array[wq_id].unit_time = poll_time;

    return ret;
}

int get_work_queue_status(int wq_id, WORK_QUEUE_STATUS_E * status)
{
    if (wq_id >= MAX_WORK_QUEUE_NUM) {
        WQ_ERR("Input wq_id error! wq_id:%d  MAX_ID:%d\n", wq_id, MAX_WORK_QUEUE_NUM);
        return -1;
    }

    if (NULL == status) {
        WQ_ERR("Input status is NULL!\n");
        return -1;
    }

    *status = g_wq_array[wq_id].status;
    return 0;
}

int get_all_work_queue_number(int * wq_number)
{
    int cnt = 0, idx = 0, i = 0;
    P_WORK_QUEUE_S p_wq = NULL;

    if (NULL == wq_number) {
        WQ_ERR("Input wq_number is NULL!\n");
        return -1;
    }

    pthread_mutex_lock(&g_operate_lock);
    for (idx = 0; idx < MAX_WORK_QUEUE_NUM; idx++) {
        if (g_wq_array[idx].group_id >= 0 && WQ_UNINIT != g_wq_array[idx].status) {
            cnt++;
            #if 0
            p_wq = g_wq_array[idx].next_wq;
            for (;;) {
                if (NULL != p_wq) {
                    if (p_wq->group_id > 0 && WQ_UNINIT != p_wq->status)
                        cnt++;
                    p_wq = p_wq->next_wq;
                } else {
                    break;
                }
            }
            #endif
        }
    }
    *wq_number = cnt;
    pthread_mutex_unlock(&g_operate_lock);

    WQ_INFO("^^^^^ Current wq number:%d  \n", cnt);

    return 0;
}


int get_all_work_queue_id(int id_array[MAX_WORK_QUEUE_NUM], int * wq_number)
{
    int cnt = 0, idx = 0, i = 0;
    P_WORK_QUEUE_S p_wq = NULL;

    if (NULL == id_array || NULL == wq_number) {
        WQ_ERR("Input id_array or wq_number is NULL!\n");
        return -1;
    }

    pthread_mutex_lock(&g_operate_lock);
    for (idx = 0; idx < MAX_WORK_QUEUE_NUM; idx++) {
        if (g_wq_array[idx].group_id >= 0 && WQ_UNINIT != g_wq_array[idx].status) {
            id_array[i] = g_wq_array[idx].group_id;
            i++;
            cnt++;
            #if 0
            p_wq = g_wq_array[idx].next_wq;
            for (;;) {
                if (NULL != p_wq) {
                    if (p_wq->group_id > 0 && WQ_UNINIT != p_wq->status)
                        cnt++;
                    p_wq = p_wq->next_wq;
                } else {
                    break;
                }
            }
            #endif
        }
    }
    *wq_number = cnt;
    pthread_mutex_unlock(&g_operate_lock);

    for (i = 0; i < cnt; i++)
        WQ_INFO("^^^^^ Current %d:wq_group_ID[%d]  \n", cnt, id_array[i]);

    return 0;
}


int get_all_task_number(int wq_id, int * task_number)
{
    int cnt = 0;
    P_WORK_TASK_S p_task = NULL;

    if (NULL == task_number) {
        WQ_ERR("Input task_number is NULL!\n");
        return -1;
    }

    if (wq_id >= MAX_WORK_QUEUE_NUM) {
        WQ_ERR("Input wq_id error! wq_id:%d  MAX_ID:%d\n", wq_id, MAX_WORK_QUEUE_NUM);
        return -1;
    }

    if(pthread_mutex_lock(&g_wq_array[wq_id].operate_lock) != 0) {
        WQ_ERR("The id:%d work_queue don't init! \n", wq_id);
        return -1;
    }

    if (WQ_UNINIT == g_wq_array[wq_id].status || g_wq_array[wq_id].group_id < 0) {
        WQ_ERR("The id:%d work_queue don't init! \n", wq_id);
        pthread_mutex_unlock(&g_wq_array[wq_id].operate_lock);
        return -1;
    }

    cnt    = 0;
    p_task = g_wq_array[wq_id].task_head;
    while (1) {
        if (NULL == p_task) {
            break;
        }
        cnt++;
        p_task = p_task->next_task;
    }
    *task_number = cnt;
    pthread_mutex_unlock(&g_wq_array[wq_id].operate_lock);

    WQ_INFO("The wq_id_%d  have [%d] task\n", wq_id, cnt);

    return 0;
}


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
             task_func handler, void *param, void *data, int *task_id)
{
    int ret = 0, t_id = 0;
    P_WORK_TASK_S p_task = NULL, p_task_tmp = NULL;

    if (wq_id >= MAX_WORK_QUEUE_NUM) {
        WQ_ERR("Input wq_id error! wq_id:%d  MAX_ID:%d\n", wq_id, MAX_WORK_QUEUE_NUM);
        return -1;
    }

    if (mode >= TASK_MODE_BOTTON) {
        WQ_ERR("Input mode:%d error!\n", mode);
        return -1;
    }

    if (NULL == handler) {
        WQ_ERR("Input handler is NULL! wq_id:%d\n", wq_id);
        return -1;
    }

    if(pthread_mutex_lock(&g_wq_array[wq_id].operate_lock) != 0) {
        WQ_ERR("The wq_id:%d work_queue don't init! \n", wq_id);
        return -1;
    }

    if (WQ_UNINIT == g_wq_array[wq_id].status || -1 == g_wq_array[wq_id].group_id) {
        WQ_ERR("The wq_id:%d work_queue don't init! \n", wq_id);
        goto TASK_ADD_EXIT;
    }

    p_task = malloc(sizeof(WORK_TASK_S));
    if (NULL == p_task) {
        WQ_ERR("Do wq_id:%d malloc WORK_TASK_S struct fail!  error:%s\n", wq_id, strerror(errno));
        goto TASK_ADD_EXIT;
    }
    memset(p_task, 0, sizeof(WORK_TASK_S));

    ret = get_valid_task_id(wq_id, &t_id);
    if (ret < 0) {
        WQ_ERR("Do get_valid_task_id faill! wq_id:%d  ret:%d\n", wq_id, ret);
        goto TASK_ADD_EXIT;
    }

    if (NULL != task_name) {
        strncpy(p_task->task_name, task_name, sizeof(p_task->task_name) - 2);
    }
    p_task->task_id     = t_id;
    p_task->work_mode   = mode;
    p_task->loop_time   = loop_time;
    p_task->handler     = handler;
    p_task->param       = param;
    p_task->data        = data;
    p_task->run_flag    = TASK_RUNNING;
    p_task->status      = TASK_IDLE;
    p_task->prev_task   = NULL;
    p_task->next_task   = NULL;

    if (NULL == g_wq_array[wq_id].task_head) {
        g_wq_array[wq_id].task_head = p_task;
    } else {
        p_task_tmp = g_wq_array[wq_id].task_head;
        while(1) {
            if(NULL != p_task_tmp->next_task) {
                p_task_tmp = p_task_tmp->next_task;
            } else {
                p_task->prev_task     = p_task_tmp;
                p_task_tmp->next_task = p_task;
                break;
            }
        }
    }

    switch (mode) {
        case TASK_MODE_LOOP:
        case TASK_MODE_ONCE:
        default:
            p_task->next_start_time = 0;
            break;

        case TASK_MODE_ONCE_DELAY:
            p_task->next_start_time = loop_time;
            break;
    }

    if (NULL != task_id) {
        *task_id = p_task->task_id;
    }

    pthread_mutex_unlock(&g_wq_array[wq_id].operate_lock);
    return p_task->task_id;

TASK_ADD_EXIT:
    pthread_mutex_unlock(&g_wq_array[wq_id].operate_lock);
    return -1;
}


int delete_task(int wq_id, int task_id)
{
    int ret = 0, success = 0;
    P_WORK_TASK_S p_task = NULL, p_task_tmp = NULL;

    if (wq_id >= MAX_WORK_QUEUE_NUM) {
        WQ_ERR("Input wq_id error! wq_id:%d  MAX_ID:%d\n", wq_id, MAX_WORK_QUEUE_NUM);
        return -1;
    }

    if (pthread_mutex_lock(&g_wq_array[wq_id].operate_lock) != 0) {
        WQ_ERR("The wq_id:%d work_queue don't init! \n", wq_id);
        return -1;
    }

    if (WQ_UNINIT == g_wq_array[wq_id].status || -1 == g_wq_array[wq_id].group_id) {
        WQ_ERR("The wq_id:%d work_queue don't init! \n", wq_id);
        goto TASK_DEL_EXIT;
    }

    success = 0;
    p_task = g_wq_array[wq_id].task_head;
    while (1) {
        if (NULL == p_task) {
            goto TASK_DEL_EXIT;
        }

        if (task_id == p_task->task_id) {
            if (NULL == p_task->prev_task && NULL == p_task->next_task) {
                g_wq_array[wq_id].task_head = NULL;
            } else if (NULL == p_task->prev_task) {
                /* The p_task is Head */
                g_wq_array[wq_id].task_head = p_task->next_task;
                p_task->next_task->prev_task = NULL;
            } else if (NULL == p_task->next_task) {
                /* The p_task is tail */
                p_task->prev_task->next_task = NULL;
            }else {
                p_task->prev_task->next_task = p_task->next_task;
                p_task->next_task->prev_task = p_task->prev_task;
            }
            break;
        }
        p_task = p_task->next_task;
    }
    p_task->run_flag = TASK_STOP;
    free(p_task);
    p_task = NULL;

    pthread_mutex_unlock(&g_wq_array[wq_id].operate_lock);
    return 0;

TASK_DEL_EXIT:
    pthread_mutex_unlock(&g_wq_array[wq_id].operate_lock);
    return -1;
}


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
int display_work_queue_info(int wq_id, int display_times)
{
    int ret = 0;
    P_WORK_TASK_S p_task = NULL, p_task_tmp = NULL;

    if (wq_id >= MAX_WORK_QUEUE_NUM) {
        WQ_ERR("Input wq_id error! wq_id:%d  MAX_ID:%d\n", wq_id, MAX_WORK_QUEUE_NUM);
        return -1;
    }

    if(pthread_mutex_lock(&g_wq_array[wq_id].operate_lock) != 0) {
        WQ_ERR("The id:%d work_queue don't init! \n", wq_id);
        return -1;
    }

    if (WQ_UNINIT == g_wq_array[wq_id].status || -1 == g_wq_array[wq_id].group_id) {
        WQ_ERR("The id:%d work_queue don't init! \n", wq_id);
        pthread_mutex_unlock(&g_wq_array[wq_id].operate_lock);
        return -1;
    }

    printf("\n*************************************************\n");
    printf("Work queue ID[%d] info\n", wq_id);
    pthread_mutex_lock(&g_operate_lock);
    p_task = g_wq_array[wq_id].task_head;
    while (1) {

        if(NULL == p_task) {
            break;
        }
        printf("task ID_%d\n", p_task->task_id);
        printf("\t\t name:      %s\n",   p_task->task_name);
        printf("\t\t work_mode: %d\n",   p_task->work_mode);
        printf("\t\t loop_time: %ld\n",  p_task->loop_time);
        printf("\t\t next_time: %ld\n",   p_task->next_start_time);
        printf("\t\t run_time:  %ld\n",   p_task->run_time);
        printf("\t\t max_time:  %ld\n",   p_task->max_run_time);
        printf("\t\t average_time:%ld\n", p_task->average_run_time);
        printf("\t\t run_number:%ld\n",   p_task->run_number);
        printf("\t\t run_flag:  %d\n",   p_task->run_flag);
        printf("\t\t status:    %d\n",   p_task->status);

        p_task = p_task->next_task;
    }
    pthread_mutex_unlock(&g_operate_lock);
    printf("*************************************************\n\n");

    pthread_mutex_unlock(&g_wq_array[wq_id].operate_lock);

    return 0;
}

