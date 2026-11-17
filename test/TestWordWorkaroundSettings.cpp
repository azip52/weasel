#include <windows.h>
#include <iostream>
#include <string>
#include "../WeaselTSF/WordWorkaroundSettings.h"

int main() {
  wchar_t ci[16] = {};
  if (!GetEnvironmentVariableW(L"GITHUB_ACTIONS", ci, 16) ||
      wcscmp(ci, L"true"))
    return 2;
  const std::wstring path = L"Software\\WeaselWordDefaultsTest-" +
                            std::to_wstring(GetCurrentProcessId());
  HKEY root = nullptr;
  DWORD disposition = 0;
  if (RegCreateKeyExW(HKEY_CURRENT_USER, path.c_str(), 0, nullptr, 0,
                      KEY_ALL_ACCESS, nullptr, &root, &disposition) !=
          ERROR_SUCCESS ||
      disposition != REG_CREATED_NEW_KEY)
    return 3;
  if (RegOverridePredefKey(HKEY_CURRENT_USER, root) != ERROR_SUCCESS) {
    RegCloseKey(root);
    RegDeleteTreeW(HKEY_CURRENT_USER, path.c_str());
    return 4;
  }
  bool passed = word_font_workaround::TextLanguage() == 0x0804 &&
                word_font_workaround::FollowStyleOnEnter();
  HKEY settings = nullptr;
  passed = passed &&
           RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Rime\\Weasel", 0,
                           nullptr, 0, KEY_ALL_ACCESS, nullptr, &settings,
                           nullptr) == ERROR_SUCCESS;
  if (settings) {
    passed = passed && word_font_workaround::TextLanguage() == 0x0804 &&
             word_font_workaround::FollowStyleOnEnter();
    DWORD value = 0;
    RegSetValueExW(settings, L"WordTextLanguage", 0, REG_DWORD,
                   reinterpret_cast<const BYTE*>(&value), sizeof(value));
    RegSetValueExW(settings, L"WordFollowStyleOnEnter", 0, REG_DWORD,
                   reinterpret_cast<const BYTE*>(&value), sizeof(value));
    passed = passed && word_font_workaround::TextLanguage() == 0 &&
             !word_font_workaround::FollowStyleOnEnter();
    const wchar_t invalid[] = L"invalid";
    RegSetValueExW(settings, L"WordTextLanguage", 0, REG_SZ,
                   reinterpret_cast<const BYTE*>(invalid), sizeof(invalid));
    RegSetValueExW(settings, L"WordFollowStyleOnEnter", 0, REG_SZ,
                   reinterpret_cast<const BYTE*>(invalid), sizeof(invalid));
    passed = passed && word_font_workaround::TextLanguage() == 0 &&
             !word_font_workaround::FollowStyleOnEnter();
    RegCloseKey(settings);
  }
  RegOverridePredefKey(HKEY_CURRENT_USER, nullptr);
  RegCloseKey(root);
  RegDeleteTreeW(HKEY_CURRENT_USER, path.c_str());
  std::cout << (passed ? "Word defaults and explicit opt-out passed.\n"
                       : "Word settings test failed.\n");
  return passed ? 0 : 1;
}
