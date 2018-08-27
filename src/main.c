/************************************************************************************************/
/* Copyright (C), 2016-2019                                                                     */
/************************************************************************************************/
/**
 * @file main.c
 * @brief : this main function file. For do work queue!
 * @author id: wangguixing
 * @version v0.1 create
 * @date 2018-04-14
 */

/************************************************************************************************/
/*                                      Include Files                                           */
/************************************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "work_queue.h"
#include "main.h"

/************************************************************************************************/
/*                                     Macros & Typedefs                                        */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                    Structure Declarations                                    */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                      Global Variables                                        */
/************************************************************************************************/
int task_test_id = 0;


/************************************************************************************************/
/*                                    Function Declarations                                     */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                     Function Definitions                                     */
/************************************************************************************************/
int task_handle_func(void *param, void *data)
{
    int ret = 0;

    WQ_INFO(" The task_test_ID: %d  \n", task_test_id);
    return 0;
}


int main(int argc, char *agrv[])
{
    int ret = 0, cnt = 0, tmp = 0, task_number = 0;
    int wq_id[MAX_WORK_QUEUE_NUM] = {0};
    int id_array[MAX_WORK_QUEUE_NUM] = {0};

    WQ_INFO("Do test work queue! ret:%d\n", ret);

    create_work_queue(500, &wq_id[0]);

    sleep(1);

    create_work_queue(500, &wq_id[1]);

    WQ_INFO(" aaaaaaaaaaaaaaaaaaaaaaaaaa! ret:%d\n", ret);
    sleep(2);

    stop_work_queue(wq_id[1]);
    sleep(3);

    stop_work_queue(wq_id[0]);
    sleep(3);

    get_all_work_queue_id(id_array, &tmp);

    start_work_queue(wq_id[0]);
    sleep(3);

    start_work_queue(wq_id[1]);
    sleep(3);

    get_all_task_number(wq_id[0], &task_number);
    get_all_task_number(wq_id[1], &task_number);

    WQ_INFO(" bbbbbbbbbbbbbbbbbbbbbbbbbb! ret:%d\n", ret);

    int task_id[64] = {0};
    add_task(wq_id[0], "wq_0 task_0", TASK_MODE_ONCE_DELAY, 300, task_handle_func, NULL, NULL, &task_id[0]);
    add_task(wq_id[0], "wq_0 task_1", TASK_MODE_LOOP, 220, task_handle_func, NULL, NULL, &task_id[1]);
    add_task(wq_id[0], "wq_0 task_2", TASK_MODE_LOOP, 260, task_handle_func, NULL, NULL, &task_id[2]);
    add_task(wq_id[0], "wq_0 task_3", TASK_MODE_LOOP, 260, task_handle_func, NULL, NULL, &task_id[3]);
    add_task(wq_id[0], "wq_0 task_4", TASK_MODE_LOOP, 260, task_handle_func, NULL, NULL, &task_id[4]);
    add_task(wq_id[0], "wq_0 task_5", TASK_MODE_LOOP, 260, task_handle_func, NULL, NULL, &task_id[5]);


    add_task(wq_id[1], "wq_1 task_0", TASK_MODE_ONCE_DELAY, 230, task_handle_func, NULL, NULL, &task_id[6]);
    add_task(wq_id[1], "wq_1 task_1", TASK_MODE_LOOP, 340, task_handle_func, NULL, NULL, &task_id[7]);
    add_task(wq_id[1], "wq_1 task_2", TASK_MODE_LOOP, 340, task_handle_func, NULL, NULL, &task_id[8]);
    add_task(wq_id[1], "wq_1 task_3", TASK_MODE_LOOP, 340, task_handle_func, NULL, NULL, &task_id[9]);
    add_task(wq_id[1], "wq_1 task_4", TASK_MODE_LOOP, 340, task_handle_func, NULL, NULL, &task_id[10]);
    add_task(wq_id[1], "wq_1 task_5", TASK_MODE_LOOP, 340, task_handle_func, NULL, NULL, &task_id[11]);
    add_task(wq_id[1], "wq_1 task_6", TASK_MODE_LOOP, 340, task_handle_func, NULL, NULL, &task_id[12]);
    add_task(wq_id[1], "wq_1 task_7", TASK_MODE_LOOP, 340, task_handle_func, NULL, NULL, &task_id[13]);

    sleep(1);

    display_work_queue_info(wq_id[0], 500);
    display_work_queue_info(wq_id[1], 500);

#if 1
    delete_task(wq_id[0], task_id[3]);
    delete_task(wq_id[0], 77);
    //delete_task(wq_id[0], task_id[2]);
    //delete_task(wq_id[0], task_id[5]);
    //delete_task(wq_id[0], task_id[1]);
    delete_task(wq_id[0], task_id[0]);
    delete_task(wq_id[0], task_id[4]);

/*
    delete_task(wq_id[0], task_id[5]);
    delete_task(wq_id[0], 77);
    delete_task(wq_id[0], task_id[4]);
    delete_task(wq_id[0], task_id[3]);
    delete_task(wq_id[0], task_id[2]);
    delete_task(wq_id[0], task_id[1]);
    delete_task(wq_id[0], task_id[0]);
    delete_task(wq_id[0], task_id[0]);
*/
    delete_task(wq_id[1], task_id[6]);
    delete_task(wq_id[1], task_id[13]);
    //delete_task(wq_id[1], task_id[7]);
    //delete_task(wq_id[1], task_id[12]);
    //delete_task(wq_id[1], 789);
    //delete_task(wq_id[1], task_id[8]);
    //delete_task(wq_id[1], task_id[11]);
    delete_task(wq_id[1], task_id[9]);
    delete_task(wq_id[1], task_id[10]);

    display_work_queue_info(wq_id[0], 500);
    display_work_queue_info(wq_id[1], 500);
#endif

    destroy_work_queue(wq_id[0]);

    sleep(2);


    display_work_queue_info(wq_id[0], 500);
    display_work_queue_info(wq_id[1], 500);

    get_all_work_queue_id(id_array, &tmp);


    start_work_queue(wq_id[0]);

    start_work_queue(wq_id[1]);
    destroy_work_queue(wq_id[1]);
    stop_work_queue(wq_id[1]);



    display_work_queue_info(wq_id[0], 500);
    display_work_queue_info(wq_id[1], 500);

    get_all_work_queue_id(id_array, &tmp);
    get_all_task_number(wq_id[0], &task_number);
    get_all_task_number(wq_id[1], &task_number);

    WQ_INFO(" The end !\n");

    return 0;
}

