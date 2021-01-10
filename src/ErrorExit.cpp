#include "ErrorExit.h"
#include "StringBuilder.h"

/// <summary>
/// We do not recurse into this as it is an exit function.
/// </summary>
/// <param name="dw"></param>
/// <param name="fileName"></param>
/// <param name="lineNumber"></param>
/// <param name="expression"></param>
void mj::ErrorExit(DWORD dw, const String& fileName, int lineNumber, const String& expression)
{
  // We specify FORMAT_MESSAGE_ALLOCATE_BUFFER so we need to LocalFree the returned string.
  LPWSTR lpMsgBuf = {};
  static_cast<void>(::FormatMessageW(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, dw,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&lpMsgBuf), 0, nullptr));

  if (lpMsgBuf)
  {
    // Display the error message and exit the process
    String msgString(lpMsgBuf);

    // Calculate display string size
    size_t displayStringLength = fileName.len     //
                                 + expression.len //
                                 + msgString.len  //
                                 + 50;            // Format string length and decimals

    mj::VirtualAllocator alloc;
    alloc.Init(0);
    Allocation allocation = alloc.Allocation(displayStringLength * sizeof(wchar_t));

    if (allocation.Ok())
    {
      MJ_DEFER(alloc.Free(allocation.pAddress));
      LinearAllocator sbAlloc;
      sbAlloc.Init(allocation);

      ArrayList<wchar_t> arrayList;
      arrayList.Init(&sbAlloc, allocation.numBytes / sizeof(wchar_t));

      StringBuilder sb;
      sb.SetArrayList(&arrayList);

      auto string = sb.Append(fileName)                   //
                        .Append(L":")                     //
                        .Append(lineNumber)               //
                        .Append(L" - ")                   //
                        .Append(expression)               //
                        .Append(L" failed with error 0x") //
                        .AppendHex32(dw)                  //
                        .Append(L": ")                    //
                        .Append(msgString)                //
                        .ToString();

      //static_cast<void>(::MessageBoxW(nullptr, string.ptr, L"Error", MB_OK));
      ::DebugBreak();
    }

    static_cast<void>(::LocalFree(lpMsgBuf));
  }

  ::ExitProcess(dw);
}

/// <summary>
/// We do not recurse into this as it is an exit function.
/// </summary>
/// <param name="fileName"></param>
/// <param name="lineNumber"></param>
/// <param name="expression"></param>
void mj::NullExit(const String& fileName, int lineNumber, const String& expression)
{
  // Display the error message and exit the process

  // Calculate display string size
  size_t displayStringLength = fileName.len     //
                               + expression.len //
                               + 50;            // Format string length and decimals

  // LPTSTR lpDisplayBuf = static_cast<LPTSTR>(::LocalAlloc(LMEM_ZEROINIT, displayStringLength * sizeof(wchar_t)));
  mj::VirtualAllocator alloc;
  alloc.Init(0);
  Allocation allocation = alloc.Allocation(displayStringLength * sizeof(wchar_t));
  if (allocation.Ok())
  {
    MJ_DEFER(alloc.Free(allocation.pAddress));
    LinearAllocator sbAlloc;
    sbAlloc.Init(allocation);

    ArrayList<wchar_t> arrayList;
    arrayList.Init(&sbAlloc, allocation.numBytes / sizeof(wchar_t));

    StringBuilder sb;
    sb.SetArrayList(&arrayList);

    auto string = sb.Append(fileName)                   //
                      .Append(L":")                     //
                      .Append(lineNumber)               //
                      .Append(L" - Pointer was null: ") //
                      .Append(expression)               //
                      .ToString();

    //static_cast<void>(::MessageBoxW(nullptr, string.ptr, L"Error", MB_OK));
    ::DebugBreak();
  }

  ::ExitProcess(EXCEPTION_ACCESS_VIOLATION);
}
