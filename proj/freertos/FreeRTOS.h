#ifndef FREE_RTOS_H
#define FREE_RTOS_H

#include <windows.h>
#include <stdint.h>

#define pdTRUE 1
#define configTICK_RATE_HZ 1000
#define configSTACK_DEPTH_TYPE    uint16_t
#define portMAX_DELAY ( TickType_t ) 0xffffffffUL

#define xTimerStart( xTimer, xTicksToWait )

typedef void* TimerHandle_t;
typedef UINT TickType_t;
typedef long BaseType_t;
typedef unsigned long UBaseType_t;
typedef void(*TimerCallbackFunction_t)();

typedef void (*TaskFunction_t)(void*);

typedef struct tskTaskControlBlock* TaskHandle_t;

struct xQueueHandle
{
    HANDLE hEvent;
    CRITICAL_SECTION section;
    BYTE* pData;
    UBaseType_t uQueueLength;
    UBaseType_t uItemSize;
    volatile int count;
    //int nStartIndex;
    //int nEndIndex;
};

typedef struct xQueueHandle* QueueHandle_t;

TimerHandle_t xTimerCreate(const char* const pcTimerName, 
    const TickType_t xTimerPeriodInTicks,
    const BaseType_t xAutoReload,
    void* const pvTimerID,
    TimerCallbackFunction_t pxCallbackFunction);

QueueHandle_t xQueueCreate(UBaseType_t uxQueueLength, UBaseType_t uxItemSize);
BaseType_t xQueueSend(QueueHandle_t xQueue, const void* pvItemToQueue, TickType_t xTicksToWait);
BaseType_t xQueueReceive(QueueHandle_t xQueue, void* pvBuffer, TickType_t xTicksToWait);


BaseType_t xTaskCreate(TaskFunction_t pxTaskCode,
    const char* const pcName,
    const configSTACK_DEPTH_TYPE usStackDepth,
    void* const pvParameters,
    UBaseType_t uxPriority,
    TaskHandle_t* const pxCreatedTask);

#endif // FREE_RTOS_H