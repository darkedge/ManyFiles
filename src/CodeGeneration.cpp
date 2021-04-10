#include <Windows.h>
#include "ErrorExit.h"
#include "ServiceProvider.h"
#include "ServiceLocator.h"

static constexpr const wchar_t* serializationCpp = L"..\\..\\src\\Serialization.cpp";

static void CreateSerializationSourceCode()
{
  MJ_UNINITIALIZED HANDLE file;
  MJ_ERR_IF(
      file = ::CreateFileW(serializationCpp, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr),
      INVALID_HANDLE_VALUE);
  MJ_DEFER(MJ_ERR_ZERO(::CloseHandle(file)));

  mj::ArrayList<wchar_t> al;
  al.Init(svc::GeneralPurposeAllocator());
  MJ_DEFER(al.Destroy());
  mj::StringBuilder sb;
  sb.SetArrayList(&al);

  sb.Append(L"0");

  mj::StringView sv = sb.ToStringOpen();
  MJ_UNINITIALIZED DWORD bytesWritten;

  // Write file contents
  MJ_ERR_ZERO(::WriteFile(file, sv.ptr, sv.len * sizeof(wchar_t), &bytesWritten, nullptr));
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
  generalPurposeAllocator.Init();
  MJ_UNINITIALIZED mj::AllocatorBase* pAllocator;
  pAllocator = &generalPurposeAllocator;
  svc::ProvideGeneralPurposeAllocator(pAllocator);

  ::CreateSerializationSourceCode();

  // Running without CRT requires a manual call to ExitProcess
  ::ExitProcess(0);
}
