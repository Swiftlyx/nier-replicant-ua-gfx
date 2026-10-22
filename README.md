<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/logo.png">
    <img src="docs/logo_light.png" alt="Нір Реплікант ver.1.22474487139..." width="760">
  </picture>
</p>

<p align="center"><b>Українська</b> · <a href="README.en.md">English</a></p>

# NieR Replicant ver.1.22474487139: ліміти тексту й графіка

Два плагіни для Steam-версії NieR Replicant ver.1.22474487139:

- **NierReplicantUA** знімає обмеження гри на довжину тексту, на які натрапив український переклад;
- **NierReplicantGFX** виправляє графіку і працює з будь-якою мовою гри.

Плагіни змінюють код гри лише в пам'яті під час запуску, файли гри лишаються як є. Перед кожною
зміною плагін перевіряє, що код на цьому місці такий самий, як у Steam-версії. Якщо ні (інша версія
гри або інший мод), зміна пропускається, а причина записується в лог поруч із DLL.

Автор: Swiftlyx. Ліцензія коду: [MIT](LICENSE), умови для файлів перекладу — у розділі «Поширення».

## NierReplicantUA

Український текст довший за англійський, і кількох внутрішніх буферів гри йому не вистачає. Плагін:

- збільшує сховище рядків інтерфейсу з 256 КБ до 1 МБ. Без цього частина меню лишається порожньою,
  а на історіях зброї гра вилітає;
- показує в повідомленні «Отримано: …» повну назву предмета, а не перші ~13 літер;
- не обрізає імена мовців над репліками (до 63 байтів);
- показує рекорди риболовлі в сантиметрах і кілограмах;
- у списках магії та зброї ставить назву перед прикріпленими словами (меню «Слова», Word Edit): «Чорна
  рука смерті наспівує пісню» замість «смерті наспівує пісню Чорна рука»;
- читає індекс архівів із `data\info_uk.arc`, тож оригінальний `data\info.arc` не треба замінювати.

Цей порядок налаштовується у файлі `NierReplicantUA.ini` поруч із DLL перекладу:
`NameFirst = 1` (за замовчуванням) — спершу назва, `NameFirst = 0` — як у грі, спершу слова. Зміни діють
після перезапуску гри.

Тексти, шрифт і текстури перекладу лежать в архіві `ua.arc`, який входить у готові файли перекладу
разом із плагіном (див. «Встановлення»). Це ШІ-переклад: текст переклала модель Claude Opus 5.5
(Extra high) від Anthropic, терміни, стиль, правки й перевірка в грі — Swiftlyx.

### Word Edit: що в плагіні, а що в перекладі

Гра зберігає порядок слів у двох різних місцях, тож і змінюється він у двох:

- **Рядок у списках магії та зброї** гра складає в коді: бере слово A, слово B і назву в порядку з
  таблиці всередині exe, окремо для кожної мови. У файлах гри цього порядку немає, тож змінити його може
  лише плагін. Він переписує рядок англійської мови (її місце займає переклад) на «назва, A, B», як у грі
  вже зроблено для французької, італійської та іспанської.
- **Три колонки на екрані, де обирають слова**, гра малює на місцях із розмітки цього екрана
  (`white_book_word_separate_word` в архіві гри). Розмітка — це дані, тож колонки переставлено в `ua.arc`,
  разом з іншими правками інтерфейсу перекладу. Те, яка колонка що означає (вибір, списки слів, колір
  назви), цей екран бере з копії тієї ж таблиці в exe. Якщо змінити копію, колонки лишаються на своїх
  місцях, а списки слів відкриваються порожніми, тому плагін її не чіпає.

Через це `NameFirst = 0` повертає порядок лише в списках магії та зброї: на екрані вибору слів назва
лишається ліворуч. Текст перекладу розрахований на порядок «назва, слова»: слово A стоїть у родовому
відмінку й пишеться з малої літери.

## NierReplicantGFX

- **Ambient occlusion.** Затінення рахує власний шейдер плагіна замість ігрового. Сила тіні та сама,
  але немає візерунка, що рухається разом із камерою, а біля країв екрана та навколо персонажів тінь
  блідне набагато менш помітно. Візерунок видно лише в русі й там, де тіней багато.
- **Без шлейфу кадрів.** Гра змішує кадр із попередніми (feedback blur), найпомітніше під час перекиду.
  Плагін це вимикає.
- **Без кілець на світінні.** Гра тримає кадр у 10-бітних буферах, тож світіння й темні переходи йдуть
  кільцями. Плагін робить ці буфери 16-бітними й додає дизеринг на виході.
- **Суперсемплінг (SSAA).** Гра рендерить кадр більшим за екран і зменшує його, тож краї предметів
  рівні. За замовчуванням вимкнено, вмикається параметром `RenderScale`. Працює разом із
  [NierReplicantFix](https://codeberg.org/Lyall/NierReplicantFix), зокрема з його підтримкою екранів
  будь-якого співвідношення сторін.

Усе це налаштовується у файлі `NierReplicantGFX.ini`, опис кожного параметра є в самому файлі. Він лежить
поруч із DLL графіки: у теці гри (Standalone), у `LunarTear\mods\NierReplicantGFX` (Lunar Tear) або поруч
із `NierReplicantGFX.asi` (ASI). Зміни діють після перезапуску гри.

### Як це виглядає

Тестова сцена, прогнана через справжній шейдер затінення гри й через шейдер плагіна: не кадри з гри, а
пояснення того, що саме змінює плагін. Ліворуч гра, праворуч NierReplicantGFX.

![Затінення під час повороту камери](docs/demo_ao_rotation.gif)

Камера повертається, як правим стіком. Плями в затіненні гри стоять на місці екрана, а сцена під ними
рухається, тож здається, що по тінях за камерою пливе візерунок. Видно це лише в русі й там, де тіней
багато, тому на знімках екрана різниця помітна слабше, ніж у грі.

![Шум, який повертає напрямки пошуку тіней](docs/ao_pattern_noise.png)

Звідки береться візерунок: гра повертає напрямки, в яких шукає тіні, на свій кут для кожного пікселя.
Цей шум надто грубий для розмиття гри. Плагін використовує візерунок 4×4 пікселі, який розмиття прибирає.

![Тьмяне світіння: 10 біт без дизерингу проти 16 біт із дизерингом](docs/demo_glow_banding.png)

Тьмяне світіння в темряві: у 10 бітах біля чорного видно кільця, у 16 бітах із дизерингом перехід плавний.
Нижній ряд — те саме з яскравістю ×6.

![Увесь кадр і лише затінення](docs/demo_frame.png)

Увесь кадр (угорі) і лише затінення з посиленим контрастом (унизу); праворуч власне затінення плагіна,
`RenderScale = 2.0` і дизеринг.

## Встановлення

Готові файли лежать на сторінці [Releases](https://github.com/Swiftlyx/nier-replicant-ua-gfx/releases/latest).
Для кожного плагіна там три архіви, по одному на спосіб встановлення:

| Спосіб | Переклад | Графіка |
|---|---|---|
| Standalone | [UkrainianTranslation-Standalone.zip](https://github.com/Swiftlyx/nier-replicant-ua-gfx/releases/latest/download/UkrainianTranslation-Standalone.zip) | [NierReplicantGFX-Standalone.zip](https://github.com/Swiftlyx/nier-replicant-ua-gfx/releases/latest/download/NierReplicantGFX-Standalone.zip) |
| Lunar Tear | [UkrainianTranslation-LunarTear.zip](https://github.com/Swiftlyx/nier-replicant-ua-gfx/releases/latest/download/UkrainianTranslation-LunarTear.zip) | [NierReplicantGFX-LunarTear.zip](https://github.com/Swiftlyx/nier-replicant-ua-gfx/releases/latest/download/NierReplicantGFX-LunarTear.zip) |
| ASI | [UkrainianTranslation-ASI.zip](https://github.com/Swiftlyx/nier-replicant-ua-gfx/releases/latest/download/UkrainianTranslation-ASI.zip) | [NierReplicantGFX-ASI.zip](https://github.com/Swiftlyx/nier-replicant-ua-gfx/releases/latest/download/NierReplicantGFX-ASI.zip) |

Ті самі архіви є на Nexus Mods: [переклад](https://www.nexusmods.com/games/nierreplicant/mods/131) і [графіка](https://www.nexusmods.com/nierreplicant/mods/130). Нові версії спершу з'являються тут.

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

Самі завантажувачі можна поєднувати: Special K підключає Lunar Tear чи файли `.asi` як плагіни, а Ultimate
ASI Loader працює поруч із Lunar Tear, якщо в них різні назви файлів. Головне, щоб кожна DLL мала свою
вільну назву.

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
   ├─ NierReplicantUA.ini          налаштування перекладу
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
   `LunarTear` чи `mods` ще немає, вони створяться під час розпакування; якщо ні, створіть їх самі й
   покладіть туди файли з архівів:

   ```
   NieR Replicant ver.1.22474487139\
   ├─ NieR Replicant ver.1.22474487139.exe
   ├─ dxgi.dll                     сам Lunar Tear (або d3d11.dll чи dinput8.dll)
   └─ LunarTear\
      └─ mods\
         ├─ UkrainianTranslation\
         │  ├─ NierReplicantUA.dll
         │  ├─ NierReplicantUA.ini     налаштування перекладу
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
├─ NierReplicantUA.ini          налаштування перекладу, завжди поруч із NierReplicantUA.asi
├─ NierReplicantGFX.asi         графіка
├─ NierReplicantGFX.ini         налаштування графіки, завжди поруч із NierReplicantGFX.asi
└─ data\
   ├─ ua.arc                    переклад, завжди в data гри
   └─ info_uk.arc               переклад, завжди в data гри
```

Файли `.asi` (разом зі своїми `.ini`) можна перенести й у підтеку `scripts` чи `plugins`, а
`ua.arc` і `info_uk.arc` мають лишатися в `data` гри.

Зі Special K, встановленим локально, файли можуть лежати так:

```
NieR Replicant ver.1.22474487139\
├─ NieR Replicant ver.1.22474487139.exe
├─ d3d11.dll                    Special K (SpecialK64.dll під вільною назвою: d3d11.dll чи dxgi.dll)
├─ d3d11.ini                    налаштування Special K, з тією ж назвою, що й його DLL
├─ NierReplicantUA.asi          переклад
├─ NierReplicantUA.ini          налаштування перекладу, поруч із NierReplicantUA.asi
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
NierReplicantUA 1.0.5 by Swiftlyx, loaded at 31 ms after process start
pool     applied: text_common pool 1 MiB
fishing  applied: records in cm/kg
pickup   applied: long item names in "Obtained"
talker   applied: long speaker names
words    applied: Word Edit lines as name, word A, word B
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
  Load Order: Early (у Special K), або перейдіть на Standalone: він завантажується найраніше.
- **Інші моди перестали працювати.** Варіанти Standalone і ASI підставляють грі власний індекс
  архівів (`info_uk.arc`), тож моди, які теж підміняють файли гри через архіви Lunar Tear, разом із ними не
  працюватимуть. Ставте все разом через Lunar Tear.
- **Плагін не завантажується, лога немає.** Windows може заблокувати файли, завантажені з інтернету.
  Відкрийте властивості DLL чи `.asi` (а також Lunar Tear, Special K чи ASI Loader, якщо ставите їх): якщо
  внизу вкладки «Загальні» написано, що файл отримано з іншого комп'ютера, поставте позначку
  «Розблокувати» й натисніть OK. Простіше розблокувати сам архів ще до розпакування: тоді розблоковані
  будуть усі файли з нього.
- **Антивірус блокує чи видаляє DLL.** Плагіни, Lunar Tear, Special K та ASI Loader — бібліотеки (DLL), які
  гра завантажує в себе, тож антивірус може помилково вважати їх підозрілими. Додайте теку гри до винятків
  і поверніть файли з карантину. Завантажуйте все лише з офіційних сторінок.
- **Гра оновилася або Steam перевірив файли.** Перевірка файлів ці моди не прибирає, бо вони не замінюють
  файлів гри. Якщо оновлення змінить код гри, плагіни нічого не чіпатимуть і напишуть у лог `skipped`; тоді
  потрібна нова версія модів.
- **Ім'я героя вводьте латиницею.** Кирилицю гра не показує: якщо набрати ім'я українською розкладкою, на
  екрані з'являться зовсім інші символи.
- **Гра гальмує з NierReplicantGFX.** Найважчий для відеокарти суперсемплінг: поставте `RenderScale = 1.0`
  (за замовчуванням) або менше значення, наприклад 1.5.
- **Затінення здається надто темним чи світлим.** Налаштуйте `Strength` у `NierReplicantGFX.ini` під себе:
  більше значення — темніші тіні, менше — світліші. Якщо вам більше до вподоби оригінальне затінення гри,
  поставте `Shader = 0`.

## Як це працює

NierReplicantUA (адреси відносно початку exe):

| Група | Адреси | Що змінюється |
|---|---|---|
| `pool` | `0xD736A`, `0xD6E84`, `0xD6F0D`, `0xD7A75`, `0xD79CA` | сховище `text_common` на 1 МБ замість 256 КБ (`VirtualAlloc`); лише поки менеджер тексту (`0x27E0590`) порожній |
| `fishing` | `0x3A7D01`, `0x3A80D9` | см і кг для англійського слота: діапазон мов 3–5 → 2–5 |
| `pickup` | виклики `snprintf` у `0xC17B0` | назва предмета пишеться відразу в 128-байтовий буфер повідомлення |
| `talker` | `0xD68F3` | завантажувач `talker_name.tnd` не обрізає імена до 31 байта |
| `words` | `0xB90E08` | таблиця порядку Word Edit `0xB90DF0` (три int32 на індекс мови: 0 — слово A, 1 — слово B, 2 — назва): рядок англійської (індекс 2) стає «назва, A, B», як у французької, італійської та іспанської. Копію таблиці `0xC11230`, з якої екран вибору слів бере вибір колонки, списки слів і колір назви, плагін не змінює. Вимикається `NameFirst = 0` у `NierReplicantUA.ini` |
| `index` | `0x8ED281` | функція `0x8ED1F0` підключає теку архівів (основні дані — з порожньою назвою теки, DLC — `dlc\dlc01\`) і відкриває в ній `info.arc`; для основних даних вставка підставляє `info_uk.arc`, якщо він є, а DLC читає свій `info.arc`. Рядки гри не змінюються |

Колонки екрана вибору слів переставлено в розмітці `white_book_word_separate_word.bxon` (`ui/widget_param`)
в `ua.arc`: три дочірні віджети прив'язані до якорів `word_topic_txt_01`–`03`, і переклад зсуває ці
прив'язки на одну позицію (01 → 02, 02 → 03, 03 → 01), тож колонка назви опиняється ліворуч.

NierReplicantGFX перехоплює `D3D11CreateDevice` в імпорті гри, а на створеному пристрої — створення
текстур, подань (views) і шейдерів:

- шейдери гри він упізнає за контрольною сумою DXBC. Шейдер AO `gen_ssao_mask_default_c` замінюється
  на [shaders/ssao_mask.hlsl](shaders/ssao_mask.hlsl), інші отримують вставки інструкцій із
  `shader_edits.h` (опис кожної правки є в `tools/gen_shader_edits.py`). У `shader_edits.h` лише
  контрольні суми, зсуви й вставлені інструкції, коду гри там немає;
- 10-бітні текстури R10G10B10A2 без початкових даних створюються як R16G16B16A16, а подання до них
  отримують відповідний формат;
- для суперсемплінгу в `0x7D425E` вставлено виклик, що множить внутрішню роздільність на
  `RenderScale`. Фінальний прохід центрує кадр за внутрішньою роздільністю, яку читає викликом у
  `0x8B3163`; цей виклик повертає роздільність до множення, тож кадр стоїть там само, де й у грі без
  суперсемплінгу. NierReplicantFix шукає свої виправлення співвідношення сторін за шаблонами байтів у
  `0x7D4226`–`0x7D4243` і `0x8B31EB`–`0x8B320E`; плагін ці байти не змінює, тож порядок завантаження
  неважливий.

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
