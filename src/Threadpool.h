#pragma once
#include "ErrorExit.h"

namespace mj
{
  // A cache line for work object context.
#pragma warning(push)
#pragma warning(disable : 4324) // structure was padded due to alignment specifier (Yes, we know. That's the point.)
  struct alignas(256) TaskContext
  {
    /// <summary>
    /// (Internal) Pointer to next available TaskContext node
    /// </summary>
    TaskContext* pNextFreeNode;
  };
#pragma warning(pop)

  struct Task;

  class ITaskCompletionHandler
  {
  public:
    virtual void OnTaskCompletion(Task* pTask) = 0;
  };

  /// <summary>
  /// There is no Init (not needed, we can do it in Execute).
  /// </summary>
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

    /// <summary>
    /// Override when a task manages its own memory.
    /// </summary>
    virtual void Destroy()
    {
      // Optional
    }

    ITaskCompletionHandler* pHandler;
    bool cancelled;
  };

  namespace detail
  {
    TaskContext* ThreadpoolAllocTaskContext();
  }

  /// <summary>
  /// Initializes the threadpool system.
  /// </summary>
  /// <param name="msg">The message ID to send. Should be WM_USER + some number.</param>
  void ThreadpoolInit(UINT msg);

  template <class T>
  T* ThreadpoolCreateTask(ITaskCompletionHandler* pHandler = nullptr)
  {
    static_assert(sizeof(T) <= sizeof(TaskContext));
    static_assert(std::is_base_of<Task, T>::value);

    TaskContext* pContext = detail::ThreadpoolAllocTaskContext();
    T* pTask              = nullptr;

    if (pContext)
    {
      pTask            = new (pContext) T;
      pTask->pHandler  = pHandler;
      pTask->cancelled = false;
    }

    return pTask;
  }

  /// <summary>
  /// Do not start any tasks before calling this function,
  /// otherwise task completion will fail.
  /// </summary>
  void ThreadpoolSetWindowHandle(HWND hWnd);

  void ThreadpoolTaskEnd(WPARAM wParam);
  void ThreadpoolSubmitTask(Task* pTask);
  void ThreadpoolDestroy();
} // namespace mj
