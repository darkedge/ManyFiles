#include <Windows.h>
#include "ErrorExit.h"
#include "ServiceProvider.h"
#include "ServiceLocator.h"

static constexpr const wchar_t* serializationCpp = L"..\\..\\src\\Serialization.cpp";

static void CreateSerializationSourceCode()
{
  mj::AllocatorBase* pAlloc = svc::GeneralPurposeAllocator();

  mj::StringBuilder sb;
  sb.Init(pAlloc);
  MJ_DEFER(sb.Destroy());

  // Content
  sb.Append(L"// Hello World!");

  mj::StringView sv = sb.ToStringOpen();
  MJ_UNINITIALIZED DWORD bytesWritten;

  // Convert to ANSI
  MJ_UNINITIALIZED int numBytes;
  MJ_ERR_ZERO(numBytes = ::WideCharToMultiByte(CP_ACP, 0, sv.ptr, sv.len, nullptr, 0, nullptr, nullptr));
  LPSTR ptr = static_cast<LPSTR>(pAlloc->Allocate(numBytes));
  MJ_DEFER(pAlloc->Free(ptr));
  static_cast<void>(::WideCharToMultiByte(CP_ACP, 0, sv.ptr, sv.len, ptr, numBytes, nullptr, nullptr));

  // Write file contents
  MJ_UNINITIALIZED HANDLE file;
  MJ_ERR_IF(
      file = ::CreateFileW(serializationCpp, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr),
      INVALID_HANDLE_VALUE);
  MJ_DEFER(MJ_ERR_ZERO(::CloseHandle(file)));
  MJ_ERR_ZERO(::WriteFile(file, ptr, numBytes, &bytesWritten, nullptr));
}

/// <summary>
/// Default entry CRT-less point
/// </summary>
void CALLBACK WinMainCRTStartup()
{
  HANDLE hStdout          = ::GetStdHandle(STD_OUTPUT_HANDLE);
  const char* lpszPrompt1 = "Hello World!";
  MJ_UNINITIALIZED DWORD cWritten;
  static_cast<void>(::WriteFile(hStdout, lpszPrompt1, lstrlenA(lpszPrompt1), &cWritten, nullptr));

  mj::HeapAllocator generalPurposeAllocator;
  MJ_UNINITIALIZED mj::AllocatorBase* pAllocator;
  pAllocator = &generalPurposeAllocator;
  svc::ProvideGeneralPurposeAllocator(pAllocator);

  ::CreateSerializationSourceCode();

  // Running without CRT requires a manual call to ExitProcess
  ::ExitProcess(0);
}
