# NieR Replicant ver.1.22474487139: ліміти тексту й графіка

Два плагіни для Steam-версії NieR Replicant ver.1.22474487139:

- **NierReplicantUA** знімає обмеження гри на довжину тексту, на які натрапив український переклад;
- **NierReplicantGFX** виправляє графіку і працює з будь-якою мовою гри.

Плагіни змінюють код гри лише в пам'яті під час запуску, файли гри лишаються як є. Перед кожною
зміною плагін перевіряє, що код на цьому місці такий самий, як у Steam-версії. Якщо ні (інша версія
гри або інший мод), зміна пропускається, а причина записується в лог поруч із DLL.

Автор: Swiftlyx. Ліцензія коду: [MIT](LICENSE), умови для файлів перекладу — у розділі «Поширення».
*English below.*

## NierReplicantUA

Український текст довший за англійський, і кількох внутрішніх буферів гри йому не вистачає. Плагін:

- збільшує сховище рядків інтерфейсу з 256 КБ до 1 МБ. Без цього частина меню лишається порожньою,
  а на історіях зброї гра вилітає;
- показує в повідомленні «Отримано: …» повну назву предмета, а не перші ~13 літер;
- не обрізає імена мовців над репліками (до 63 байтів);
- показує рекорди риболовлі в сантиметрах і кілограмах;
- читає індекс архівів із `data\info_uk.arc`, тож оригінальний `data\info.arc` не треба замінювати.

Тексти, шрифт і текстури перекладу лежать в архіві `ua.arc`, який входить у готові файли перекладу
разом із плагіном (див. «Встановлення»). Це ШІ-переклад: текст переклала модель Claude Opus 5.5
(Extra high) від Anthropic, терміни, стиль, правки й перевірка в грі — Swiftlyx.

## NierReplicantGFX

- **Ambient occlusion.** Затінення рахує власний шейдер плагіна замість ігрового. Сила тіні та сама,
  але немає візерунка, що рухається разом із камерою, і тінь не блідне біля країв екрана та навколо
  персонажів.
- **Без шлейфу кадрів.** Гра змішує кадр із попередніми (feedback blur), найпомітніше під час перекиду.
  Плагін це вимикає.
- **Без кілець на світінні.** Гра тримає кадр у 10-бітних буферах, тож світіння й темні переходи йдуть
  кільцями. Плагін робить ці буфери 16-бітними й додає дизеринг на виході.
- **Суперсемплінг (SSAA).** Гра рендерить кадр більшим за екран і зменшує його, тож краї предметів
  рівні. За замовчуванням вимкнено, вмикається параметром `RenderScale`.

Усе це налаштовується в `NierReplicantGFX.ini` поруч із DLL, опис кожного параметра є в самому файлі.

## Встановлення

Готові файли лежать на сторінці [Releases](https://github.com/Swiftlyx/nier-replicant-ua-gfx/releases/latest).
Для кожного плагіна там три архіви, по одному на спосіб підключення:

| Спосіб | Переклад | Графіка |
|---|---|---|
| Standalone | [UkrainianTranslation-Standalone.zip](https://github.com/Swiftlyx/nier-replicant-ua-gfx/releases/latest/download/UkrainianTranslation-Standalone.zip) | [NierReplicantGFX-Standalone.zip](https://github.com/Swiftlyx/nier-replicant-ua-gfx/releases/latest/download/NierReplicantGFX-Standalone.zip) |
| Lunar Tear | [UkrainianTranslation-LunarTear.zip](https://github.com/Swiftlyx/nier-replicant-ua-gfx/releases/latest/download/UkrainianTranslation-LunarTear.zip) | [NierReplicantGFX-LunarTear.zip](https://github.com/Swiftlyx/nier-replicant-ua-gfx/releases/latest/download/NierReplicantGFX-LunarTear.zip) |
| ASI | [UkrainianTranslation-ASI.zip](https://github.com/Swiftlyx/nier-replicant-ua-gfx/releases/latest/download/UkrainianTranslation-ASI.zip) | [NierReplicantGFX-ASI.zip](https://github.com/Swiftlyx/nier-replicant-ua-gfx/releases/latest/download/NierReplicantGFX-ASI.zip) |

### Який спосіб обрати

- **Standalone** — якщо вам потрібні лише ці моди. Сторонні програми не потрібні, а плагін завантажується
  найраніше.
- **Lunar Tear** — якщо ви вже ставите через нього інші моди. Він об'єднує архіви всіх модів, тож вони не
  заважають один одному.
- **ASI** — якщо у вас уже стоїть Ultimate ASI Loader або Special K, наприклад для інших модів.

Для кожного плагіна оберіть один спосіб і не змішуйте їх: переклад, скажімо, через Standalone, а графіку
через Lunar Tear можна, але переклад двома способами одночасно — ні.

Теку гри в Steam відкриває пункт контекстного меню гри **Керування → Переглянути локальні файли**
(Manage → Browse local files). У ній лежить `NieR Replicant ver.1.22474487139.exe`.

### Standalone

1. Розпакуйте `UkrainianTranslation-Standalone.zip` і, якщо потрібна графіка,
   `NierReplicantGFX-Standalone.zip` у теку гри.
2. Поруч із `NieR Replicant ver.1.22474487139.exe` мають опинитися `dinput8.dll` і файли `data\ua.arc`,
   `data\info_uk.arc` (переклад), а також `xinput9_1_0.dll` і `NierReplicantGFX.ini` (графіка). Гра сама
   завантажує DLL з такими назвами зі своєї теки.

У Steam Deck або Proton додайте в параметри запуску гри
`WINEDLLOVERRIDES="dinput8,xinput9_1_0=n,b" %command%`.

### Lunar Tear

1. Встановіть [Lunar Tear](https://www.nexusmods.com/nierreplicant/mods/87) за інструкцією з його сторінки.
2. Розпакуйте `UkrainianTranslation-LunarTear.zip` і, якщо потрібна графіка, `NierReplicantGFX-LunarTear.zip`
   у теку гри. В архівах уже є шлях `LunarTear\mods\…`, тож моди самі опиняться в
   `LunarTear\mods\UkrainianTranslation` і `LunarTear\mods\NierReplicantGFX`. Якщо тек `LunarTear` чи `mods`
   ще немає, вони створяться під час розпакування.

Lunar Tear можна завантажувати й через Special K: додайте його в меню Special K як плагін (див. нижче) з
**Load Order: Early**. Він має завантажитися раніше, ніж гра відкриє свої архіви. Якщо в
`LunarTear\lunartear.log` є рядок «VFS hook missed», він завантажився запізно, і архіви модів, зокрема
переклад, гра не побачить.

### Ultimate ASI Loader і Special K

Гра 64-бітна й працює на DirectX 11, тож потрібні 64-бітні (x64) версії завантажувачів. Спершу
розпакуйте в теку гри архіви `*-ASI.zip`. Файли `data\ua.arc` і `data\info_uk.arc` мають лишатися в теці
`data` гри, а `NierReplicantGFX.ini` — поруч із `NierReplicantGFX.asi`.

Завантажувач кладуть у теку гри під назвою бібліотеки, яку гра завантажує сама під час запуску: Windows
бере файл із теки гри замість системного. Для NieR Replicant підходять, зокрема, `winmm.dll`, `d3d11.dll`,
`dxgi.dll` і `dinput8.dll` (DirectX 11 і функції, які гра імпортує). Конкретна назва неважлива, якщо
завантажувач її підтримує і її не займає інший мод: `dinput8.dll` і `xinput9_1_0.dll` — наші
Standalone-файли, `dxgi.dll` — Lunar Tear.

**[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader)**

1. На сторінці [релізів](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases) завантажте
   `Ultimate-ASI-Loader_x64.zip`.
2. Покладіть DLL з архіву в теку гри під вільною назвою, наприклад `winmm.dll` або `d3d11.dll`.
3. Файли `.asi` можна лишити в теці гри або перенести в підтеку `scripts` чи `plugins`.

**[Special K](https://www.special-k.info/)** ([GitHub](https://github.com/SpecialKO/SpecialK))

1. Встановіть Special K із сайту або з [релізів на GitHub](https://github.com/SpecialKO/SpecialK/releases).
   Найпростіше — глобальна ін'єкція через програму Special K (SKIF): запустіть її службу, потім гру. Для
   локального встановлення покладіть `SpecialK64.dll` у теку гри під вільною назвою для DirectX 11:
   `d3d11.dll` або `dxgi.dll`, якщо її не займає Lunar Tear.
2. Запустіть гру й відкрийте меню Special K (Ctrl + Shift + Backspace): **Plug-Ins → Third-Party →
   Add Plug-In**. Додайте кожен `.asi` і поставте йому **Load Order: Early**. Плагіни запрацюють з
   наступного запуску гри.
3. Або те саме вручну: допишіть у `SpecialK.ini` профілю гри (`Документи\My Mods\SpecialK\Profiles\…`)
   при глобальній ін'єкції чи в ini поруч із DLL Special K (`d3d11.ini`, `dxgi.ini`) при локальному
   встановленні. У `Filename` — повний шлях до файлу:

   ```ini
   [Import.NierReplicantUA]
   Architecture=x64
   Role=ThirdParty
   When=Early
   Filename=C:\Program Files (x86)\Steam\steamapps\common\NieR Replicant ver.1.22474487139\NierReplicantUA.asi

   [Import.NierReplicantGFX]
   Architecture=x64
   Role=ThirdParty
   When=Early
   Filename=C:\Program Files (x86)\Steam\steamapps\common\NieR Replicant ver.1.22474487139\NierReplicantGFX.asi
   ```

   Раннє завантаження (Early) обов'язкове для перекладу: плагін має змінити гру раніше, ніж вона прочитає
   текст.

### Після встановлення

Щоб увімкнути переклад, оберіть у налаштуваннях гри мову тексту English. Переклад займає її місце,
тому в списку мов він так і називається: English.

Щоб видалити плагіни, приберіть файли, розпаковані з архівів.

## Лог

Кожен плагін пише лог поруч зі своєю DLL: `NierReplicantUA.log` і `NierReplicantGFX.log`. Наприклад:

```
NierReplicantGFX 1.0.0 by Swiftlyx, loaded at 508 ms after process start
ssaa     off (RenderScale = 1.0)
shaders  waiting for D3D11CreateDevice (AO: NierReplicantGFX shader, strength 1.00; feedback blur removed, dither on)
shaders  10-bit colour buffers created at 16 bits
shaders  device created at 838 ms, creation methods wrapped
shader   fixed: pfx_manual_apply_oetf_p.0
shader   replaced: gen_ssao_mask_default_c -> NierReplicantGFX AO
```

Якщо в лозі NierReplicantUA є рядок `pool     skipped: text_common is already loaded`, плагін
завантажився запізно, коли гра вже прочитала текст. Підключіть його як `dinput8.dll`: так він
завантажується найраніше.

## Як це працює

NierReplicantUA (адреси відносно початку exe):

| Група | Адреси | Що змінюється |
|---|---|---|
| `pool` | `0xD736A`, `0xD6E84`, `0xD6F0D`, `0xD7A75`, `0xD79CA` | сховище `text_common` на 1 МБ замість 256 КБ (`VirtualAlloc`); лише поки менеджер тексту (`0x27E0590`) порожній |
| `fishing` | `0x3A7D01`, `0x3A80D9` | см і кг для англійського слота: діапазон мов 3–5 → 2–5 |
| `pickup` | виклики `snprintf` у `0xC17B0` | назва предмета пишеться відразу в 128-байтовий буфер повідомлення |
| `talker` | `0xD68F3` | завантажувач `talker_name.tnd` не обрізає імена до 31 байта |
| `index` | `0x8ED281` | функція `0x8ED1F0` підключає теку архівів (основні дані — з порожньою назвою теки, DLC — `dlc\dlc01\`) і відкриває в ній `info.arc`; для основних даних вставка підставляє `info_uk.arc`, якщо він є, а DLC читає свій `info.arc`. Рядки гри не змінюються |

NierReplicantGFX перехоплює `D3D11CreateDevice` в імпорті гри, а на створеному пристрої — створення
текстур, подань (views) і шейдерів:

- шейдери гри він упізнає за контрольною сумою DXBC. Шейдер AO `gen_ssao_mask_default_c` замінюється
  на [shaders/ssao_mask.hlsl](shaders/ssao_mask.hlsl), інші отримують вставки інструкцій із
  `shader_edits.h` (опис кожної правки є в `tools/gen_shader_edits.py`). У `shader_edits.h` лише
  контрольні суми, зсуви й вставлені інструкції, коду гри там немає;
- 10-бітні текстури R10G10B10A2 без початкових даних створюються як R16G16B16A16, а подання до них
  отримують відповідний формат;
- для суперсемплінгу в `0x7D425E` вставлено виклик, що множить внутрішню роздільність на
  `RenderScale`, а фінальний прохід (`0x8B31FB`, `0x8B3236`) центрує кадр за прямокутником 16:9 екрана.

Обидві DLL експортують `LunarTearPluginInit`, щоб їх завантажував Lunar Tear, але його Plugin API не
використовують.

## Поширення

- Код плагінів поширюється за ліцензією [MIT](LICENSE): копії мають зберігати рядок
  `Copyright (c) 2026 Swiftlyx` і текст ліцензії.
- Файли перекладу з релізів (`ua.arc`, `info.arc`, `info_uk.arc`) під MIT не підпадають. Поширювати їх,
  зокрема на інших сайтах і в збірках модів, можна лише з обов'язковим зазначенням автора (Swiftlyx) і
  посиланням на цей репозиторій або сторінку мода.

## Збірка

Потрібні MSVC Build Tools 2022 (x64), а для тестів і генераторів ще Python 3.

```
build.bat                          збирає обидва плагіни в out\ разом із копіями .asi, dinput8.dll, xinput9_1_0.dll
build.bat test [exe гри] [libtp]   збирає й запускає офлайн-тести
python tools\gen_shader_edits.py <pfx_shader> <resident_shader> [--check]   оновлює shader_edits.h
python tools\gen_ao_shader.py [--check]                                      компілює shaders\ssao_mask.hlsl в ao_shader.h
```

`libtp` — тека з файлами `pfx_shader` і `resident_shader` із `system\graphic\libtp` гри. Тести
відкривають exe гри як образ у пам'яті (гра при цьому не запускається), перевіряють кожне змінене
місце й виконують вставлений код. Змінені шейдери вони порівнюють із результатом генератора на Python
і створюють на програмному пристрої Direct3D 11 (WARP), а обидва шейдери AO запускають на тестовій
сцені.

---

## English: text limits and graphics fixes

Two plugins for the Steam version of NieR Replicant ver.1.22474487139:

- **NierReplicantUA** lifts the text length limits of the game that the Ukrainian translation runs into;
- **NierReplicantGFX** fixes graphics and works with any game language.

Both change the game's code in memory at startup and leave the game files alone. Before each change a
plugin checks that the code at that place matches the Steam version. If it does not (another game
version or another mod), the change is skipped and the reason goes to the log next to the DLL.

Author: Swiftlyx. Code license: MIT; see Redistribution for the translation files.

### NierReplicantUA

Ukrainian text is longer than English, and several internal buffers of the game are too small for it.
The plugin:

- enlarges the interface string pool from 256 KB to 1 MB (otherwise parts of the menus stay empty and
  the game crashes on weapon stories);
- shows the full item name in "Obtained ..." instead of the first ~13 letters;
- keeps speaker names up to 63 bytes;
- shows fishing records in centimetres and kilograms;
- reads the archive index from `data\info_uk.arc`, so the original `data\info.arc` stays as it is.

The translation itself (text, font, textures) is in `ua.arc`, which comes with the plugin in the
translation archives. It is an AI translation: the text was translated by Claude Opus 5.5 (Extra high)
by Anthropic; terminology, style, corrections and in-game testing by Swiftlyx.

### NierReplicantGFX

- **Ambient occlusion.** The plugin's own shader replaces the game's. The strength is the same, but
  there is no pattern moving with the camera, and shadows do not fade near the screen edges or around
  characters.
- **No feedback blur.** The game blends each frame with the previous ones, most visibly when rolling.
- **No rings in glows.** The game keeps the frame in 10-bit buffers; the plugin makes them 16-bit and
  dithers the output.
- **Supersampling (SSAA).** The frame is rendered larger than the screen and scaled down. Off by
  default, see `RenderScale`.

Settings are in `NierReplicantGFX.ini` next to the DLL, each one described in the file.

### Installation

Ready-to-use files are on the [Releases](https://github.com/Swiftlyx/nier-replicant-ua-gfx/releases/latest)
page, three archives per plugin, one per loading method. Which one to pick:

- **Standalone**, if you only want these mods: no third-party tools, and the plugin loads first;
- **Lunar Tear**, if you already install other mods with it: it merges the archives of all mods;
- **ASI**, if you already use Ultimate ASI Loader or Special K.

Use one method per plugin. The game folder is the one with `NieR Replicant ver.1.22474487139.exe`
(in Steam: Manage → Browse local files).

- **Standalone:** extract `*-Standalone.zip` into the game folder. `dinput8.dll` with `data\ua.arc` and
  `data\info_uk.arc` (translation), and `xinput9_1_0.dll` with `NierReplicantGFX.ini` (graphics) end up
  next to the exe; the game loads these DLLs by itself. On Steam Deck or Proton add
  `WINEDLLOVERRIDES="dinput8,xinput9_1_0=n,b" %command%` to the launch options.
- **Lunar Tear:** install [Lunar Tear](https://www.nexusmods.com/nierreplicant/mods/87), then extract
  `*-LunarTear.zip` into the game folder; the archives already contain `LunarTear\mods\…`, so the folders
  are created if missing. Lunar Tear can also be loaded through Special K with Load Order Early; if
  `LunarTear\lunartear.log` says "VFS hook missed", it loaded too late and archive mods are not applied.
- **ASI:** extract `*-ASI.zip` into the game folder (`data\` files stay in the game's `data` folder,
  `NierReplicantGFX.ini` next to `NierReplicantGFX.asi`). The game is 64-bit DirectX 11, so use x64
  loaders, named after a library the game loads at startup, such as `winmm.dll`, `d3d11.dll`, `dxgi.dll`
  or `dinput8.dll`. Any such name works if the loader supports it and no other mod uses it
  (`dinput8.dll` and `xinput9_1_0.dll` are the Standalone files, `dxgi.dll` is Lunar Tear).
  - [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader): put the DLL from
    `Ultimate-ASI-Loader_x64.zip` into the game folder, for example as `winmm.dll` or `d3d11.dll`;
  - [Special K](https://www.special-k.info/) ([GitHub](https://github.com/SpecialKO/SpecialK)): in the
    Special K menu (Ctrl + Shift + Backspace) open Plug-Ins → Third-Party → Add Plug-In, add each `.asi`
    and set its Load Order to Early; it takes effect on the next start. Or add an
    `[Import.NierReplicantUA]` section (and the same for GFX) to `SpecialK.ini` of the game profile or to
    the ini of a local install, with `Architecture=x64`, `Role=ThirdParty`, `When=Early` and the full
    path in `Filename`. Early loading is required for the translation.

For the translation, choose English as the text language in the game settings: the translation takes
the English slot.

### Redistribution

The plugin code is MIT licensed: copies must keep `Copyright (c) 2026 Swiftlyx` and the license text.
The translation files in the releases (`ua.arc`, `info.arc`, `info_uk.arc`) are not covered by the MIT
license. They may be redistributed, including on other sites and in modpacks, only with credit to
Swiftlyx and a link to this repository or the mod page.

### Building

`build.bat` builds both plugins with MSVC Build Tools 2022 (x64). `build.bat test [game exe] [libtp folder]`
also runs the offline tests, which need Python 3.
