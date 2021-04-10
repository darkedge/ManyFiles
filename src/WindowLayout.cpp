#include "WindowLayout.h"
#include "Control.h"
#include "ErrorExit.h"
#include "ServiceLocator.h"

#include "DirectoryNavigationPanel.h"
#include "HorizontalLayout.h"
#include "VerticalLayout.h"
#include "TabLayout.h"

static constexpr const wchar_t* s_WindowLayoutFilename = L"window_layout.txt";

struct ETokenType
{
  enum Enum
  {
    Identifier,
    Dot,
    OpenBrace,
    CloseBrace,
    OpenBracket,
    CloseBracket,
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

static void PrintTokenType(ETokenType::Enum parserState)
{
  ::OutputDebugStringW(L"Token type: ");
  switch (parserState)
  {
  case ETokenType::Identifier:
    ::OutputDebugStringW(L"Identifier\r\n");
    break;
  case ETokenType::Dot:
    ::OutputDebugStringW(L"Dot\r\n");
    break;
  case ETokenType::OpenBrace:
    ::OutputDebugStringW(L"OpenBrace\r\n");
    break;
  case ETokenType::CloseBrace:
    ::OutputDebugStringW(L"CloseBrace\r\n");
    break;
  case ETokenType::OpenBracket:
    ::OutputDebugStringW(L"OpenBracket\r\n");
    break;
  case ETokenType::CloseBracket:
    ::OutputDebugStringW(L"CloseBracket\r\n");
    break;
  case ETokenType::Number:
    ::OutputDebugStringW(L"Number\r\n");
    break;
  default:
    break;
  }
};

struct EParserState
{
  enum Enum
  {
    ExpectType,        // Identifier
    AfterDot,          // Identifier
    AfterType,         // OpenBrace
    AfterOpenBrace,    // Dot
    AfterCloseBrace,   // Identifier or CloseBracket
    AfterOpenBracket,  // Identifier
    AfterCloseBracket, // CloseBrace or Dot
    AfterField,        // Number or OpenBracket
    AfterValue,        // CloseBrace or Dot
    Error,
  };
};

struct EControlType
{
  enum Enum
  {
    DirectoryNavigationPanel,
    HorizontalLayout,
    VerticalLayout,
    TabLayout,
  };
};

static void PrintParserState(EParserState::Enum parserState)
{
  ::OutputDebugStringW(L"Parser state: ");
  switch (parserState)
  {
  case EParserState::ExpectType:
    ::OutputDebugStringW(L"ExpectType\r\n");
    break;
  case EParserState::AfterDot:
    ::OutputDebugStringW(L"AfterDot\r\n");
    break;
  case EParserState::AfterType:
    ::OutputDebugStringW(L"AfterType\r\n");
    break;
  case EParserState::AfterOpenBrace:
    ::OutputDebugStringW(L"AfterOpenBrace\r\n");
    break;
  case EParserState::AfterCloseBrace:
    ::OutputDebugStringW(L"AfterCloseBrace\r\n");
    break;
  case EParserState::AfterOpenBracket:
    ::OutputDebugStringW(L"AfterOpenBracket\r\n");
    break;
  case EParserState::AfterCloseBracket:
    ::OutputDebugStringW(L"AfterCloseBracket\r\n");
    break;
  case EParserState::AfterField:
    ::OutputDebugStringW(L"AfterField\r\n");
    break;
  case EParserState::AfterValue:
    ::OutputDebugStringW(L"AfterValue\r\n");
    break;
  case EParserState::Error:
    ::OutputDebugStringW(L"Error\r\n");
    break;
  default:
    break;
  }
};

struct Token
{
  ETokenType::Enum tokenType;
  mj::StringView sv;
  union {
    uint32_t number;
  };
};

struct ECharacterClass
{
  enum Enum
  {
    Alpha,
    Digit,
  };
};

static wchar_t ClassifyCharacter(wchar_t c)
{
  if (c < L'0')
  {
    return c;
  }
  else if (c < L':')
  {
    return ECharacterClass::Digit;
  }
  else if (c < L'A')
  {
    return c;
  }
  else if (c < L'[')
  {
    return ECharacterClass::Alpha;
  }
  else if (c < L'a')
  {
    return c;
  }
  else if (c < L'{')
  {
    return ECharacterClass::Alpha;
  }

  return c;
};

struct LexerContext
{
  wchar_t* pLexemeStart;
  wchar_t* pLexemeEnd;
  const wchar_t* pEndOfFile;
};

static bool GetNextToken(LexerContext* pContext, Token* pToken)
{
  ELexerState::Enum lexerState = ELexerState::AcceptAny;
  while (lexerState != ELexerState::Done)
  {
    if (pContext->pLexemeEnd >= pContext->pEndOfFile)
    {
      ::OutputDebugStringW(L"== End of file ==\r\n");
      return false;
    }

    wchar_t cc = ClassifyCharacter(*pContext->pLexemeEnd);
    pContext->pLexemeEnd++;
    switch (lexerState)
    {
    case ELexerState::AcceptAny:
      switch (cc)
      {
      case ECharacterClass::Alpha:
        lexerState = ELexerState::AfterAlpha;
        break;
      case ECharacterClass::Digit:
        lexerState = ELexerState::AfterDigit;
        break;
      case L'.':
        pToken->tokenType = ETokenType::Dot;
        lexerState        = ELexerState::Done;
        break;
      case L'{':
        pToken->tokenType = ETokenType::OpenBrace;
        lexerState        = ELexerState::Done;
        break;
      case L'}':
        pToken->tokenType = ETokenType::CloseBrace;
        lexerState        = ELexerState::Done;
        break;
      case L'[':
        pToken->tokenType = ETokenType::OpenBracket;
        lexerState        = ELexerState::Done;
        break;
      case L']':
        pToken->tokenType = ETokenType::CloseBracket;
        lexerState        = ELexerState::Done;
        break;
      default:
        // Skip first characters until they are known
        pContext->pLexemeStart++;
        break;
      }
      break;
    case ELexerState::AfterAlpha:
      if (cc != ECharacterClass::Alpha)
      {
        pContext->pLexemeEnd--;
        pToken->tokenType = ETokenType::Identifier;
        lexerState        = ELexerState::Done;
      }
      break;
    case ELexerState::AfterDigit:
      if (cc != ECharacterClass::Digit)
      {
        pContext->pLexemeEnd--;
        pToken->tokenType = ETokenType::Number;
        lexerState        = ELexerState::Done;
      }
      break;
    default:
      break;
    }
  }

  pToken->sv.Init(pContext->pLexemeStart, pContext->pLexemeEnd - pContext->pLexemeStart);

  switch (pToken->tokenType)
  {
  case ETokenType::Number:
    // This should always succeed
    static_cast<void>(pToken->sv.ParseNumber(&pToken->number));
    break;
  default:
    break;
  }

  pContext->pLexemeStart = pContext->pLexemeEnd;

  return true;
};

void mj::LoadWindowLayout(mj::AllocatorBase* pAllocator)
{
  // Read file into memory
  MJ_UNINITIALIZED HANDLE file;
  MJ_ERR_IF(file = ::CreateFileW(s_WindowLayoutFilename, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                 nullptr),
            INVALID_HANDLE_VALUE);
  MJ_DEFER(MJ_ERR_ZERO(::CloseHandle(file)));

  MJ_UNINITIALIZED DWORD fileSize;
  MJ_ERR_IF(fileSize = ::GetFileSize(file, nullptr), INVALID_FILE_SIZE);

  mj::Allocation allocation = pAllocator->Allocation(fileSize);
  MJ_DEFER(pAllocator->Free(allocation.pAddress));

  MJ_UNINITIALIZED LexerContext lexerContext;

  lexerContext.pLexemeStart = reinterpret_cast<wchar_t*>(allocation.pAddress);
  lexerContext.pLexemeEnd   = lexerContext.pLexemeStart;
  lexerContext.pEndOfFile   = lexerContext.pLexemeStart + (allocation.numBytes / sizeof(wchar_t));

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
      while (::GetNextToken(&lexerContext, &token))
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

#if 0
            // TODO: We should generate this bit
            Control* pControl = nullptr;
            if (token.sv.Equals(L"DirectoryNavigationPanel"))
            {
              auto* pControl = pAllocator->New<DirectoryNavigationPanel>();
            }
            else if (token.sv.Equals(L"HorizontalLayout"))
            {
              auto* pControl = pAllocator->New<HorizontalLayout>();
            }
            else if (token.sv.Equals(L"VerticalLayout"))
            {
              auto* pControl = pAllocator->New<VerticalLayout>();
            }
            if (pControl)
            {
              pControl->Init()
            }
#endif
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
            // Begin type description
            parserState = EParserState::AfterOpenBrace;
          }
          else
          {
            parserState = EParserState::Error;
          }
          break;
        case EParserState::AfterOpenBrace:
          if (token.tokenType == ETokenType::Dot)
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
          else if (token.tokenType == ETokenType::CloseBracket)
          {
            // Close collection
            parserState = EParserState::AfterCloseBracket;
          }
          else
          {
            parserState = EParserState::Error;
          }
          break;
        case EParserState::AfterOpenBracket:
          if (token.tokenType == ETokenType::Identifier)
          {
            parserState = EParserState::AfterType;
          }
          else
          {
            parserState = EParserState::Error;
          }
          break;
        case EParserState::AfterCloseBracket:
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
        case EParserState::AfterField:
          if (token.tokenType == ETokenType::Number)
          {
            parserState = EParserState::AfterValue;
          }
          else if (token.tokenType == ETokenType::OpenBracket)
          {
            parserState = EParserState::AfterOpenBracket;
          }
          else
          {
            parserState = EParserState::Error;
          }
          break;
        case EParserState::AfterValue:
          if (token.tokenType == ETokenType::CloseBrace)
          {
            // Close collection
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

        ::PrintParserState(parserState);
        ::OutputDebugStringW(L"\r\n");

        if (parserState == EParserState::Error)
        {
          break;
        }
      }
    }

    sb.Append(L"\r\n");
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
