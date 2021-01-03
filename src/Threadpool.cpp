#include "Threadpool.h"
#include "mj_win32.h"
#include "mj_common.h"
#include "ErrorExit.h"
#include "ServiceLocator.h"

static constexpr auto MAX_TASKS = 4096;
static mj::TaskContext s_TaskContextArray[MAX_TASKS];
static mj::Task s_TaskFreeList[MAX_TASKS];
static mj::Task* s_pTaskHead;

static TP_CALLBACK_ENVIRON s_CallbackEnvironment;
static TP_POOL* s_pPool;
static TP_CLEANUP_GROUP* s_pCleanupGroup;
static CRITICAL_SECTION s_CriticalSection;
static HWND s_Hwnd;
static UINT s_Msg;

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

void mj::ThreadpoolInit(HWND hWnd, UINT msg)
{
  s_Hwnd = hWnd;
  s_Msg  = msg;
  ::InitializeThreadpoolEnvironment(&s_CallbackEnvironment);

  MJ_ERR_IF(s_pPool = ::CreateThreadpool(nullptr), nullptr);

  ::SetThreadpoolThreadMaximum(s_pPool, 8);
  // Setting SetThreadpoolThreadMinimum will immediately create new threads,
  // which takes multiple milliseconds per thread.
  // MJ_ERR_ZERO(::SetThreadpoolThreadMinimum(s_pPool, 8));

  MJ_ERR_IF(s_pCleanupGroup = ::CreateThreadpoolCleanupGroup(), nullptr);

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
    MJ_ERR_ZERO(::PostMessageW(s_Hwnd, s_Msg, reinterpret_cast<WPARAM>(pTask),
                               reinterpret_cast<LPARAM>(pTask->pMainThreadCallback)));
  }
};

mj::Task* mj::detail::ThreadpoolTaskAlloc(TaskEndFn<mj::TaskContext> pInitContext, TaskEndFn<mj::TaskContext> pCallback,
                                          CTaskEndFn<mj::TaskContext> pMainThreadCallback)
{
  mj::Task* pTask = mj::TaskAlloc();
  if (pInitContext)
  {
    pInitContext(pTask->pContext);
  }
  pTask->pCallback           = pCallback;
  pTask->pMainThreadCallback = pMainThreadCallback;
  MJ_ERR_IF(pTask->pWork = ::CreateThreadpoolWork(TaskMain, pTask, &s_CallbackEnvironment), nullptr);
  return pTask;
}

void mj::ThreadpoolTaskEnd(WPARAM wParam, LPARAM lParam)
{
  mj::Task* pTask   = reinterpret_cast<mj::Task*>(wParam);
    mj::CTaskEndFn<mj::TaskContext> pFn = reinterpret_cast<mj::CTaskEndFn<mj::TaskContext>>(lParam);
    if (pFn)
    {
    pFn(pTask->pContext);
    }

    mj::ThreadpoolTaskFree(pTask);
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
