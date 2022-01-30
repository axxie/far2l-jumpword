#include <KeyFileHelper.h>
#include <farplug-wide.h>
#include <utils.h>

#include "JumpWord.hpp"
#include "JumpWordLng.hpp"

const TCHAR *GetMsg(int MsgId) {
  return (Info.GetMsg(Info.ModuleNumber, MsgId));
}

SHAREDSYMBOL int WINAPI EXP_NAME(GetMinFarVersion)() {
  return FARMANAGERVERSION;
}

SHAREDSYMBOL void WINAPI
EXP_NAME(SetStartupInfo)(const struct PluginStartupInfo *Info) {
  ::Info     = *Info;
  ::FSF      = *Info->FSF;
  ::Info.FSF = &::FSF;
}

bool isIdChar(const wchar_t c) {
  return c == L'_' || FSF.LIsAlphanum(c);
}

bool FindCurrentWord(
    int startPosition,
    const wchar_t **resLineBegin,
    const wchar_t **resLineEnd,
    const wchar_t **resBegin,
    const wchar_t **resEnd) {

  EditorGetString edGetString = {};

  edGetString.StringNumber = -1;
  if (!Info.EditorControl(ECTL_GETSTRING, &edGetString)) return false;

  const wchar_t *line = edGetString.StringText;
  const int length    = edGetString.StringLength;

  if (startPosition >= length) return false;

  int i = startPosition;
  while (i >= 0 && isIdChar(line[i])) i--;

  int j = startPosition;
  while (j < length && isIdChar(line[j])) j++;

  if (j <= i + 1) return false;

  *resLineBegin = line;
  *resLineEnd   = line + length;
  *resBegin     = line + (i + 1);
  *resEnd       = line + j;
  return true;
}

bool FindNextWord(
    const wchar_t *begin,
    const wchar_t *end,
    const wchar_t *wordBegin,
    const wchar_t *wordEnd,
    const wchar_t **result) {
  // Find the next word in the line. Two usage scenarios are possible:
  // 1. search in the lines that are below the line containing original word
  // 2. search within the line contained original word, but after the word
  // itself
  //
  // Based on the supported search scenarios, begin is guaranteed to either
  // point to the start of the line or to point to the element that is
  // immediately after the element delimiting the word being searched.
  //
  // Because of that we can safely assume that we can compare character
  // immediately.
  bool isCheckingWord          = true;
  const wchar_t *wordCurrent   = wordBegin;
  const wchar_t *foundLocation = begin;

  while (begin < end) {
    if (isCheckingWord && *begin == *wordCurrent) {
      wordCurrent++;
      if (wordCurrent == wordEnd) {
        *result = foundLocation;
        return true;
      }
    } else {
      if (isIdChar(*begin)) {
        isCheckingWord = false;
      } else {
        isCheckingWord = true;
        wordCurrent    = wordBegin;
        // the word potentially starts after current character
        foundLocation = begin + 1;
      }
    }
    begin++;
  }
  return false;
}

bool FindPreviousWord(
    const wchar_t *begin,
    const wchar_t *end,
    const wchar_t *wordBegin,
    const wchar_t *wordEnd,
    const wchar_t **result) {

  bool isCheckingWord        = true;
  const wchar_t *wordCurrent = wordEnd - 1;

  while (begin <= end) {
    if (isCheckingWord && *end == *wordCurrent) {
      if (wordCurrent == wordBegin) {
        *result = end;
        return true;
      }
      wordCurrent--;
    } else {
      if (isIdChar(*end)) {
        isCheckingWord = false;
      } else {
        isCheckingWord = true;
        wordCurrent    = wordEnd - 1;
      }
    }
    end--;
  }
  return false;
}

void GotoPosition(int x, int y) {
  EditorSetPosition edSetPos;
  edSetPos.CurTabPos     = -1;
  edSetPos.LeftPos       = -1;
  edSetPos.Overtype      = -1;
  edSetPos.TopScreenLine = -1;
  edSetPos.CurPos        = x;
  edSetPos.CurLine       = y;
  Info.EditorControl(ECTL_SETPOSITION, &edSetPos);
}

SHAREDSYMBOL HANDLE WINAPI EXP_NAME(OpenPlugin)(int OpenFrom, INT_PTR Item) {
  int menuTexts[] = {MAbove, MBelow};

  FarMenuItem menuItems[ARRAYSIZE(menuTexts)];
  memset(menuItems, 0, sizeof(menuItems));

  for (size_t i = 0; i < ARRAYSIZE(menuTexts); i++) {
    menuItems[i].Text = GetMsg(menuTexts[i]);
  }

  int selectedItem = Info.Menu(
      Info.ModuleNumber,
      -1,                // X, -1 - auto
      -1,                // Y, -1 - auto
      0,                 // MaxHeight, 0 - maximum
      FMENU_WRAPMODE,    // Flags
      GetMsg(MJumpWord), // Title
      nullptr,           // Bottom
      nullptr,           // HelpTopic
      nullptr,           // BreakKeys
      nullptr,           // BreakCode
      menuItems, ARRAYSIZE(menuItems));
  if (selectedItem == -1) {
    return (INVALID_HANDLE_VALUE);
  }

  bool isSearchingAbove = selectedItem == 0;

  EditorInfo edInfo;
  if (!Info.EditorControl(ECTL_GETINFO, &edInfo)) return (INVALID_HANDLE_VALUE);

  const wchar_t *lineBegin;
  const wchar_t *lineEnd;
  const wchar_t *wordBegin;
  const wchar_t *wordEnd;
  if (!FindCurrentWord(
          edInfo.CurPos, &lineBegin, &lineEnd, &wordBegin, &wordEnd))
    return (INVALID_HANDLE_VALUE);

#ifdef _DEBUG
  std::wstring line(lineBegin, lineEnd);
  fprintf(stderr, "\033[0;31mJUMPWORD:\033[m line:  '%ls'\n", line.c_str());
  std::string markers = std::string(wordBegin - lineBegin, ' ') + '[' +
                        std::string(wordEnd - wordBegin - 1, ' ') + ')';
  fprintf(stderr, "\033[0;31mJUMPWORD:\033[m word:   %s \n", markers.c_str());
#endif

  int currentLine            = edInfo.CurLine;
  const wchar_t *searchBreak = isSearchingAbove ? wordBegin : wordEnd + 1;

  while (currentLine < edInfo.TotalLines && currentLine >= 0) {

    if (isSearchingAbove) {
      // If the word is found at the start of line, there is no need to search
      // it again in the same line
      if (currentLine != edInfo.CurLine || wordBegin > lineBegin) {
        const wchar_t *foundWord;
        if (FindPreviousWord(
                lineBegin, searchBreak, wordBegin, wordEnd, &foundWord)) {
#ifdef _DEBUG
          std::wstring line(lineBegin, lineEnd);
          fprintf(
              stderr, "\033[0;31mJUMPWORD:\033[m line:  '%ls'\n", line.c_str());
          markers = std::string(foundWord - lineBegin, ' ') + '^';
          fprintf(
              stderr, "\033[0;31mJUMPWORD:\033[m found:  %s \n",
              markers.c_str());
#endif
          GotoPosition(foundWord - lineBegin, currentLine);
          return (INVALID_HANDLE_VALUE);
        }
      }
    } else {
      // If the word is found at the end of line, there is no need to search it
      // again in the same line
      if (currentLine != edInfo.CurLine || wordEnd < lineEnd) {
        const wchar_t *foundWord;
        if (FindNextWord(
                searchBreak, lineEnd, wordBegin, wordEnd, &foundWord)) {
#ifdef _DEBUG
          std::wstring line(lineBegin, lineEnd);
          fprintf(
              stderr, "\033[0;31mJUMPWORD:\033[m line:  '%ls'\n", line.c_str());
          markers = std::string(foundWord - lineBegin, ' ') + '^';
          fprintf(
              stderr, "\033[0;31mJUMPWORD:\033[m found:  %s \n",
              markers.c_str());
#endif
          GotoPosition(foundWord - lineBegin, currentLine);
          return (INVALID_HANDLE_VALUE);
        }
      }
    }

    if (isSearchingAbove) {
      currentLine--;
    } else {
      currentLine++;
    }

    EditorGetString edGetString = {};

    edGetString.StringNumber = currentLine;
    if (!Info.EditorControl(ECTL_GETSTRING, &edGetString))
      return (INVALID_HANDLE_VALUE);
    lineBegin   = edGetString.StringText;
    lineEnd     = edGetString.StringText + edGetString.StringLength;
    searchBreak = isSearchingAbove ? lineEnd : lineBegin;
  }

  return (INVALID_HANDLE_VALUE);
}

SHAREDSYMBOL void WINAPI EXP_NAME(GetPluginInfo)(struct PluginInfo *Info) {
  Info->StructSize            = sizeof(*Info);
  Info->Flags                 = PF_EDITOR | PF_DISABLEPANELS;
  Info->DiskMenuStringsNumber = 0;

  static const TCHAR *PluginMenuStrings[1];
  PluginMenuStrings[0]            = GetMsg(MJumpWord);
  Info->PluginMenuStrings         = PluginMenuStrings;
  Info->PluginMenuStringsNumber   = ARRAYSIZE(PluginMenuStrings);
  Info->PluginConfigStringsNumber = 0;
}
