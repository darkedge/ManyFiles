#include "Threadpool.h"
#include "mj_win32.h"
#include "mj_common.h"
#include "ErrorExit.h"
#include "FloatMagic.h"
#include "minicrt.h"

static constexpr auto MAX_TASKS = 4096;
static mj::TaskContext s_TaskContextArray[MAX_TASKS];
static mj::Task s_TaskFreeList[MAX_TASKS];
static mj::Task* s_pTaskHead;

static uint32_t s_NumTasksAtomic;

static TP_CALLBACK_ENVIRON s_CallbackEnvironment;
static TP_POOL* s_pPool;
static TP_CLEANUP_GROUP* s_pCleanupGroup;
static CRITICAL_SECTION s_CriticalSection;

namespace mj
{
  static Task* TaskAlloc()
  {
    MJ_EXIT_NULL(s_pTaskHead);

    Task* pTask = s_pTaskHead;

    ::EnterCriticalSection(&s_CriticalSection);
    s_pTaskHead = s_pTaskHead->pNextFreeNode;
    ::LeaveCriticalSection(&s_CriticalSection);

    return pTask;
  }

  static void TaskFree(Task* pNode)
  {
    ::EnterCriticalSection(&s_CriticalSection);
    pNode->pNextFreeNode = s_pTaskHead;
    s_pTaskHead          = pNode;
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
  Task* pNext = nullptr;
  for (int i = 0; i < MAX_TASKS; i++)
  {
    s_TaskFreeList[i].pNextFreeNode = pNext;
    s_TaskFreeList[i].pContext      = &s_TaskContextArray[i];

    pNext = &s_TaskFreeList[i];
  }

  s_pTaskHead = &s_TaskFreeList[MAX_TASKS - 1];
}

static void TaskMain(TP_CALLBACK_INSTANCE* pInstance, void* pContext, TP_WORK* pWork)
{
  static_cast<void>(pInstance);
  static_cast<void>(pWork);

  mj::Task* pTask = static_cast<mj::Task*>(pContext);

  pTask->pCallback(pTask->pContext);

  if (pTask->pMainThreadCallback)
  {
    MJ_ERR_ZERO(::PostMessageW(mj::GetMainWindowHandle(), MJ_TASKEND, reinterpret_cast<WPARAM>(pTask),
                               reinterpret_cast<LPARAM>(pTask->pMainThreadCallback)));
  }
};

mj::Task* mj::ThreadpoolTaskAlloc(TaskEndFn pCallback, CTaskEndFn pMainThreadCallback)
{
  mj::Task* pTask            = mj::TaskAlloc();
  pTask->pCallback           = pCallback;
  pTask->pMainThreadCallback = pMainThreadCallback;
  MJ_ERR_NULL(pTask->pWork = ::CreateThreadpoolWork(TaskMain, pTask, &s_CallbackEnvironment));
  return pTask;
}

void mj::ThreadpoolTaskFree(Task* pTask)
{
  ::CloseThreadpoolWork(pTask->pWork);
  pTask->pWork = nullptr;
  mj::TaskFree(pTask);
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
}
