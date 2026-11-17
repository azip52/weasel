#pragma once

#include <oleacc.h>
#include <cwchar>

#pragma comment(lib, "oleacc.lib")

namespace word_font_workaround {

inline bool Enabled() {
  static const bool isWord = [] {
    wchar_t path[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(nullptr, path, ARRAYSIZE(path));
    if (!length || length >= ARRAYSIZE(path))
      return false;
    const wchar_t* name = wcsrchr(path, L'\\');
    return _wcsicmp(name ? name + 1 : path, L"WINWORD.EXE") == 0;
  }();
  if (!isWord || (LOWORD(GetKeyboardLayout(0)) != 0x0404))
    return false;
  DWORD enabled = 0;
  DWORD size = sizeof(enabled);
  return RegGetValueW(HKEY_CURRENT_USER, L"Software\\Rime\\Weasel",
                       L"WordFollowStyleOnEnter", RRF_RT_REG_DWORD, nullptr,
                       &enabled, &size) == ERROR_SUCCESS &&
         enabled == 1;
}

inline HRESULT GetProperty(IDispatch* object,
                            const wchar_t* name,
                            CComVariant& value) {
  if (!object)
    return E_POINTER;
  DISPID id;
  LPOLESTR member = const_cast<LPOLESTR>(name);
  HRESULT hr = object->GetIDsOfNames(IID_NULL, &member, 1, LOCALE_USER_DEFAULT,
                                     &id);
  if (FAILED(hr))
    return hr;
  DISPPARAMS args = {};
  return object->Invoke(id, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET,
                        &args, &value, nullptr, nullptr);
}

inline com_ptr<IDispatch> GetObject(IDispatch* object, const wchar_t* name) {
  CComVariant value;
  if (FAILED(GetProperty(object, name, value)) || value.vt != VT_DISPATCH)
    return nullptr;
  return value.pdispVal;
}

inline HRESULT PutProperty(IDispatch* object,
                            const wchar_t* name,
                            CComVariant& value) {
  if (!object)
    return E_POINTER;
  DISPID id;
  LPOLESTR member = const_cast<LPOLESTR>(name);
  HRESULT hr = object->GetIDsOfNames(IID_NULL, &member, 1, LOCALE_USER_DEFAULT,
                                     &id);
  if (FAILED(hr))
    return hr;
  DISPID named = DISPID_PROPERTYPUT;
  DISPPARAMS args = {&value, &named, 1, 1};
  return object->Invoke(id, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT,
                        &args, nullptr, nullptr, nullptr);
}

inline BOOL CALLBACK FindDocumentWindow(HWND hwnd, LPARAM data) {
  wchar_t name[32] = {};
  if (GetClassNameW(hwnd, name, ARRAYSIZE(name)) &&
      wcscmp(name, L"_WwG") == 0) {
    com_ptr<IDispatch> candidate;
    if (SUCCEEDED(AccessibleObjectFromWindow(hwnd, OBJID_NATIVEOM, IID_IDispatch,
                                             reinterpret_cast<void**>(&candidate))) &&
        candidate) {
      *reinterpret_cast<com_ptr<IDispatch>*>(data) = candidate;
      return FALSE;
    }
  }
  return TRUE;
}

// Run after a pass-through, unmodified Enter, once Word has created its new
// paragraph. Use that paragraph's current style rather than hard-coding Normal
// or FangSong. Only the East Asian font is changed; paragraph style, numbering,
// size, emphasis, and Word's own Enter behavior are left to Word.
inline HRESULT RestoreStyleFont(ITfContext* context) {
  if (!Enabled())
    return S_FALSE;
  com_ptr<ITfContextView> view;
  HWND hwnd = nullptr;
  if (FAILED(context->GetActiveView(&view)) ||
      FAILED(view->GetWnd(&hwnd)) || !hwnd)
    return E_FAIL;
  DWORD process = 0;
  GetWindowThreadProcessId(hwnd, &process);
  if (process != GetCurrentProcessId())
    return E_FAIL;
  com_ptr<IDispatch> window;
  EnumChildWindows(GetAncestor(hwnd, GA_ROOT), FindDocumentWindow,
                   reinterpret_cast<LPARAM>(std::addressof(window)));
  auto selection = GetObject(window, L"Selection");
  CComVariant type;
  if (FAILED(GetProperty(selection, L"Type", type)) || type.vt != VT_I4 ||
      type.lVal != 1)  // wdSelectionIP: never reformat selected document text.
    return S_FALSE;
  auto style = GetObject(selection, L"Style");
  auto styleFont = GetObject(style, L"Font");
  CComVariant name;
  if (FAILED(GetProperty(styleFont, L"NameFarEast", name)) ||
      name.vt != VT_BSTR || !name.bstrVal || !SysStringLen(name.bstrVal))
    return E_FAIL;
  auto font = GetObject(selection, L"Font");
  return PutProperty(font, L"NameFarEast", name);
}

inline void FollowParagraphStyle(ITfContext* context) {
  static thread_local bool restoring = false;
  if (restoring)
    return;
  restoring = true;
  const HRESULT hr = RestoreStyleFont(context);
  restoring = false;
  if (hr == S_OK)
    OutputDebugStringW(L"Weasel Word paragraph font: restored\n");
  else
    OutputDebugStringW(L"Weasel Word paragraph font: skipped\n");
}

inline bool IsPlainEnter(WPARAM key) {
  return key == VK_RETURN && !(GetKeyState(VK_SHIFT) & 0x8000) &&
         !(GetKeyState(VK_CONTROL) & 0x8000) &&
         !(GetKeyState(VK_MENU) & 0x8000) && Enabled();
}

}  // namespace word_font_workaround
