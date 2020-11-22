#include "Threadpool.h"
#include "mj_win32.h"
#include "mj_common.h"
#include "ErrorExit.h"

static constexpr auto MAX_TASKS = 4096;
static mj::TaskContext s_TaskContextArray[MAX_TASKS];
static mj::TaskContextNode s_TaskContextFreeList[MAX_TASKS];
static mj::TaskContextNode* s_pTaskContextNodeHead;

static uint32_t s_NumTasksAtomic;

static TP_CALLBACK_ENVIRON s_CallbackEnvironment;
static TP_POOL* s_pPool;
static TP_CLEANUP_GROUP* s_pCleanupGroup;
static CRITICAL_SECTION s_CriticalSection;

namespace mj
{
  static TaskContextNode* TaskContextAlloc()
  {
    MJ_EXIT_NULL(s_pTaskContextNodeHead);

    TaskContextNode* pContext = s_pTaskContextNodeHead;

    ::EnterCriticalSection(&s_CriticalSection);
    s_pTaskContextNodeHead = s_pTaskContextNodeHead->pNextFreeNode;
    ::LeaveCriticalSection(&s_CriticalSection);

    return pContext;
  }

  static void TaskContextFree(TaskContextNode* pNode)
  {
    ::EnterCriticalSection(&s_CriticalSection);
    pNode->pNextFreeNode   = s_pTaskContextNodeHead;
    s_pTaskContextNodeHead = pNode;
    ::LeaveCriticalSection(&s_CriticalSection);
  }
} // namespace mj

void mj::ThreadpoolInit()
{
  ::InitializeThreadpoolEnvironment(&s_CallbackEnvironment);

  MJ_ERR_NULL(s_pPool = ::CreateThreadpool(nullptr));

  ::SetThreadpoolThreadMaximum(s_pPool, 8);
  MJ_ERR_ZERO(::SetThreadpoolThreadMinimum(s_pPool, 8));

  MJ_ERR_NULL(s_pCleanupGroup = CreateThreadpoolCleanupGroup());

  ::SetThreadpoolCallbackPool(&s_CallbackEnvironment, s_pPool);
  ::SetThreadpoolCallbackCleanupGroup(&s_CallbackEnvironment, s_pCleanupGroup, nullptr);

  ::InitializeCriticalSection(&s_CriticalSection);

  // Initialize free list
  TaskContextNode* pNext = nullptr;
  for (int i = 0; i < MAX_TASKS; i++)
  {
    s_TaskContextFreeList[i].Init(pNext, &s_TaskContextArray[i]);
    pNext = &s_TaskContextFreeList[i];
  }

  s_pTaskContextNodeHead = &s_TaskContextFreeList[MAX_TASKS - 1];
}

mj::Task mj::ThreadpoolTaskAlloc(PTP_WORK_CALLBACK pCallback)
{
  MJ_UNINITIALIZED mj::Task task;
  mj::TaskContextNode* pNext = mj::TaskContextAlloc();
  task.pNode = pNext;
  MJ_ERR_NULL(task.pWork = ::CreateThreadpoolWork(pCallback, pNext->pContext, &s_CallbackEnvironment));
  return task;
}

void mj::ThreadpoolTaskFree(Task task)
{
  mj::TaskContextFree(task.pNode);
}

void mj::ThreadpoolDestroy()
{
  ::CloseThreadpoolCleanupGroupMembers(s_pCleanupGroup, false, nullptr);
  ::CloseThreadpoolCleanupGroup(s_pCleanupGroup);
  ::CloseThreadpool(s_pPool);
  ::DestroyThreadpoolEnvironment(&s_CallbackEnvironment);
  ::DeleteCriticalSection(&s_CriticalSection);
}

void mj::Task::Submit()
{
  ::SubmitThreadpoolWork(this->pWork);
}

void mj::Task::Wait()
{
  ::WaitForThreadpoolWorkCallbacks(this->pWork, false);
  ::CloseThreadpoolWork(pWork);
}
