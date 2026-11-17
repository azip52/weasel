# Word / zh-TW font experiment

**Current status:** the first text-language-only build does not fix pressing Enter
and accepting the automatically inherited style. That path can select PMingLiU
before input and DengXian after input. The earlier positive results below apply
only when the style was explicitly reapplied after Enter.

This revision adds an opt-in Word-specific Enter workaround. It lets Word handle
a plain Enter normally, then on key release copies the resulting paragraph
style's `Font.NameFarEast` into the empty insertion point. It does not hard-code
FangSong or Normal. It skips Enter used to commit an IME composition, modified
Enter, and nonempty selections. It is not yet verified in Word.

Enable the new experiment together with the existing text-language experiment:

```powershell
New-ItemProperty -Path HKCU:\Software\Rime\Weasel -Name WordFollowStyleOnEnter -PropertyType DWord -Value 1 -Force
New-ItemProperty -Path HKCU:\Software\Rime\Weasel -Name WordTextLanguage -PropertyType DWord -Value 0x0804 -Force
```

This intentionally makes a new paragraph follow its paragraph style's East Asian
font, rather than inheriting a manually overridden font from the previous
paragraph. Other direct formatting, including size, bold and italic, is retained.
Word's native object model is obtained from the same Word document window; no
external Word instance is created and no document text is logged.


This branch tests whether explicitly setting TSF `GUID_PROP_LANGID` after
inserting text prevents Word from replacing an inherited FangSong font with
PMingLiU. It is an opt-in workaround verified for the reported workflow in the environment below.

Verified for the reported new-paragraph workflow on 2026-11-17 (Asia/Taipei)
with Word 16.0.20527.20196 and build commit
`e55e7b2edbd2d6babaca34601654b1f0f1f50c46`.

| Test | Actual input LANGID | Selected committed text font |
| --- | --- | --- |
| Setting absent; new paragraph, explicitly apply Normal | 0x0404 | PMingLiU / 新細明體 |
| WordTextLanguage = 0x0804; same Word process and workflow | 0x0404 | FangSong |
| Word fully restarted; fresh original document; new paragraph; two consecutive commits | 0x0404 | FangSong for both selected characters |

The active Word thread keyboard language was checked separately from the
registry preference: it stayed 0x0404. The registered profile remained
`Hant=1`, `Profile=hant`. A preliminary 0x0804 input-language run was excluded
from the Taiwan comparison. The default-off binary reproduces the bug, so the
result is attributable to enabling the property workaround in this build.

**Mechanism and scope:** the inserted TSF range receives language property
0x0804 during insertion while the input method stays registered and active as
Chinese (Taiwan). In the observed tests, Word's status bar continued to show
Chinese (Taiwan) for the selected text. TSF language metadata should not be
assumed to be identical to Word's saved proofing-language metadata; the saved
DOCX language tags have not been audited.
Other Word versions, inline preedit, Undo, and replacement of existing selected
text have not yet been verified.

The change applies only inside WINWORD.EXE. It does not change the registered
input profile, the candidate window font, or Rime's Traditional Chinese output.
The default is disabled; the setting must be enabled separately after installation.
No document text is logged.

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
