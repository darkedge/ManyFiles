#pragma once
#include "ErrorExit.h"

namespace mj
{
  // A cache line for work object context.
#pragma warning(push)
#pragma warning(disable : 4324) // structure was padded due to alignment specifier (Yes, we know. That's the point.)
  struct alignas(64) TaskContext
  {
    /// <summary>
    /// (Internal) Pointer to next available TaskContext node
    /// </summary>
    TaskContext* pNextFreeNode;
  };
#pragma warning(pop)

  class Task;

  class ITaskCompletionHandler
  {
  public:
    virtual void OnTaskCompletion(Task* pTask) = 0;
  };

  struct Task
  {
    /// <summary>
    /// Called from a threadpool thread.
    /// </summary>
    virtual void Execute() = 0;

    /// <summary>
    /// Called in the main thread when the task is done.
    /// </summary>
    virtual void OnDone()
    {
      // Optional
    }

    ITaskCompletionHandler* pHandler;
  };

  namespace detail
  {
    TaskContext* ThreadpoolAllocTaskContext();
  }

  /// <summary>
  /// Initializes the threadpool system.
  /// </summary>
  /// <param name="hWnd">The window to send a message to once a task has finished</param>
  /// <param name="msg">The message ID to send. Should be WM_USER + some number.</param>
  void ThreadpoolInit(HWND hWnd, UINT msg);

  template <class T>
  T* ThreadpoolCreateTask(ITaskCompletionHandler* pHandler = nullptr)
  {
    static_assert(sizeof(T) <= sizeof(TaskContext));
    static_assert(std::is_base_of<Task, T>::value);

    TaskContext* pContext = detail::ThreadpoolAllocTaskContext();
    T* pTask              = nullptr;

    if (pContext)
    {
      pTask           = new (pContext) T;
      pTask->pHandler = pHandler;
    }

    return pTask;
  }

  void ThreadpoolTaskEnd(WPARAM wParam);
  void ThreadpoolSubmitTask(Task* pTask);
  void ThreadpoolDestroy();
} // namespace mj
