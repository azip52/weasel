# Word / zh-TW font experiment

This branch tests whether explicitly setting TSF `GUID_PROP_LANGID` after
inserting text prevents Word from replacing an inherited FangSong font with
PMingLiU. It is an experiment, not a verified fix.

The change applies only inside WINWORD.EXE. It does not change the registered
input profile, the candidate window font, or Rime's Traditional Chinese output.
The default is disabled. No document text is logged.

## Compare the same binary with and without the experiment

1. Install the build and register Weasel as Traditional Chinese (Taiwan).
2. Close all Word windows and open the original test document again.
3. With the registry value absent (or zero), create a new paragraph, explicitly
   apply the Normal / 內文 style, type Chinese with Weasel, then **select the
   newly committed characters** and record the font shown by Word.
4. Enable the experimental text language (Simplified Chinese / China):

   ```powershell
   New-ItemProperty -Path HKCU:\Software\Rime\Weasel -Name WordTextLanguage -PropertyType DWord -Value 0x0804 -Force
   ```

5. Repeat in a fresh paragraph with the same style. Select the committed text
   again; the caret's font display alone is not evidence of success.
6. Also test multiple commits, inline preedit, replacing selected text, Undo,
   and another application. Confirm the active input profile remains zh-TW.

`0x0404` can be used as a control with an explicit Taiwan text language. Any
other value, including zero, disables the experiment. The setting is read for
each edit, so toggling it does not require reinstalling.

To restore the original behavior:

```powershell
Remove-ItemProperty -Path HKCU:\Software\Rime\Weasel -Name WordTextLanguage -ErrorAction SilentlyContinue
```

DebugView or a debugger can observe `Weasel WordTextLanguage: applied/rejected`
messages. These report only the property API result, not the resulting font.
An accepted property does not prove that Word preserves formatting.

## API basis

- [TSF predefined properties](https://learn.microsoft.com/en-us/windows/win32/tsf/predefined-properties)
- [ITfProperty::SetValue](https://learn.microsoft.com/en-us/windows/win32/api/msctf/nf-msctf-itfproperty-setvalue)

The property requires a nonempty range and a read/write edit cookie. Editing
the covered text discards it, so this branch applies it after both inline
preedit and committed `SetText` calls, before collapsing the range.
