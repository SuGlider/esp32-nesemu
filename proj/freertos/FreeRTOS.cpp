#include "FreeRTOS.h"

struct TIMER
{
    ULONG elapse;
    TimerCallbackFunction_t timer;
};

DWORD __stdcall TimerProc(TIMER* pTimer)
{
    while(1)
    {
        Sleep(pTimer->elapse);
        pTimer->timer();
    }
}

TimerHandle_t xTimerCreate(const char* const pcTimerName,
    const TickType_t xTimerPeriodInTicks,
    const BaseType_t xAutoReload,
    void* const pvTimerID,
    TimerCallbackFunction_t pxCallbackFunction)
{
    TIMER* pTimer = new TIMER();
    pTimer->elapse = xTimerPeriodInTicks;
    pTimer->timer = pxCallbackFunction;
    HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)TimerProc, pTimer, 0, NULL);
    if (hThread)
    {
        CloseHandle(hThread);
    }
    return (TimerHandle_t)pTimer;
    //return SetTimer(0, 1, xTimerPeriodInTicks, pxCallbackFunction);
}

QueueHandle_t xQueueCreate(UBaseType_t uxQueueLength, UBaseType_t uxItemSize)
{
    xQueueHandle* pHandle = new xQueueHandle;
    memset(&pHandle->section, 0, sizeof(CRITICAL_SECTION));
    InitializeCriticalSection(&pHandle->section);
    pHandle->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    pHandle->pData = new BYTE[uxQueueLength * uxItemSize];
    pHandle->uQueueLength = uxQueueLength;
    pHandle->uItemSize = uxItemSize;
    pHandle->count = 0;
    //q.nEndIndex = 0;
    return pHandle;
}

BaseType_t xQueueSend(QueueHandle_t xQueue, const void* pvItemToQueue, TickType_t xTicksToWait)
{
    EnterCriticalSection(&xQueue->section);
    if (xQueue->count < xQueue->uQueueLength)
    {
        memcpy((void*)((INT_PTR)xQueue->pData + xQueue->count * xQueue->uItemSize), pvItemToQueue, xQueue->uItemSize);
        xQueue->count++;
    }
    LeaveCriticalSection(&xQueue->section);
    SetEvent(xQueue->hEvent);
    return xQueue->count;
}

BaseType_t xQueueReceive(QueueHandle_t xQueue, void* pvBuffer, TickType_t xTicksToWait)
{
    while (1)
    {
        WaitForSingleObject(xQueue->hEvent, INFINITE);
        EnterCriticalSection(&xQueue->section);
        if (xQueue->count > 0)
        {
            memcpy(pvBuffer, xQueue->pData, xQueue->uItemSize);
            xQueue->count--;
            if (xQueue->count > 0)
            {
                memmove(xQueue->pData, (void*)((INT_PTR)xQueue->pData + xQueue->uItemSize), xQueue->count * xQueue->uItemSize);
            }

            
            LeaveCriticalSection(&xQueue->section);
            return xQueue->count;
        }
        LeaveCriticalSection(&xQueue->section);
    }
}

BaseType_t xTaskCreate(TaskFunction_t pxTaskCode, const char* const pcName, const configSTACK_DEPTH_TYPE usStackDepth, void* const pvParameters, UBaseType_t uxPriority, TaskHandle_t* const pxCreatedTask)
{
    HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)pxTaskCode, pvParameters, 0, NULL);
    return CloseHandle(hThread);
}
