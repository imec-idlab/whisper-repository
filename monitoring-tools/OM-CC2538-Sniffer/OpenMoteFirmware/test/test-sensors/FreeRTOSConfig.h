/**
 * @file       FreeRTOSConfig.h
 * @author     Pere Tuset-Peiro (peretuset@openmote.com)
 * @version    v0.1
 * @date       May, 2015
 * @brief
 *
 * @copyright  Copyright 2015, OpenMote Technologies, S.L.
 *             This file is licensed under the GNU General Public License v2.
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#define configUSE_TICKLESS_IDLE			        1
#define configCPU_CLOCK_HZ				        32000000
#define configTICK_RATE_HZ				        ( ( TickType_t ) 100 )

#define configPRE_STOP_PROCESSING(x)            board_sleep(x)
#define configPOST_STOP_PROCESSING(x)           board_wakeup(x)

#define configUSE_PREEMPTION			        1
#define configUSE_IDLE_HOOK				        0
#define configUSE_TICK_HOOK				        0
#define configMAX_PRIORITIES			        ( 5 )
#define configMINIMAL_STACK_SIZE		        ( ( unsigned short ) 64 )
#define configTOTAL_HEAP_SIZE			        ( ( size_t ) ( 4 * 1024 ) )
#define configMAX_TASK_NAME_LEN			        ( 16 )
#define configUSE_TRACE_FACILITY		        0
#define configUSE_16_BIT_TICKS			        0
#define configIDLE_SHOULD_YIELD			        1
#define configUSE_MUTEXES				        1
#define configQUEUE_REGISTRY_SIZE		        5
#define configCHECK_FOR_STACK_OVERFLOW	        0
#define configUSE_RECURSIVE_MUTEXES		        1
#define configUSE_MALLOC_FAILED_HOOK	        0
#define configUSE_APPLICATION_TASK_TAG	        0
#define configUSE_COUNTING_SEMAPHORES	        1

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES 			        0
#define configMAX_CO_ROUTINE_PRIORITIES         ( 2 )

/* Software timer definitions. */
#define configUSE_TIMERS				        0
#define configTIMER_TASK_PRIORITY		        ( configMAX_PRIORITIES - 1 )
#define configTIMER_QUEUE_LENGTH		        5
#define configTIMER_TASK_STACK_DEPTH	        ( configMINIMAL_STACK_SIZE * 2 )

/* Set the following definitions to 1 to include the API function, or zero
   to exclude the API function. */
#define INCLUDE_vTaskPrioritySet		        1
#define INCLUDE_uxTaskPriorityGet		        1
#define INCLUDE_vTaskDelete				        1
#define INCLUDE_vTaskCleanUpResources	        0
#define INCLUDE_vTaskSuspend			        1
#define INCLUDE_vTaskDelayUntil			        1
#define INCLUDE_vTaskDelay				        1

/* Cortex-M specific definitions. */
#ifdef __NVIC_PRIO_BITS
	/* __NVIC_PRIO_BITS will be specified when CMSIS is being used. */
	#define configPRIO_BITS       		        __NVIC_PRIO_BITS
#else
    /* The Texas Instruments CC2538 SoC has 8 priority levels. */
	#define configPRIO_BITS       		        3
#endif

/* The lowest interrupt priority that can be used in a call to a "set priority"
   function. */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY			0x07

/* The highest interrupt priority that can be used by any interrupt service
   routine that makes calls to interrupt safe FreeRTOS API functions. DO NOT CALL
   INTERRUPT SAFE FREERTOS API FUNCTIONS FROM ANY INTERRUPT THAT HAS A HIGHER
   PRIORITY THAN THIS! (higher priorities are lower numeric values. */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY	0x05

/* Interrupt priorities used by the kernel port layer itself. These are generic
   to all Cortex-M ports, and do not rely on any particular library functions. */
#define configKERNEL_INTERRUPT_PRIORITY 		        ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#define configTICK_LOWEST_INTERRUPT_PRIORITY            ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	        ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) ) 

#endif /* FREERTOS_CONFIG_H */
