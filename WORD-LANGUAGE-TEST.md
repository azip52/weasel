# Word / zh-TW font workaround

Verified for the reported workflow on 2026-11-17 (Asia/Taipei), with Word
16.0.20527.20196 and build commit
`9faf00e9b09c0b8c350d8d81cbbd738c62d355b1`.

[Successful build and upgrade tests](https://github.com/azip52/weasel/actions/runs/43996297729)

## Reported problem and fix

With Weasel registered and active as Chinese (Taiwan), pressing Enter at the end
of a Normal / 內文 paragraph can change the insertion font to PMingLiU. A
text-language-only workaround can then produce DengXian on committed text.
Explicitly reapplying the style after Enter hides this additional failure path.

This build adds `WordFollowStyleOnEnter`. After a plain Enter has been passed
through to Word, the key-release callback reads the resulting selection's style
and assigns that style's `Font.NameFarEast` to the empty insertion point. It uses
Word's native object model from the same document window. The paragraph style
and desired font are not hard-coded.

The existing `WordTextLanguage` experiment sets TSF `GUID_PROP_LANGID` after
inline or committed text insertion. Both settings were enabled in the successful
tests below.

## Verified comparison

The original document's theme was left unchanged. No style was manually
reapplied after Enter.

| Setting / test | After Enter | After input and selecting the committed text |
| --- | --- | --- |
| Text-language-only build | PMingLiU / 新細明體 | DengXian |
| Same new binary, WordFollowStyleOnEnter = 0 | PMingLiU / 新細明體 | DengXian after the UI settled |
| WordFollowStyleOnEnter = 1 and WordTextLanguage = 0x0804 | FangSong | FangSong |
| Word fully closed and restarted, original document reopened | FangSong | FangSong for two consecutive commits |
| Another plain Enter from that newly typed paragraph | FangSong | FangSong for a different candidate |

For the restart checks, the selected text was observed again after a four-second
pause so the result did not rely on a stale ribbon display. The Word thread's
actual keyboard LANGID was 0x0404 before and after testing. Registration stayed
`Hant=1`, `Profile=hant`.

## Enable / disable

The settings are opt-in, disabled by default, and read for each relevant key or
edit. Installation alone does not enable them on another machine.

```powershell
New-ItemProperty -Path HKCU:\Software\Rime\Weasel -Name WordFollowStyleOnEnter -PropertyType DWord -Value 1 -Force
New-ItemProperty -Path HKCU:\Software\Rime\Weasel -Name WordTextLanguage -PropertyType DWord -Value 0x0804 -Force
```

To disable the experiments:

```powershell
Remove-ItemProperty -Path HKCU:\Software\Rime\Weasel -Name WordFollowStyleOnEnter -ErrorAction SilentlyContinue
Remove-ItemProperty -Path HKCU:\Software\Rime\Weasel -Name WordTextLanguage -ErrorAction SilentlyContinue
```

## Scope and behavior

- The Enter hook runs only in WINWORD.EXE with keyboard LANGID 0x0404.
- Enter used to commit an IME composition, modified Enter, and nonempty
  selections are excluded.
- A new paragraph intentionally follows its style's East Asian font instead of
  inheriting a manually overridden East Asian font from the previous paragraph.
  The code assigns only NameFarEast, rather than resetting all character
  formatting.
- Word still creates the paragraph and chooses its next style. No paragraph
  text, theme, style definition, numbering, or Rime conversion scheme is changed.
- The TSF inserted range receives language property 0x0804 during insertion.
  Registration and the actual keyboard language remain Taiwan. In these tests
  Word's status bar continued to show Chinese (Taiwan); saved DOCX language tags
  have not been audited.
- Other Word versions, tables, protected documents, inline preedit, and Undo
  grouping have not been verified. This is an application-specific workaround.
- Diagnostics emit only restored/skipped or applied/rejected API outcomes. No
  document text is logged.

## API references

- [AccessibleObjectFromWindow / Word native object model](https://learn.microsoft.com/en-us/windows/win32/api/oleacc/nf-oleacc-accessibleobjectfromwindow)
- [Word Font.NameFarEast](https://learn.microsoft.com/en-us/office/vba/api/word.font.namefareast)
- [TSF predefined properties](https://learn.microsoft.com/en-us/windows/win32/tsf/predefined-properties)
- [ITfProperty::SetValue](https://learn.microsoft.com/en-us/windows/win32/api/msctf/nf-msctf-itfproperty-setvalue)
