#pragma once
#include "ErrorExit.h"

#define MJ_TASKEND (WM_USER + 0)

namespace mj
{
  // A cache line for work object context.
  struct alignas(64) TaskContext
  {
    char padding[64];
  };

  struct Task
  {
    Task* pNextFreeNode;
    mj::TaskContext* pContext;
    TP_WORK* pWork;

    void Init(Task* pNextFreeNode, mj::TaskContext* pContext)
    {
      this->pNextFreeNode = pNextFreeNode;
      this->pContext      = pContext;
    }

    void Submit();
    void Wait();
  };

  typedef void (*TaskEndFn)(mj::Task* pTask);

  void ThreadpoolInit();
  Task* ThreadpoolTaskAlloc(PTP_WORK_CALLBACK pCallback);
  void ThreadpoolTaskFree(Task* pTask);
  void ThreadpoolDestroy();

} // namespace mj
