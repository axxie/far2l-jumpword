#include <farplug-wide.h>
#include "JumpWordLng.hpp"
#include "JumpWord.hpp"

#include <utils.h>
#include <KeyFileHelper.h>

const TCHAR *GetMsg(int MsgId)
{
  return (Info.GetMsg(Info.ModuleNumber, MsgId));
}

SHAREDSYMBOL int WINAPI EXP_NAME(GetMinFarVersion)()
{
  return FARMANAGERVERSION;
}

SHAREDSYMBOL void WINAPI EXP_NAME(SetStartupInfo)(const struct PluginStartupInfo *Info)
{
  ::Info = *Info;
  ::FSF = *Info->FSF;
  ::Info.FSF = &::FSF;
}

SHAREDSYMBOL HANDLE WINAPI EXP_NAME(OpenPlugin)(int OpenFrom, INT_PTR Item)
{
  int menuTexts[] = {MAbove, MBelow};

  FarMenuItem menuItems[ARRAYSIZE(menuTexts)];
  memset(menuItems, 0, sizeof(menuItems));

  for (size_t i = 0; i < ARRAYSIZE(menuTexts); i++)
  {
    menuItems[i].Text = GetMsg(menuTexts[i]);
  }

  if (Info.Menu(
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
          menuItems,
          ARRAYSIZE(menuItems)) == -1)
  {
    return (INVALID_HANDLE_VALUE);
  }

  return (INVALID_HANDLE_VALUE);
}

SHAREDSYMBOL void WINAPI EXP_NAME(GetPluginInfo)(struct PluginInfo *Info)
{
  Info->StructSize = sizeof(*Info);
  Info->Flags = PF_EDITOR | PF_DISABLEPANELS;
  Info->DiskMenuStringsNumber = 0;
  static const TCHAR *PluginMenuStrings[1];
  PluginMenuStrings[0] = GetMsg(MJumpWord);
  Info->PluginMenuStrings = PluginMenuStrings;
  Info->PluginMenuStringsNumber = ARRAYSIZE(PluginMenuStrings);
  Info->PluginConfigStringsNumber = 0;
}
