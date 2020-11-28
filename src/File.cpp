#include "File.h"
#include "mj_common.h"
#include "ErrorExit.h"
#include "Threadpool.h"
#include "ConvertToHex.h"
#include "minicrt.h"

struct LoadFileContext
{
  // In
  HANDLE hFile;

  // Out
  wchar_t* pText;
};

static void LoadFileCallback(mj::TaskContext* pTaskContext)
{
  LoadFileContext* pContext = reinterpret_cast<LoadFileContext*>(pTaskContext);

  MJ_UNINITIALIZED LARGE_INTEGER fileSize;
  MJ_ERR_ZERO(::GetFileSizeEx(pContext->hFile, &fileSize));

  char* pMemory = static_cast<char*>(mj::Win32Alloc(fileSize.QuadPart));
  if (pMemory)
  {
    // Note MJ: ReadFile does not accept 64 bit integers for read sizes.
    // We are currently passing a LONGLONG obtained from GetFileSizeEx.
    MJ_UNINITIALIZED DWORD dwBytesRead;
    MJ_ERR_ZERO(::ReadFile(pContext->hFile, pMemory, static_cast<DWORD>(fileSize.QuadPart), &dwBytesRead, nullptr));

    pContext->pText = mj::AllocConvertToHex(pMemory, dwBytesRead);

    // TODO MJ: Keep the binary loaded as well in case we want to edit it later on
    mj::Win32Free(pMemory);
  }

  // Handle can be freed before returning to main thread.
  MJ_ERR_ZERO(::CloseHandle(pContext->hFile));
}

static void LoadFileDone(const mj::TaskContext* pTaskContext)
{
  // TODO: Render if possible
  const LoadFileContext* pContext = reinterpret_cast<const LoadFileContext*>(pTaskContext);

  mj::FreeConvertToHex(pContext->pText);
}

void mj::LoadFileAsync(const wchar_t* pFilePath)
{
  HANDLE hFile = ::CreateFileW(pFilePath,             // file to open
                               GENERIC_READ,          // open for reading
                               FILE_SHARE_READ,       // share for reading
                               nullptr,               // default security
                               OPEN_EXISTING,         // existing file only
                               FILE_ATTRIBUTE_NORMAL, // normal file
                               nullptr);              // no attr. template

  if (hFile != INVALID_HANDLE_VALUE)
  {
    mj::Task* pTask           = mj::ThreadpoolTaskAlloc(LoadFileCallback, LoadFileDone);
    LoadFileContext* pContext = reinterpret_cast<LoadFileContext*>(pTask->pContext);
    pContext->hFile           = hFile;

    pTask->Submit();
  }
}
