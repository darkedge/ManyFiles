#include "Threadpool.h"
#include "mj_win32.h"
#include "mj_common.h"
#include "ErrorExit.h"
#include "ServiceLocator.h"
#include "../3rdparty/tracy/Tracy.hpp"

static constexpr auto MAX_TASKS   = 4096;
static constexpr auto NUM_THREADS = 8;
static mj::TaskContext s_TaskContextArray[MAX_TASKS];
static mj::TaskContext* s_pTaskHead;
static HWND s_Hwnd;
static UINT s_Msg;
static HANDLE s_Threads[NUM_THREADS];
static HANDLE s_Iocp;

/// <summary>
/// The return value of this function can be cast to anything you want
/// (as long as its size is less or equal)
/// </summary>
mj::TaskContext* mj::detail::ThreadpoolAllocTaskContext()
{
  MJ_EXIT_NULL(s_pTaskHead);

  mj::TaskContext* pTaskContext = s_pTaskHead;

  s_pTaskHead = s_pTaskHead->pNextFreeNode;

  return pTaskContext;
}

namespace mj
{
  static void ThreadpoolFreeContext(TaskContext* pContext)
  {
    // Write a new TaskContext over this piece of memory
    TaskContext* pNode   = new (pContext) TaskContext;
    pNode->pNextFreeNode = s_pTaskHead;
    s_pTaskHead          = pNode;
  }
} // namespace mj

static DWORD WINAPI ThreadMain(LPVOID lpThreadParameter)
{
  static_cast<void>(lpThreadParameter);

  while (true)
  {
    MJ_UNINITIALIZED DWORD numBytes;
    MJ_UNINITIALIZED mj::Task* pTask;
    MJ_UNINITIALIZED LPOVERLAPPED pOverlapped;

    {
      ZoneScopedNC("Sleeping", 0x21231C);
      BOOL ret =
          ::GetQueuedCompletionStatus(s_Iocp, &numBytes, reinterpret_cast<PULONG_PTR>(&pTask), &pOverlapped, INFINITE);
      if (ret == FALSE && ::GetLastError() == ERROR_ABANDONED_WAIT_0)
      {
        break;
      }
    }

    if (pTask)
    {
      pTask->Execute();

      {
        ZoneScopedNC("PostMessageW", 0x31332C);
        MJ_ERR_ZERO(::PostMessageW(s_Hwnd, s_Msg, reinterpret_cast<WPARAM>(pTask), 0));
      }
    }
  }

  return 0;
}

void mj::ThreadpoolInit(UINT msg)
{
  ZoneScoped;

  s_Msg  = msg;
  MJ_ERR_IF(s_Iocp = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0), nullptr);

  // Initialize free list
  mj::TaskContext* pNext = nullptr;
  for (int i = 0; i < MAX_TASKS; i++)
  {
    s_TaskContextArray[i].pNextFreeNode = pNext;
    pNext                               = &s_TaskContextArray[i];
  }
  s_pTaskHead = &s_TaskContextArray[MAX_TASKS - 1];

  for (int i = 0; i < NUM_THREADS; i++)
  {
    ZoneScopedN("CreateThread");
    MJ_ERR_IF(s_Threads[i] = ::CreateThread(nullptr,    // default security attributes
                                            0,          // default stack size
                                            ThreadMain, // entry point
                                            nullptr,    // argument
                                            0,          // default flags
                                            nullptr),
              nullptr);
  }
}

void mj::ThreadpoolSetWindowHandle(HWND hWnd)
{
  s_Hwnd = hWnd;
}

void mj::ThreadpoolTaskEnd(WPARAM wParam)
{
  mj::Task* pTask = reinterpret_cast<mj::Task*>(wParam);
  pTask->OnDone();
  mj::ThreadpoolFreeContext(reinterpret_cast<mj::TaskContext*>(pTask));
}

void mj::ThreadpoolDestroy()
{
  ::CloseHandle(s_Iocp);
  s_Iocp = nullptr;
}

void mj::ThreadpoolSubmitTask(mj::Task* pTask)
{
  ::PostQueuedCompletionStatus(s_Iocp, 0, reinterpret_cast<ULONG_PTR>(pTask), nullptr);
}
