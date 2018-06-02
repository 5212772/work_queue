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
    unsigned int        run_number;        /* This task total run numbers   */
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
    
    prctl(PR_SET_NAME, "work_queue_manager", 0, 0, 0);

    while (g_manage_run_flag) {

        WQ_INFO("  ============= guixing === \n"); sleep(3);
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

    WQ_INFO("  ======2222 proccess id:%d === guixing === \n", wq_id); usleep(200 *10);

    wq_id = *(int*)para;

    //TODO: check wq_id
    
    snprintf(name, 63, "WQ_proccess_ID_%d", wq_id);

    prctl(PR_SET_NAME, "work_queue_proccess", 0, 0, 0);

    while (1) {
        /* This part is run control function */
        usleep(g_wq_array[wq_id].unit_time * 1000);  /* 50ms */
        if (WQ_EXIT == g_wq_array[wq_id].run_flag) {
            WQ_INFO(" The ID:%d work_queue proccess exit! \n", wq_id);
            break;
        } else if (WQ_STOP == g_wq_array[wq_id].run_flag) {
            continue;
        }

        switch (g_wq_array[wq_id].run_flag)
        {
            case WQ_EXIT:
                break;

            default:
                break;
        }


        WQ_INFO("  ======ID:%d proccess === guixing === \n", wq_id); sleep(2);
    }

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

        pthread_mutex_init(&g_wq_array[idx].operate_lock, NULL);
        g_wq_array[idx].status = WQ_UNINIT;
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
        g_wq_array[idx].status = WQ_UNINIT;
    }

    WQ_INFO("All of work queue haved destroy. so, exit work queue and destroy manager thread.");
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
    pthread_mutex_lock(&g_operate_lock);

    ret = check_wq_id(wq_id);
    if (ret) {
        pthread_mutex_unlock(&g_operate_lock);
        WQ_ERR("Input wq_id error! wq_id:%d  MAX_ID:%d\n", wq_id, MAX_WORK_QUEUE_NUM);
        return -1;
    }

    //TODO: lock wq_mux

    g_wq_array[wq_id].run_flag = WQ_EXIT;

    #if 1
    pthread_join(g_wq_array[wq_id].thread_id, NULL);
    #else
    ret = sem_wait(&g_wq_array[wq_id].sem);
    if (ret) {
        WQ_ERR("Do sem_wait fail! errno:%d %s \n", errno, perror(errno));
    }
    #endif

    /* TODO: release task */
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

    //TODO: unLock wq_mux

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

    g_wq_array[wq_id].run_flag = WQ_RUNNING;

    pthread_mutex_unlock(&g_wq_array[wq_id].operate_lock);
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

    g_wq_array[wq_id].run_flag = WQ_STOP;

    pthread_mutex_unlock(&g_wq_array[wq_id].operate_lock);
    return 0;
}

