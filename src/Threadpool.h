#pragma once
#include "ErrorExit.h"

#define MJ_TASKEND (WM_USER + 0)

namespace mj
{
  // A cache line for work object context.
  struct alignas(64) TaskContext
  {
  };

  struct Task;

  typedef void (*TaskEndFn)(mj::TaskContext* pContext);
  typedef void (*CTaskEndFn)(const mj::TaskContext* pContext);

  struct Task
  {
    /// <summary>
    /// (Internal) Pointer to next available Task node
    /// </summary>
    Task* pNextFreeNode;

    /// <summary>
    /// Pointer to work object context (cache line)
    /// </summary>
    mj::TaskContext* pContext;

    /// <summary>
    /// Win32 threadpool work object handle
    /// </summary>
    TP_WORK* pWork;

    /// <summary>
    /// Function that does the work for this task
    /// </summary>
    TaskEndFn pCallback;

    /// <summary>
    /// Optional function to call on the main thread after this task finishes
    /// </summary>
    CTaskEndFn pMainThreadCallback;

    void Submit();
    void Wait();
  };

  void ThreadpoolInit();
  Task* ThreadpoolTaskAlloc(TaskEndFn pCallback, CTaskEndFn pMainThreadCallback = nullptr);
  void ThreadpoolTaskFree(Task* pTask);
  void ThreadpoolDestroy();

} // namespace mj
