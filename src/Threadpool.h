#pragma once
#include "ErrorExit.h"

namespace mj
{
  // A cache line for work object context.
#pragma warning(push)
#pragma warning(disable : 4324) // structure was padded due to alignment specifier (Yes, we know. That's the point.)
  struct alignas(64) TaskContext
  {
  };
#pragma warning(pop)

  struct Task;

  template <class T>
  using TaskEndFn = void (*)(T* pContext);
  template <class T>
  using CTaskEndFn = void (*)(const T* pContext);

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
    TaskEndFn<mj::TaskContext> pCallback;

    /// <summary>
    /// Optional function to call on the main thread after this task finishes
    /// </summary>
    CTaskEndFn<mj::TaskContext> pMainThreadCallback;

    void Submit();
    void Wait();
  };

  namespace detail
  {
    Task* ThreadpoolTaskAlloc(TaskEndFn<mj::TaskContext> pInitContext, TaskEndFn<mj::TaskContext> pCallback,
                              CTaskEndFn<mj::TaskContext> pMainThreadCallback = nullptr);
  }

  void ThreadpoolInit(HWND hWnd, UINT msg);
  template <class T>
  Task* ThreadpoolTaskAlloc(TaskEndFn<T> pInitContext, TaskEndFn<T> pCallback,
                            CTaskEndFn<T> pMainThreadCallback = nullptr)
  {
    // Note: We are casting the function pointer to prevent having to cast a TaskContext
    // to a user-defined type on the user side.
    // This _should_ work on all systems, as long as the parameter types are "compatible".
    return detail::ThreadpoolTaskAlloc(reinterpret_cast<TaskEndFn<mj::TaskContext>>(pInitContext),
                                       reinterpret_cast<TaskEndFn<mj::TaskContext>>(pCallback),
                                       reinterpret_cast<CTaskEndFn<mj::TaskContext>>(pMainThreadCallback));
  }

  void ThreadpoolTaskEnd(WPARAM wParam, LPARAM lParam);
  void ThreadpoolTaskFree(Task* pTask);
  void ThreadpoolDestroy();

} // namespace mj
