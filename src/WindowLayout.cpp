#include "WindowLayout.h"
#include "Control.h"
#include "ErrorExit.h"
#include "ServiceLocator.h"

static constexpr const wchar_t* s_WindowLayoutFilename = L"window_layout.txt";

void mj::LoadWindowLayout()
{
  // Read file into memory
  MJ_UNINITIALIZED HANDLE file;
  MJ_ERR_IF(file = ::CreateFileW(s_WindowLayoutFilename, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                 nullptr),
            INVALID_HANDLE_VALUE);
  MJ_DEFER(MJ_ERR_ZERO(::CloseHandle(file)));

  MJ_UNINITIALIZED DWORD fileSize;
  MJ_ERR_IF(fileSize = ::GetFileSize(file, nullptr), INVALID_FILE_SIZE);

  mj::AllocatorBase* pAllocator = svc::GeneralPurposeAllocator();
  MJ_EXIT_NULL(pAllocator);
  mj::Allocation allocation = pAllocator->Allocation(fileSize);
  MJ_DEFER(pAllocator->Free(allocation.pAddress));

  struct ETokenType
  {
    enum Enum
    {
      Identifier,
      Dot,
      OpenBrace,
      CloseBrace,
      Equals,
      Number
    };
  };

  struct ELexerState
  {
    enum Enum
    {
      AcceptAny,
      AfterAlpha,
      AfterDigit,
      Done
    };
  };

  auto PrintTokenType = [](ETokenType::Enum parserState) {
    ::OutputDebugStringW(L"Token type: ");
    switch (parserState)
    {
    case ETokenType::Identifier:
      ::OutputDebugStringW(L"Identifier\n");
      break;
    case ETokenType::Dot:
      ::OutputDebugStringW(L"Dot\n");
      break;
    case ETokenType::OpenBrace:
      ::OutputDebugStringW(L"OpenBrace\n");
      break;
    case ETokenType::CloseBrace:
      ::OutputDebugStringW(L"CloseBrace\n");
      break;
    case ETokenType::Equals:
      ::OutputDebugStringW(L"Equals\n");
      break;
    case ETokenType::Number:
      ::OutputDebugStringW(L"Number\n");
      break;
    default:
      break;
    }
  };

  struct EParserState
  {
    enum Enum
    {
      ExpectType,      // Identifier
      AfterDot,        // Identifier
      AfterType,       // OpenBrace
      AfterOpenBrace,  // Identifier or Dot
      AfterCloseBrace, // Identifier or Dot
      AfterField,      // Equals
      AfterEquals,     // OpenBrace or Number
      AfterValue,      // CloseBrace or Dot
      Error,
    };
  };

  auto PrintParserState = [](EParserState::Enum parserState) {
    ::OutputDebugStringW(L"Parser state: ");
    switch (parserState)
    {
    case EParserState::ExpectType:
      ::OutputDebugStringW(L"ExpectType\n");
      break;
    case EParserState::AfterDot:
      ::OutputDebugStringW(L"AfterDot\n");
      break;
    case EParserState::AfterType:
      ::OutputDebugStringW(L"AfterType\n");
      break;
    case EParserState::AfterOpenBrace:
      ::OutputDebugStringW(L"AfterOpenBrace\n");
      break;
    case EParserState::AfterCloseBrace:
      ::OutputDebugStringW(L"AfterCloseBrace\n");
      break;
    case EParserState::AfterField:
      ::OutputDebugStringW(L"AfterField\n");
      break;
    case EParserState::AfterEquals:
      ::OutputDebugStringW(L"AfterEquals\n");
      break;
    case EParserState::AfterValue:
      ::OutputDebugStringW(L"AfterValue\n");
      break;
    case EParserState::Error:
      ::OutputDebugStringW(L"Error\n");
      break;
    default:
      break;
    }
  };

  struct Token
  {
    ETokenType::Enum tokenType;
    StringView sv;
  };

  wchar_t* pLexemeStart     = reinterpret_cast<wchar_t*>(allocation.pAddress);
  wchar_t* pLexemeEnd       = reinterpret_cast<wchar_t*>(allocation.pAddress);
  const wchar_t* pEndOfFile = pLexemeStart + (allocation.numBytes / sizeof(wchar_t));

  struct ECharacterClass
  {
    enum Enum
    {
      Unknown,
      Dot,
      Alpha,
      Digit,
      Equals,
      OpenBrace,
      CloseBrace,
    };
  };

  auto ClassifyCharacter = [&](wchar_t c) {
    if (c == 0x2E)
    {
      return ECharacterClass::Dot;
    }
    else if (c < 0x30)
    {
      return ECharacterClass::Unknown;
    }
    else if (c < 0x3A)
    {
      return ECharacterClass::Digit;
    }
    else if (c == 0x3D)
    {
      return ECharacterClass::Equals;
    }
    else if (c < 0x41)
    {
      return ECharacterClass::Unknown;
    }
    else if (c < 0x5B)
    {
      return ECharacterClass::Alpha;
    }
    else if (c < 0x61)
    {
      return ECharacterClass::Unknown;
    }
    else if (c < 0x7B)
    {
      return ECharacterClass::Alpha;
    }
    else if (c == 0x7B)
    {
      return ECharacterClass::OpenBrace;
    }
    else if (c == 0x7D)
    {
      return ECharacterClass::CloseBrace;
    }

    return ECharacterClass::Unknown;
  };

  auto GetNextToken = [&](Token* pToken) {
    ELexerState::Enum lexerState = ELexerState::AcceptAny;
    while (lexerState != ELexerState::Done)
    {
      if (pLexemeEnd >= pEndOfFile)
      {
        ::OutputDebugStringW(L"== End of file ==\n");
        return false;
      }

      ECharacterClass::Enum cc = ClassifyCharacter(*pLexemeEnd);
      pLexemeEnd++;
      switch (lexerState)
      {
      case ELexerState::AcceptAny:
        switch (cc)
        {
        case ECharacterClass::Dot:
          pToken->tokenType = ETokenType::Dot;
          lexerState = ELexerState::Done;
          break;
        case ECharacterClass::Alpha:
          lexerState = ELexerState::AfterAlpha;
          break;
        case ECharacterClass::Digit:
          lexerState = ELexerState::AfterDigit;
          break;
        case ECharacterClass::Equals:
          pToken->tokenType = ETokenType::Equals;
          lexerState        = ELexerState::Done;
          break;
        case ECharacterClass::OpenBrace:
          pToken->tokenType = ETokenType::OpenBrace;
          lexerState        = ELexerState::Done;
          break;
        case ECharacterClass::CloseBrace:
          pToken->tokenType = ETokenType::CloseBrace;
          lexerState        = ELexerState::Done;
          break;
        default:
          // Skip first characters until they are known
          pLexemeStart++;
          break;
        }
        break;
      case ELexerState::AfterAlpha:
        if (cc != ECharacterClass::Alpha)
        {
          pToken->tokenType = ETokenType::Identifier;
          lexerState        = ELexerState::Done;
        }
        break;
      case ELexerState::AfterDigit:
        if (cc != ECharacterClass::Digit)
        {
          pToken->tokenType = ETokenType::Number;
          lexerState        = ELexerState::Done;
        }
        break;
      default:
        break;
      }
    }

    pToken->sv.Init(pLexemeStart, pLexemeEnd - pLexemeStart);
    pLexemeStart = pLexemeEnd;

    return true;
  };

  EParserState::Enum parserState = EParserState::ExpectType;
  if (allocation.Ok())
  {
    MJ_UNINITIALIZED DWORD numRead;
    MJ_ERR_ZERO(::ReadFile(file, allocation.pAddress, fileSize, &numRead, nullptr));

    // Debug
    StringBuilder sb;
    ArrayList<wchar_t> sbal;
    sbal.Init(svc::GeneralPurposeAllocator());
    sb.SetArrayList(&sbal);
    MJ_DEFER(sbal.Destroy());

    if (fileSize == numRead)
    {
      // Start parsing
      MJ_UNINITIALIZED Token token;
      while (GetNextToken(&token))
      {
        PrintParserState(parserState);
        PrintTokenType(token.tokenType);

        sb.Append(token.sv).Indent(1);
        switch (parserState)
        {
        case EParserState::ExpectType:
          if (token.tokenType == ETokenType::Identifier)
          {
            parserState = EParserState::AfterType;
          }
          else
          {
            parserState = EParserState::Error;
          }
          break;
        case EParserState::AfterDot:
          if (token.tokenType == ETokenType::Identifier)
          {
            parserState = EParserState::AfterField;
          }
          else
          {
            parserState = EParserState::Error;
          }
          break;
        case EParserState::AfterType:
          if (token.tokenType == ETokenType::OpenBrace)
          {
            parserState = EParserState::AfterOpenBrace;
          }
          else
          {
            parserState = EParserState::Error;
          }
          break;
        case EParserState::AfterOpenBrace:
          if (token.tokenType == ETokenType::Identifier)
          {
            parserState = EParserState::AfterType;
          }
          else if (token.tokenType == ETokenType::Dot)
          {
            parserState = EParserState::AfterDot;
          }
          else
          {
            parserState = EParserState::Error;
          }
          break;
        case EParserState::AfterCloseBrace:
          if (token.tokenType == ETokenType::Identifier)
          {
            parserState = EParserState::AfterType;
          }
          else if (token.tokenType == ETokenType::Dot)
          {
            parserState = EParserState::AfterField;
          }
          else if (token.tokenType == ETokenType::CloseBrace)
          {
            parserState = EParserState::AfterCloseBrace;
          }
          else
          {
            parserState = EParserState::Error;
          }
          break;
        case EParserState::AfterField:
          if (token.tokenType == ETokenType::Equals)
          {
            parserState = EParserState::AfterEquals;
          }
          else
          {
            parserState = EParserState::Error;
          }
          break;
        case EParserState::AfterEquals:
          if (token.tokenType == ETokenType::OpenBrace)
          {
            parserState = EParserState::AfterOpenBrace;
          }
          else if (token.tokenType == ETokenType::Number)
          {
            parserState = EParserState::AfterValue;
          }
          else
          {
            parserState = EParserState::Error;
          }
          break;
        case EParserState::AfterValue:
          if (token.tokenType == ETokenType::CloseBrace)
          {
            parserState = EParserState::AfterCloseBrace;
          }
          else if (token.tokenType == ETokenType::Dot)
          {
            parserState = EParserState::AfterDot;
          }
          else
          {
            parserState = EParserState::Error;
          }
          break;
        default:
          parserState = EParserState::Error;
          break;
        }

        PrintParserState(parserState);
        ::OutputDebugStringW(L"\n");

        if (parserState == EParserState::Error)
        {
          break;
        }
      }
    }

    sb.Append(L"\n");
    ::OutputDebugStringW(sb.ToStringClosed().ptr);
  }
}

void mj::SaveWindowLayout(mj::Control* pRootControl)
{
  // Skip window x/y width/height for now.

  MJ_UNINITIALIZED HANDLE file;
  MJ_ERR_IF(file = ::CreateFileW(s_WindowLayoutFilename, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                 FILE_ATTRIBUTE_NORMAL, nullptr),
            INVALID_HANDLE_VALUE);
  MJ_DEFER(MJ_ERR_ZERO(::CloseHandle(file)));

  ArrayList<wchar_t> al;
  al.Init(svc::GeneralPurposeAllocator());
  MJ_DEFER(al.Destroy());
  StringBuilder sb;
  sb.SetArrayList(&al);

  pRootControl->SaveToString(sb, 0);

  StringView sv = sb.ToStringOpen();
  MJ_UNINITIALIZED DWORD bytesWritten;

  // Write the UTF-16 LE BOM here, until we know what's best to do here.
  uint16_t bom = 0xFEFF;
  MJ_ERR_ZERO(::WriteFile(file, &bom, sizeof(bom), &bytesWritten, nullptr));
  MJ_ERR_ZERO(::WriteFile(file, sv.ptr, sv.len * sizeof(wchar_t), &bytesWritten, nullptr));
}
