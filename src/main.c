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
/* None */


/************************************************************************************************/
/*                                    Function Declarations                                     */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                     Function Definitions                                     */
/************************************************************************************************/
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
    sleep(4);

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

    destroy_work_queue(wq_id[0]);

    WQ_INFO(" bbbbbbbbbbbbbbbbbbbbbbbbbb! ret:%d\n", ret);

    sleep(2);

    get_all_work_queue_id(id_array, &tmp);

    
    start_work_queue(wq_id[0]);
    
    start_work_queue(wq_id[1]);
    destroy_work_queue(wq_id[1]);
    stop_work_queue(wq_id[1]);

    get_all_work_queue_id(id_array, &tmp);
    get_all_task_number(wq_id[0], &task_number);
    get_all_task_number(wq_id[1], &task_number);

    WQ_INFO(" The end !\n");

    return 0;
}

