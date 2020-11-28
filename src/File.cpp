#include "File.h"
#include "mj_common.h"
#include "ErrorExit.h"
#include "Threadpool.h"
#include "HexEdit.h"
#include "minicrt.h"

struct LoadFileContext
{
  // In
  HANDLE hFile;

  // Out
  mj::BinaryBlob blob;
};

static void LoadFileCallback(mj::TaskContext* pTaskContext)
{
  LoadFileContext* pContext = reinterpret_cast<LoadFileContext*>(pTaskContext);

  MJ_UNINITIALIZED LARGE_INTEGER fileSize;
  MJ_ERR_ZERO(::GetFileSizeEx(pContext->hFile, &fileSize));

  // Make sure we return null if allocation fails
  pContext->blob.pBinary = nullptr;

  char* pMemory = static_cast<char*>(mj::Win32Alloc(fileSize.QuadPart));
  if (pMemory)
  {
    // Note MJ: ReadFile does not accept 64-bit integers for read sizes.
    // We are currently passing a LONGLONG obtained from GetFileSizeEx.
    // We currently cannot read 64-bit size files!
    MJ_UNINITIALIZED DWORD dwBytesRead;
    MJ_ERR_ZERO(::ReadFile(pContext->hFile, pMemory, static_cast<DWORD>(fileSize.QuadPart), &dwBytesRead, nullptr));

    pContext->blob.pBinary = pMemory;
    pContext->blob.length  = dwBytesRead;
  }

  // Handle can be freed before returning to main thread.
  MJ_ERR_ZERO(::CloseHandle(pContext->hFile));
}

/// <summary>
/// File loading complete, send binary blob over to HexEdit for further processing.
/// </summary>
/// <param name="pTaskContext"></param>
static void LoadFileDone(const mj::TaskContext* pTaskContext)
{
  const LoadFileContext* pContext = reinterpret_cast<const LoadFileContext*>(pTaskContext);

  // If file loading failed, do nothing.
  if (pContext->blob.pBinary)
  {
    mj::HexEditSetBinary(pContext->blob);
  }
}

void mj::LoadFileAsync(const wchar_t* pFilePath)
{
  MJ_EXIT_NULL(pFilePath);

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
