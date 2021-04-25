#include "ErrorExit.h"
#include "mj_string.h"

/// <summary>
/// We do not recurse into this as it is an exit function.
/// </summary>
/// <param name="dw"></param>
/// <param name="fileName"></param>
/// <param name="lineNumber"></param>
/// <param name="expression"></param>
void mj::ErrorExit(DWORD dw, const StringView& fileName, int lineNumber, const StringView& expression)
{
  // We specify FORMAT_MESSAGE_ALLOCATE_BUFFER so we need to LocalFree the returned string.
  LPWSTR lpMsgBuf = {};
  static_cast<void>(::FormatMessageW(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, dw,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&lpMsgBuf), 0, nullptr));

  if (lpMsgBuf)
  {
    // Display the error message and exit the process
    StringView msgString;
    msgString.Init(lpMsgBuf);

    // Calculate display string size
    size_t displayStringLength = fileName.len     //
                                 + expression.len //
                                 + msgString.len  //
                                 + 50;            // Format string length and decimals

    mj::HeapAllocator alloc;
    Allocation allocation = alloc.Allocation(displayStringLength * sizeof(wchar_t));
    if (allocation.Ok())
    {
      MJ_DEFER(alloc.Free(allocation.pAddress));
      mj::StaticStringBuilder sb;
      sb.Init(allocation);

      // The string is formatted such that you can double-click the output from
      // OutputDebugString in Visual Studio's Output window to go to the error source
      auto string = sb.Append(fileName)                   //
                        .Append(L"(")                     //
                        .Append(lineNumber)               //
                        .Append(L"): ")                   //
                        .Append(expression)               //
                        .Append(L" failed with error 0x") //
                        .AppendHex32(dw)                  //
                        .Append(L": ")                    //
                        .Append(msgString)                //
                        .ToStringClosed();

      if (::IsDebuggerPresent())
      {
        ::OutputDebugStringW(string.ptr);
      }
      else
      {
        MessageBoxW(nullptr, string.ptr, L"Error", MB_ICONERROR);
      }
    }

    static_cast<void>(::LocalFree(lpMsgBuf));
  }
}

/// <summary>
/// We do not recurse into this as it is an exit function.
/// </summary>
/// <param name="fileName"></param>
/// <param name="lineNumber"></param>
/// <param name="expression"></param>
void mj::NullExit(const StringView& fileName, int lineNumber, const StringView& expression)
{
  // Display the error message and exit the process

  // Calculate display string size
  size_t displayStringLength = fileName.len     //
                               + expression.len //
                               + 50;            // Format string length and decimals

  // LPTSTR lpDisplayBuf = static_cast<LPTSTR>(::LocalAlloc(LMEM_ZEROINIT, displayStringLength * sizeof(wchar_t)));
  mj::HeapAllocator alloc;
  mj::Allocation allocation = alloc.Allocation(displayStringLength * sizeof(wchar_t));
  if (allocation.Ok())
  {
    MJ_DEFER(alloc.Free(allocation.pAddress));
    mj::StaticStringBuilder sb;
    sb.Init(allocation);

    // The string is formatted such that you can double-click the output from
    // OutputDebugString in Visual Studio's Output window to go to the error source
    auto string = sb.Append(fileName)                   //
                      .Append(L"(")                     //
                      .Append(lineNumber)               //
                      .Append(L"): Pointer was null: ") //
                      .Append(expression)               //
                      .ToStringClosed();

    if (::IsDebuggerPresent())
    {
      ::OutputDebugStringW(string.ptr);
    }
    else
    {
      MessageBoxW(nullptr, string.ptr, L"Error", MB_ICONERROR);
    }
  }
}
