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
  ::Info = *Info;
  ::FSF = *Info->FSF;
  ::Info.FSF = &::FSF;
}

bool isIdChar(const wchar_t c) {
  return c == L'_' || FSF.LIsAlphanum(c);
}

bool FindCurrentWord(
    const wchar_t **resLineBegin, const wchar_t **resLineEnd,
    const wchar_t **resBegin, const wchar_t **resEnd) {
  EditorInfo edInfo;
  if (!Info.EditorControl(ECTL_GETINFO, &edInfo)) return false;

  EditorGetString edGetString = {};
  edGetString.StringNumber = -1;
  if (!Info.EditorControl(ECTL_GETSTRING, &edGetString)) return false;

  const size_t length = edGetString.StringLength;
  size_t x = edInfo.CurPos;
  if (x >= length) return false;

  const wchar_t *line = edGetString.StringText;
  size_t i = x;
  while (i >= 0 && isIdChar(line[i])) i--;

  size_t j = x;
  while (j < length && isIdChar(line[j])) j++;

  if (j <= i + 1) return false;

  *resLineBegin = line;
  *resLineEnd = line + length;
  *resBegin = line + (i + 1);
  *resEnd = line + j;
  return true;
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

  const wchar_t *lineBegin;
  const wchar_t *lineEnd;
  const wchar_t *wordBegin;
  const wchar_t *wordEnd;
  if (!FindCurrentWord(&lineBegin, &lineEnd, &wordBegin, &wordEnd))
    return (INVALID_HANDLE_VALUE);

#ifdef _DEBUG
  std::wstring line(lineBegin, lineEnd);
  fprintf(stderr, "\033[0;31mJUMPWORD:\033[m '%ls'\n", line.c_str());
  std::string markers = std::string(wordBegin - lineBegin, ' ') + '^' +
                        std::string(wordEnd - wordBegin - 1, ' ') + '^';
  fprintf(stderr, "\033[0;31mJUMPWORD:\033[m '%s'\n", markers.c_str());
#endif

  return (INVALID_HANDLE_VALUE);
}

SHAREDSYMBOL void WINAPI EXP_NAME(GetPluginInfo)(struct PluginInfo *Info) {
  Info->StructSize = sizeof(*Info);
  Info->Flags = PF_EDITOR | PF_DISABLEPANELS;
  Info->DiskMenuStringsNumber = 0;
  static const TCHAR *PluginMenuStrings[1];
  PluginMenuStrings[0] = GetMsg(MJumpWord);
  Info->PluginMenuStrings = PluginMenuStrings;
  Info->PluginMenuStringsNumber = ARRAYSIZE(PluginMenuStrings);
  Info->PluginConfigStringsNumber = 0;
}
