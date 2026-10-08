# NieR Replicant ver.1.22474487139: ліміти тексту й графіка

Два плагіни для Steam-версії NieR Replicant ver.1.22474487139:

- **NierReplicantUA** знімає обмеження гри на довжину тексту, на які натрапив український переклад;
- **NierReplicantGFX** виправляє графіку і працює з будь-якою мовою гри.

Плагіни змінюють код гри лише в пам'яті під час запуску, файли гри лишаються як є. Перед кожною
зміною плагін перевіряє, що код на цьому місці такий самий, як у Steam-версії. Якщо ні (інша версія
гри або інший мод), зміна пропускається, а причина записується в лог поруч із DLL.

Автор: Swiftlyx. Ліцензія: [MIT](LICENSE). *English below.*

## NierReplicantUA

Український текст довший за англійський, і кількох внутрішніх буферів гри йому не вистачає. Плагін:

- збільшує сховище рядків інтерфейсу з 256 КБ до 1 МБ. Без цього частина меню лишається порожньою,
  а на історіях зброї гра вилітає;
- показує в повідомленні «Отримано: …» повну назву предмета, а не перші ~13 літер;
- не обрізає імена мовців над репліками (до 63 байтів);
- показує рекорди риболовлі в сантиметрах і кілограмах;
- читає індекс архівів із `data\info_uk.arc`, тож оригінальний `data\info.arc` не треба замінювати.

Тексти, шрифт і текстури перекладу лежать в архіві `ua.arc`, який входить у готові файли перекладу
разом із плагіном (див. «Встановлення»).

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
Для кожного плагіна там три архіви, по одному на спосіб підключення. Оберіть один спосіб і розпакуйте
архів у теку гри, де лежить `NieR Replicant ver.1.22474487139.exe`.

| Спосіб | Переклад | Графіка |
|---|---|---|
| [Lunar Tear](https://github.com/ifa-ifa/Lunar-Tear) (має бути вже встановлений) | [UkrainianTranslation-LunarTear.zip](https://github.com/Swiftlyx/nier-replicant-ua-gfx/releases/latest/download/UkrainianTranslation-LunarTear.zip) | [NierReplicantGFX-LunarTear.zip](https://github.com/Swiftlyx/nier-replicant-ua-gfx/releases/latest/download/NierReplicantGFX-LunarTear.zip) |
| Без завантажувача: гра сама завантажує `dinput8.dll` і `xinput9_1_0.dll` зі своєї теки | [UkrainianTranslation-Standalone.zip](https://github.com/Swiftlyx/nier-replicant-ua-gfx/releases/latest/download/UkrainianTranslation-Standalone.zip) | [NierReplicantGFX-Standalone.zip](https://github.com/Swiftlyx/nier-replicant-ua-gfx/releases/latest/download/NierReplicantGFX-Standalone.zip) |
| ASI-завантажувач ([Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) або [Special K](https://www.special-k.info/) з `When=Early`) | [UkrainianTranslation-ASI.zip](https://github.com/Swiftlyx/nier-replicant-ua-gfx/releases/latest/download/UkrainianTranslation-ASI.zip) | [NierReplicantGFX-ASI.zip](https://github.com/Swiftlyx/nier-replicant-ua-gfx/releases/latest/download/NierReplicantGFX-ASI.zip) |

У Steam Deck або Proton для способу без завантажувача додайте в параметри запуску гри
`WINEDLLOVERRIDES="dinput8,xinput9_1_0=n,b" %command%`.

Щоб увімкнути переклад, оберіть у налаштуваннях гри мову тексту English. Переклад займає англійський
слот, тому після вибору ця мова в списку називатиметься «Українська».

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
| `index` | рядок `0xD07728` | `info.arc` → `info_uk.arc`, якщо є `data\info_uk.arc` |

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

Author: Swiftlyx. License: MIT.

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
translation archives.

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
page, three archives per plugin, one per loading method. Pick one method per plugin and extract its
archive into the game folder (where `NieR Replicant ver.1.22474487139.exe` is):

- `*-LunarTear.zip`: a mod for [Lunar Tear](https://github.com/ifa-ifa/Lunar-Tear), which must be installed;
- `*-Standalone.zip`: no loader needed, the game loads `dinput8.dll` and `xinput9_1_0.dll` from its
  folder. On Steam Deck or Proton add `WINEDLLOVERRIDES="dinput8,xinput9_1_0=n,b" %command%` to the
  launch options;
- `*-ASI.zip`: for an ASI loader ([Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader), or [Special K](https://www.special-k.info/) with `When=Early`).

For the translation, choose English as the text language in the game settings: the translation takes
the English slot.

### Building

`build.bat` builds both plugins with MSVC Build Tools 2022 (x64). `build.bat test [game exe] [libtp folder]`
also runs the offline tests, which need Python 3.
