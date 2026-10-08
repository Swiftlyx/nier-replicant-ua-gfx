<p align="center">
  <img src="docs/logo.png" alt="Ніер Реплікант вер.1.22474487139..." width="760">
</p>

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

Усе це налаштовується у файлі `NierReplicantGFX.ini`, опис кожного параметра є в самому файлі. Він лежить
поруч із DLL графіки: у теці гри (Standalone), у `LunarTear\mods\NierReplicantGFX` (Lunar Tear) або поруч
із `NierReplicantGFX.asi` (ASI). Зміни діють після перезапуску гри.

## Встановлення

Готові файли лежать на сторінці [Releases](https://github.com/Swiftlyx/nier-replicant-ua-gfx/releases/latest).
Для кожного плагіна там три архіви, по одному на спосіб підключення:

| Спосіб | Переклад | Графіка |
|---|---|---|
| Standalone | [UkrainianTranslation-Standalone.zip](https://github.com/Swiftlyx/nier-replicant-ua-gfx/releases/latest/download/UkrainianTranslation-Standalone.zip) | [NierReplicantGFX-Standalone.zip](https://github.com/Swiftlyx/nier-replicant-ua-gfx/releases/latest/download/NierReplicantGFX-Standalone.zip) |
| Lunar Tear | [UkrainianTranslation-LunarTear.zip](https://github.com/Swiftlyx/nier-replicant-ua-gfx/releases/latest/download/UkrainianTranslation-LunarTear.zip) | [NierReplicantGFX-LunarTear.zip](https://github.com/Swiftlyx/nier-replicant-ua-gfx/releases/latest/download/NierReplicantGFX-LunarTear.zip) |
| ASI | [UkrainianTranslation-ASI.zip](https://github.com/Swiftlyx/nier-replicant-ua-gfx/releases/latest/download/UkrainianTranslation-ASI.zip) | [NierReplicantGFX-ASI.zip](https://github.com/Swiftlyx/nier-replicant-ua-gfx/releases/latest/download/NierReplicantGFX-ASI.zip) |

### Який спосіб обрати

Найпростіше й найшвидше — **Standalone**: завантажте архів і перетягніть його вміст у теку гри, нічого
більше встановлювати не треба.

- **Standalone** — якщо вам потрібні лише ці моди. Сторонні програми не потрібні, а плагін завантажується
  найраніше.
- **Lunar Tear** — якщо ви вже ставите через нього інші моди. Він об'єднує архіви всіх модів, тож вони не
  заважають один одному.
- **ASI** — якщо у вас уже стоїть Ultimate ASI Loader або Special K, наприклад для інших модів.

Для кожного плагіна оберіть один спосіб і не змішуйте їх: переклад, скажімо, через Standalone, а графіку
через Lunar Tear можна, але переклад двома способами одночасно — ні.

Усі архіви розпаковуються в теку гри, де лежить `NieR Replicant ver.1.22474487139.exe`. У Steam її відкриває
пункт контекстного меню гри **Керування → Переглянути локальні файли** (Manage → Browse local files).
Зазвичай це `C:\Program Files (x86)\Steam\steamapps\common\NieR Replicant ver.1.22474487139`.

### Standalone

1. Розпакуйте `UkrainianTranslation-Standalone.zip` і, якщо потрібна графіка,
   `NierReplicantGFX-Standalone.zip` у теку гри, або відкрийте архів і просто перетягніть його вміст туди.
2. Перевірте, що файли лежать так (у `data` є й власні файли гри, їх не чіпайте):

   ```
   NieR Replicant ver.1.22474487139\
   ├─ NieR Replicant ver.1.22474487139.exe
   ├─ dinput8.dll                  переклад
   ├─ xinput9_1_0.dll              графіка
   ├─ NierReplicantGFX.ini         налаштування графіки
   └─ data\
      ├─ ua.arc                    переклад
      └─ info_uk.arc               переклад
   ```

Гра сама завантажує DLL з такими назвами зі своєї теки. У Steam Deck або Proton для цього додайте в
параметри запуску гри `WINEDLLOVERRIDES="dinput8,xinput9_1_0=n,b" %command%`.

### Lunar Tear

1. Встановіть [Lunar Tear](https://www.nexusmods.com/nierreplicant/mods/87) за інструкцією з його сторінки.
2. Розпакуйте `UkrainianTranslation-LunarTear.zip` і, якщо потрібна графіка, `NierReplicantGFX-LunarTear.zip`
   у теку гри. В архівах уже є шлях `LunarTear\mods\…`, тож моди самі опиняться на місці. Якщо тек
   `LunarTear` чи `mods` ще немає, вони створяться під час розпакування:

   ```
   NieR Replicant ver.1.22474487139\
   ├─ NieR Replicant ver.1.22474487139.exe
   ├─ dxgi.dll                     сам Lunar Tear (або d3d11.dll чи dinput8.dll)
   └─ LunarTear\
      └─ mods\
         ├─ UkrainianTranslation\
         │  ├─ NierReplicantUA.dll
         │  ├─ ua.arc
         │  ├─ info.arc
         │  └─ manifest.json
         └─ NierReplicantGFX\
            ├─ NierReplicantGFX.dll
            ├─ NierReplicantGFX.ini   налаштування графіки
            └─ manifest.json
   ```

Lunar Tear підміняє одразу три бібліотеки, тож його файл може називатися `dxgi.dll` (так радить його
автор), `d3d11.dll` або `dinput8.dll` — будь-яка з цих назв, яку не займає інший мод. `dinput8.dll` — це
також наш Standalone-переклад.

Lunar Tear можна завантажувати й через Special K: додайте його файл у меню Special K як плагін (див.
нижче) з **Load Order: Early**. Тоді назва й тека файлу довільні, а моди лишаються в `LunarTear\mods` у
теці гри:

```
NieR Replicant ver.1.22474487139\
├─ NieR Replicant ver.1.22474487139.exe
├─ d3d11.dll                    Special K, якщо встановлений локально (див. нижче)
├─ d3d11.ini                    налаштування Special K
├─ LunarTear.dll                Lunar Tear, підключений у меню Special K (назва й тека довільні)
└─ LunarTear\
   └─ mods\                     моди Lunar Tear лишаються тут
      ├─ UkrainianTranslation\
      └─ NierReplicantGFX\
```

Lunar Tear має завантажитися раніше, ніж гра відкриє свої архіви. Якщо в `LunarTear\lunartear.log` є
рядок «VFS hook missed», він завантажився запізно, і архіви модів, зокрема переклад, гра не побачить.

### Ultimate ASI Loader і Special K

Гра 64-бітна й працює на DirectX 11, тож потрібні 64-бітні (x64) версії завантажувачів. Спершу
розпакуйте в теку гри архіви `*-ASI.zip`.

Завантажувач кладуть у теку гри під назвою бібліотеки, яку гра завантажує сама під час запуску: Windows
бере файл із теки гри замість системного. Для NieR Replicant підходять, зокрема, `winmm.dll`, `d3d11.dll`,
`dxgi.dll` і `dinput8.dll` (DirectX 11 і функції, які гра імпортує). Конкретна назва неважлива, якщо
завантажувач її підтримує і її не займає інший мод: `dinput8.dll` і `xinput9_1_0.dll` — наші
Standalone-файли, `dxgi.dll` — Lunar Tear.

З Ultimate ASI Loader під назвою `winmm.dll` файли лежать так:

```
NieR Replicant ver.1.22474487139\
├─ NieR Replicant ver.1.22474487139.exe
├─ winmm.dll                    Ultimate ASI Loader (або інша вільна назва)
├─ NierReplicantUA.asi          переклад
├─ NierReplicantGFX.asi         графіка
├─ NierReplicantGFX.ini         налаштування графіки, завжди поруч із NierReplicantGFX.asi
└─ data\
   ├─ ua.arc                    переклад, завжди в data гри
   └─ info_uk.arc               переклад, завжди в data гри
```

Файли `.asi` (разом з `NierReplicantGFX.ini`) можна перенести й у підтеку `scripts` чи `plugins`, а
`ua.arc` і `info_uk.arc` мають лишатися в `data` гри.

Зі Special K, встановленим локально, файли можуть лежати так:

```
NieR Replicant ver.1.22474487139\
├─ NieR Replicant ver.1.22474487139.exe
├─ d3d11.dll                    Special K (SpecialK64.dll під вільною назвою: d3d11.dll чи dxgi.dll)
├─ d3d11.ini                    налаштування Special K, з тією ж назвою, що й його DLL
├─ NierReplicantUA.asi          переклад
├─ NierReplicantGFX.asi         графіка
├─ NierReplicantGFX.ini         налаштування графіки, поруч із NierReplicantGFX.asi
└─ data\
   ├─ ua.arc                    переклад, завжди в data гри
   └─ info_uk.arc               переклад, завжди в data гри
```

При глобальній ін'єкції `d3d11.dll` і `d3d11.ini` в теці гри немає: налаштування Special K лежать у
`Документи\My Mods\SpecialK\Profiles\…`.

**[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader)**

1. На сторінці [релізів](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases) завантажте
   `Ultimate-ASI-Loader_x64.zip`.
2. Покладіть DLL з архіву в теку гри під вільною назвою, наприклад `winmm.dll` або `d3d11.dll`.

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

Щоб видалити плагіни, приберіть файли, розпаковані з архівів. Файли гри вони не змінюють.

## Лог

Кожен плагін пише лог поруч зі своєю DLL чи `.asi`: `NierReplicantUA.log` і `NierReplicantGFX.log`. Якщо
лога немає, плагін не завантажився: перевірте, чи файли лежать так, як на схемах вище. Так виглядає лог
перекладу, коли все працює (Standalone чи ASI):

```
NierReplicantUA 1.0.4 by Swiftlyx, loaded at 31 ms after process start
pool     applied: text_common pool 1 MiB
fishing  applied: records in cm/kg
pickup   applied: long item names in "Obtained"
talker   applied: long speaker names
index    applied: data\info_uk.arc
index    mount "" at 133 ms: info_uk.arc
index    mount "dlc\dlc01\" at 829 ms: info.arc
```

А так — лог графіки:

```
NierReplicantGFX 1.0.1 by Swiftlyx, loaded at 508 ms after process start
ssaa     off (RenderScale = 1.0)
shaders  waiting for D3D11CreateDevice (AO: NierReplicantGFX shader, strength 1.00; feedback blur removed, dither on)
shaders  10-bit colour buffers created at 16 bits
shaders  device created at 838 ms, creation methods wrapped
shader   fixed: pfx_manual_apply_oetf_p.0
shader   replaced: gen_ssao_mask_default_c -> NierReplicantGFX AO
```

## Якщо щось не так

- **Текст англійський.** У налаштуваннях гри має бути мова тексту English. Якщо вона вже стоїть, гляньте в
  `NierReplicantUA.log`: при Standalone і ASI там має бути рядок `index    mount "" … info_uk.arc`, а файл
  `data\info_uk.arc` — на своєму місці. Мод для Lunar Tear підхоплює сам Lunar Tear: у
  `LunarTear\lunartear.log` не має бути рядка «VFS hook missed».
- **Частина меню й налаштувань порожня, гра вилітає на історіях зброї.** У `NierReplicantUA.log` буде
  рядок `pool     skipped`: плагін завантажився пізніше, ніж гра прочитала текст. Для ASI поставте
  Load Order: Early, або перейдіть на Standalone: він завантажується найраніше.
- **Інші моди перестали працювати.** Варіанти Standalone і ASI підставляють грі власний індекс
  архівів (`info_uk.arc`), тож моди, які теж підміняють файли гри через архіви Lunar Tear, разом із ними не
  працюватимуть. Ставте все разом через Lunar Tear.
- **Антивірус позначає `dxgi.dll`.** У цих архівах такого файлу немає: `dxgi.dll` у теці гри — це Lunar Tear
  або Special K. Завантажуйте їх лише з офіційних сторінок. Standalone обходиться без сторонніх DLL.
- **Гра оновилася або Steam перевірив файли.** Перевірка файлів ці моди не прибирає, бо вони не замінюють
  файлів гри. Якщо оновлення змінить код гри, плагіни нічого не чіпатимуть і напишуть у лог `skipped`; тоді
  потрібна нова версія модів.
- **Ім'я героя не вводиться кирилицею.** Гра читає клавіші як латиницю, тож ім'я вводиться латиницею.
- **Гра гальмує з NierReplicantGFX.** Найважчий для відеокарти суперсемплінг: поставте `RenderScale = 1.0`
  (за замовчуванням) або менше значення, наприклад 1.5.
- **Затінення здається надто темним чи світлим.** Змініть `Strength` у `NierReplicantGFX.ini`; `Shader = 0`
  повертає затінення гри.

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

Settings are in `NierReplicantGFX.ini`, each one described in the file. It lies next to the graphics DLL:
in the game folder (Standalone), in `LunarTear\mods\NierReplicantGFX` (Lunar Tear) or next to
`NierReplicantGFX.asi` (ASI). Restart the game after a change.

### Installation

Ready-to-use files are on the [Releases](https://github.com/Swiftlyx/nier-replicant-ua-gfx/releases/latest)
page, three archives per plugin, one per loading method. The simplest and fastest is Standalone: download
the archive and drag its contents into the game folder. Which one to pick:

- **Standalone**, if you only want these mods: no third-party tools, and the plugin loads first;
- **Lunar Tear**, if you already install other mods with it: it merges the archives of all mods;
- **ASI**, if you already use Ultimate ASI Loader or Special K.

Use one method per plugin. Extract the archives into the game folder, the one with
`NieR Replicant ver.1.22474487139.exe` (in Steam: Manage → Browse local files; usually
`C:\Program Files (x86)\Steam\steamapps\common\NieR Replicant ver.1.22474487139`).

- **Standalone:** extract `*-Standalone.zip` into the game folder; the game loads these DLLs by itself. On
  Steam Deck or Proton add `WINEDLLOVERRIDES="dinput8,xinput9_1_0=n,b" %command%` to the launch options.

  ```
  NieR Replicant ver.1.22474487139\
  ├─ NieR Replicant ver.1.22474487139.exe
  ├─ dinput8.dll                  translation
  ├─ xinput9_1_0.dll              graphics
  ├─ NierReplicantGFX.ini         graphics settings
  └─ data\
     ├─ ua.arc                    translation
     └─ info_uk.arc               translation
  ```

- **Lunar Tear:** install [Lunar Tear](https://www.nexusmods.com/nierreplicant/mods/87), then extract
  `*-LunarTear.zip` into the game folder; the archives already contain `LunarTear\mods\…`, so missing
  folders are created. Lunar Tear stands in for three libraries, so its file may be named `dxgi.dll` (as its
  author suggests), `d3d11.dll` or `dinput8.dll`, whichever no other mod uses. It can also be loaded
  through Special K as a plug-in with Load Order Early, under any file name and in any folder; the mods
  stay in `LunarTear\mods`. If `LunarTear\lunartear.log` says "VFS hook missed", it loaded too late and
  archive mods are not applied.

  ```
  NieR Replicant ver.1.22474487139\
  ├─ dxgi.dll                     Lunar Tear itself (or d3d11.dll, dinput8.dll; or loaded by Special K)
  └─ LunarTear\
     └─ mods\
        ├─ UkrainianTranslation\    NierReplicantUA.dll, ua.arc, info.arc, manifest.json
        └─ NierReplicantGFX\        NierReplicantGFX.dll, NierReplicantGFX.ini, manifest.json
  ```

- **ASI:** extract `*-ASI.zip` into the game folder. The game is 64-bit DirectX 11, so use x64 loaders,
  named after a library the game loads at startup, such as `winmm.dll`, `d3d11.dll`, `dxgi.dll` or
  `dinput8.dll`. Any such name works if the loader supports it and no other mod uses it (`dinput8.dll` and
  `xinput9_1_0.dll` are the Standalone files, `dxgi.dll` is Lunar Tear). The `.asi` files may move to a
  `scripts` or `plugins` subfolder together with `NierReplicantGFX.ini`; `ua.arc` and `info_uk.arc` stay in
  the game's `data` folder.

  ```
  NieR Replicant ver.1.22474487139\
  ├─ winmm.dll                    Ultimate ASI Loader (or another free name)
  ├─ NierReplicantUA.asi          translation
  ├─ NierReplicantGFX.asi         graphics
  ├─ NierReplicantGFX.ini         graphics settings, always next to NierReplicantGFX.asi
  └─ data\
     ├─ ua.arc                    translation, always in the game's data
     └─ info_uk.arc               translation, always in the game's data
  ```

  - [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader): put the DLL from
    `Ultimate-ASI-Loader_x64.zip` into the game folder, for example as `winmm.dll` or `d3d11.dll`;
  - [Special K](https://www.special-k.info/) ([GitHub](https://github.com/SpecialKO/SpecialK)): in the
    Special K menu (Ctrl + Shift + Backspace) open Plug-Ins → Third-Party → Add Plug-In, add each `.asi`
    and set its Load Order to Early; it takes effect on the next start. Or add an
    `[Import.NierReplicantUA]` section (and the same for GFX) to `SpecialK.ini` of the game profile or to
    the ini of a local install, with `Architecture=x64`, `Role=ThirdParty`, `When=Early` and the full
    path in `Filename`. Early loading is required for the translation. A local install may look like this
    (with global injection there is no Special K DLL in the game folder; its settings are in
    `Documents\My Mods\SpecialK\Profiles\…`):

    ```
    NieR Replicant ver.1.22474487139\
    ├─ d3d11.dll                    Special K (SpecialK64.dll under a free name: d3d11.dll or dxgi.dll)
    ├─ d3d11.ini                    Special K settings, named like its DLL
    ├─ NierReplicantUA.asi
    ├─ NierReplicantGFX.asi
    ├─ NierReplicantGFX.ini         next to NierReplicantGFX.asi
    └─ data\
       ├─ ua.arc                    always in the game's data
       └─ info_uk.arc               always in the game's data
    ```

For the translation, choose English as the text language in the game settings: the translation takes
the English slot. To remove the mods, delete the extracted files; the game files are not changed.

### If something is wrong

Each plugin writes a log next to its DLL or `.asi` (`NierReplicantUA.log`, `NierReplicantGFX.log`). No log
means the plugin was not loaded: check the file layout above.

- **The text is in English.** Choose English as the text language. With Standalone or ASI the
  translation log must contain `index    mount "" … info_uk.arc`; with Lunar Tear, `lunartear.log` must not
  say "VFS hook missed".
- **Parts of the menus are empty, the game crashes on weapon stories.** The log says `pool     skipped`:
  the plugin loaded after the game read its text. Set Load Order to Early or use Standalone.
- **Other archive mods stopped working.** Standalone and ASI give the game their own archive index, so
  mods that replace game files through Lunar Tear archives do not combine with them; install everything
  through Lunar Tear.
- **An antivirus flags `dxgi.dll`.** These archives contain no such file: `dxgi.dll` in the game folder is
  Lunar Tear or Special K. Get them from their official pages; Standalone needs no third-party DLL.
- **The game was updated or Steam verified the files.** Verification leaves these mods in place, since
  they replace no game files. If an update changes the game code, the plugins change nothing and log
  `skipped`; a new version of the mods is needed then.
- **The game runs slowly with NierReplicantGFX.** Supersampling is the heaviest: use `RenderScale = 1.0`
  (the default) or a lower value such as 1.5.
- **The ambient occlusion looks too dark or too light.** Change `Strength` in `NierReplicantGFX.ini`;
  `Shader = 0` brings back the game's own shading.

### Redistribution

The plugin code is MIT licensed: copies must keep `Copyright (c) 2026 Swiftlyx` and the license text.
The translation files in the releases (`ua.arc`, `info.arc`, `info_uk.arc`) are not covered by the MIT
license. They may be redistributed, including on other sites and in modpacks, only with credit to
Swiftlyx and a link to this repository or the mod page.

### Building

`build.bat` builds both plugins with MSVC Build Tools 2022 (x64). `build.bat test [game exe] [libtp folder]`
also runs the offline tests, which need Python 3.
