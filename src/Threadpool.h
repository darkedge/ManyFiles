#pragma once
#include "ErrorExit.h"

namespace mj
{
  struct TaskContext;

  // A cache line for work object context.
  struct alignas(64) TaskContext
  {
    char padding[64];
  };

  struct TaskContextNode
  {
    TaskContextNode* pNextFreeNode;
    mj::TaskContext* pContext;

    void Init(TaskContextNode* pNextFreeNode, mj::TaskContext* pContext)
    {
      this->pNextFreeNode = pNextFreeNode;
      this->pContext      = pContext;
    }
  };

  struct Task
  {
    TP_WORK* pWork;
    TaskContextNode* pNode;

    void Submit();
    void Wait();

    template <typename T>
    T* Context()
    {
      MJ_EXIT_NULL(pNode);
      return reinterpret_cast<T*>(pNode->pContext);
    }
  };

  void ThreadpoolInit();
  Task ThreadpoolTaskAlloc(PTP_WORK_CALLBACK pCallback);
  void ThreadpoolTaskFree(Task task);
  void ThreadpoolDestroy();

} // namespace mj
