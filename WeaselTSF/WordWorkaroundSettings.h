#pragma once

namespace word_font_workaround {

inline DWORD ReadSetting(const wchar_t* name, DWORD defaultValue) {
  DWORD value = defaultValue;
  DWORD size = sizeof(value);
  const LSTATUS result =
      RegGetValueW(HKEY_CURRENT_USER, L"Software\\Rime\\Weasel", name,
                   RRF_RT_REG_DWORD, nullptr, &value, &size);
  if (result == ERROR_SUCCESS)
    return value;
  if (result == ERROR_FILE_NOT_FOUND || result == ERROR_PATH_NOT_FOUND)
    return defaultValue;
  return 0;
}

inline DWORD TextLanguage() {
  return ReadSetting(L"WordTextLanguage", 0x0804);
}

inline bool FollowStyleOnEnter() {
  return ReadSetting(L"WordFollowStyleOnEnter", 1) == 1;
}

}  // namespace word_font_workaround
