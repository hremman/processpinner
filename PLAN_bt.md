# План: build.py — единый скрипт сборки, деплоя и упаковки

## 0. Контекст и цели

**Задача**: Создать единый Python-скрипт `build.py`, заменяющий `buildWin.bat` + `deployWin.bat` и дорабатывающий `doRelease.py`.

**Целевые платформы**:
| Платформа | Компилятор | Формат дистрибутива | Требования к Qt |
|-----------|------------|---------------------|-----------------|
| Windows 10 | MSVC / GCC (MinGW) | 7z-архив | Qt не нужен на целевой машине |
| Linux (Fedora 43) | GCC | AppImage | Qt не нужен на целевой машине |
| macOS | Clang | `.app` бандл + `.dmg` | Qt не нужен на целевой машине |

**Ключевые требования**:
- Один файл `build.py`, без внешних зависимостей (только stdlib)
- `pathlib` для работы с путями
- Конфигурация: CLI-аргументы + опциональный config-файл (`key=value`)
- Локализованный вывод (ru/en), раскрашенные логи для WARN/ERROR
- Без `main()` — скрипт не импортируется
- Остановка на нужном этапе: `--stage build | deploy | package`
- Для деплоя: CQtDeployer + ручная чистка переводов (оставить только ru/en)
- Отсутствие Qt-окружения на машине сборки = критическая ошибка

---

## 1. Архитектура скрипта

### 1.1 Структура модуля (порядок блоков в файле)

```
build.py
├── Imports (stdlib only)
├── Constants (платформы, цвета, пути)
├── Config class + parse_config()
├── Localization (trs словари + tr()/trs() функции)
├── Logging (цветной вывод, уровни)
├── Utility functions (mkdir, run_cmd, etc.)
├── Platform detection & toolchain resolution
├── Version management (getVersion, incrementVersion, writeVersionToProject)
├── Changelog builder (build_changelog)
├── Git log analysis (getGitLog)
├── Build phase (qmake + make/ninja/msbuild)
├── Deploy phase (CQtDeployer + manual translation cleanup)
├── Package phase (7z / AppImage / macOS bundle)
└── __main__ entry point (последовательный вызов фаз)
```

### 1.2 Фазы выполнения

```
[1] Parse config (CLI + optional file)
[2] Detect platform & toolchain
[3] Check prerequisites (qmake, compiler, CQtDeployer, etc.)
[4] Git log analysis + version increment
[5] Update changelog file + ProcessPinner.pro
[6] BUILD: qmake → compiler
[7] DEPLOY: CQtDeployer → cleanup translations
[8] PACKAGE: compress to target format
```

---

## 2. Конфигурация

### 2.1 CLI-аргументы

| Аргумент | Краткий | Описание | Значение по умолчанию |
|----------|---------|----------|----------------------|
| `--qmake-path` | `-q` | Путь к qmake | автопоиск |
| `--make-tool` | `-m` | Инструмент сборки (make/ninja/msbuild) | автоопределение |
| `--make-args` | | Аргументы для параллельной сборки | `-j` (число потоков = cpu_count-1) |
| `--build-dir` | `-b` | Каталог сборки | `build` |
| `--deploy-dir` | `-d` | Каталог для деплоя | `deploy` |
| `--config` | `-c` | Путь к config-файлу | нет |
| `--stage` | `-s` | Остановиться после фазы | `None` = собрать полный дистрибутив (build→deploy→package) |
| `--only-build` | `-o` | Только сборка | `False` |
| `--no-package` | `-n` | Без упаковки | `False` |
| `--changelog-only` | `-l` | Только показать changelog | `False` |
| `--verbose` | `-v` | Подробный вывод | `False` |
| `--lang` | | Язык вывода (`ru`/`en`) | автоопределение из локали ОС |

### 2.2 Config-файл (key=value, строки с `#` — комментарии)

```
QMAKE=/opt/qt6/bin/qmake
MAKE_TOOL=make
MAKE_THREADS=4
BUILD_DIR=build
DEPLOY_DIR=deploy
ONLY_BUILD=false
NO_PACKAGE=false
CHANGELOG=false
```

**Приоритет**: CLI-аргументы > config-файл > значения по умолчанию.

### 2.3 Config class

```python
class Config:
    QMAKE: Path
    MAKE_TOOL: str          # 'make', 'ninja', 'msbuild'
    MAKE_THREADS: int
    BUILD_DIR: Path
    DEPLOY_DIR: Path
    ONLY_BUILD: bool
    NO_PACKAGE: bool
    CHANGELOG: bool
    PLATFORM: str           # 'windows', 'linux', 'darwin'
    COMPILER: str           # 'msvc', 'gcc', 'clang'
    STAGE: str              # 'build', 'deploy', 'package'
    VERBOSE: bool
    LANG: str               # 'ru', 'en'
```

---

## 3. Локализация и логирование

### 3.1 Словарь переводов `trs`

Структура аналогична `doRelease.py`:

```python
trs = {
    "ru": {
        "message key": "русский перевод",
        ...
    },
    "en": {
        "message key": "english text",
        ...
    }
}

def tr(msg: str) -> str:
    """Перевести строку на текущий язык."""
    return trs.get(lang, {}).get(msg, msg)
```

**Все строки, выводимые пользователю**, проходят через `tr()`.

### 3.2 Цветной логгер

Уровни: `DEBUG` (без цвета), `INFO` (без цвета), `WARN` (жёлтый), `ERROR` (красный), `SUCCESS` (зелёный).

```python
class Logger:
    COLORS = {
        'RESET': '\033[0m',
        'BOLD':  '\033[1m',
        'GREEN': '\033[92m',
        'YELLOW': '\033[93m',
        'RED':   '\033[91m',
        'CYAN':  '\033[96m',
    }
    
    def __init__(self, lang: str, verbose: bool = False):
        self.lang = lang
        self.verbose = verbose
        self.prefix = trs.get(lang, trs['en'])  # префиксы по уровню
    
    def _colorize(self, level: str, msg: str) -> str:
        """Вернуть раскрашенную строку."""
        color = self.COLORS.get(level, '')
        return f"{color}{self._level_prefix(level)}: {msg}{self.COLORS['RESET']}"
    
    def info(self, msg: str): ...
    def warn(self, msg: str): ...
    def error(self, msg: str): ...
    def success(self, msg: str): ...
    def debug(self, msg: str): ...
```

**Детали**:
- Цвета работают в терминалах, поддерживающих ANSI (Windows 10+ — по умолчанию с v1511+; для старых — fallback на `color` command через `ctypes`)
- На Windows без поддержки ANSI — использовать `ctypes.SetConsoleTextAttribute`
- Префиксы: `✓` / `⚠` / `✗` / `ℹ` (или текстовые: `OK`, `WARN`, `ERROR`, `INFO`)

---

## 4. Определение платформы и инструментации

### 4.1 Платформа

```python
PLATFORM = platform.system().lower()
# 'windows', 'linux', 'darwin'
```

### 4.2 Компилятор

| Платформа | Как определить |
|-----------|---------------|
| Windows | `cl` в PATH → `msvc`; `g++` в PATH → `gcc`; `clang-cl` → `clang` |
| Linux | `gcc` в PATH → `gcc`; `clang` в PATH → `clang` |
| macOS | `clang` в PATH → `clang` (на macOS clang по умолчанию) |

### 4.3 Инструмент сборки

| Платформа | Компилятор | Инструмент |
|-----------|------------|------------|
| Windows | MSVC | `nmake` или `jom` |
| Windows | GCC/MinGW | `mingw32-make` или `ninja` |
| Linux | GCC/Clang | `make` или `ninja` |
| macOS | Clang | `make` или `ninja` |

**Автоопределение**:
1. Проверить `ninja` в PATH → использовать ninja
2. Иначе для MinGW/GCC → `make` / `mingw32-make`
3. Иначе для MSVC → `nmake` / `jom`
4. Иначе для Clang (macOS) → `make`

### 4.4 Проверка обязательных зависимостей

| Зависимость | Windows | Linux | macOS |
|-------------|---------|-------|-------|
| qmake | ✓ | ✓ | ✓ |
| Компилятор | ✓ | ✓ | ✓ |
| CQtDeployer | ✓ | ✓ | ✓ |
| 7-Zip | ✓ | — | — |
| mkbundle (macdeployqt) | — | — | ✓ |
| linuxdeploy (AppImage) | — | ✓ | — |

**Если чего-то нет → критическая ошибка**. Скрипт прекращает работу с сообщением на текущем языке.

---

## 5. Работа с версией и changelog

### 5.1 Чтение версии

Из `ProcessPinner.pro`:
```
VERSION = "1.2.3"
```

### 5.2 Автоинкремент (берём из doRelease.py)

Префиксы коммитов (от последнего тега до HEAD):

| Префикс | Действие | Инкремент |
|---------|----------|-----------|
| `!` | Флаг релиза | мажор +1, сброс минора и патча |
| `f:` | Исправление | патч +1 |
| `c:` | Изменение | минор +1, патч → 0 |
| `a:` | Новая фича | минор +1, патч → 0 |
| (без префикса) | Без изменений | версия не меняется |

Можно комбинировать: `!f: fixed something`

### 5.3 Обновление changelog

Файл `changelog` в корне проекта. Формат **сохраняется существующий** (без изменений структуры):

```
1.4.0(2026-10-26):
Изменено:
 - item 1
 - item 2

Добавлено:
 - item 3

Исправлено:
 - item 4


/**********************************************************************************************************/
1.3.0(2026-10-20):
...
```

**Правила обновления**:
- Новая запись пишется в начало файла
- Старое содержимое — после разделителя `/**********************************************************************************************************/`
- Формат (отступы, разделители, порядок секций) — **без изменений**

---

## 6. Фаза сборки (BUILD)

### 6.1 Конфигурация qmake

```
CONFIG-=debug_and_release
CONFIG-=debug
CONFIG+=release
```

### 6.2 Компиляция

| Платформа | Команда |
|-----------|---------|
| Windows (MinGW) | `mingw32-make -j N` |
| Windows (MSVC) | `nmake` (через vcvarsall.bat) |
| Linux | `make -j N` или `ninja` |
| macOS | `make -j N` или `ninja` |

### 6.3 Результат сборки

| Платформа | Бинарник |
|-----------|----------|
| Windows | `ProcessPinner.exe` |
| Linux | `ProcessPinner` |
| macOS | `ProcessPinner.app` |

---

## 7. Фаза деплоя (DEPLOY)

### 7.1 CQtDeployer

Вызов:
```
cqtdeployer -bin <путь к бинарнику> \
             -qmake <путь к qmake> \
             -targetDir <deploy-dir>/<platform> \
             -extraPlugins <если нужно> \
             -bundles <если нужно>
```

### 7.2 Платформенно-специфичные параметры

**Windows**:
```
cqtdeployer -bin ProcessPinner.exe \
             -qmake <qmake_path> \
             -targetDir deploy/windows \
             -noCopyQtDbus   # не нужен на Windows
```

**Linux**:
```
cqtdeployer -bin ProcessPinner \
             -qmake <qmake_path> \
             -targetDir deploy/linux \
             -appimage        # включить генерацию AppImage
```

**macOS**:
```
cqtdeployer -bin ProcessPinner \
             -qmake <qmake_path> \
             -targetDir deploy/macos \
             -macBundle       # создать .app бандл
             -macDeviceType   # если нужен универсальный бандл
```

### 7.3 Ручная чистка переводов

После CQtDeployer в `deploy/<platform>/translations/` оказываются все `.qm` файлы, включая qtbase с другими языками.

**Логика определения нужных переводов**:
1. **en** (`qtbase_en.qm`) — включается всегда по умолчанию
2. **ru и другие** — только те, что есть в `.ts` файлах проекта (например `ProcessPinner_ru_RU.ts` в `resouces/`)
3. Собрать список необходимых `qtbase_*.qm` файлов
4. Удалить все остальные `.qm` файлы из `deploy/<platform>/translations/`
5. Удалить каталог `translations/` целиком (если не нужен) — аналог `del /Q /F .\ProcessPinner\translations` из `deployWin.bat`

### 7.4 Копирование changelog

```
copy changelog → deploy/<platform>/changelog.txt
```

---

## 8. Фаза упаковки (PACKAGE)

### 8.1 Windows — 7z-архив

```
7z a ProcessPinner_<version>_windows.7z deploy/windows/
```

**Детали**:
- Имя архива: `ProcessPinner_<MAJOR.MINOR.PATCH>_windows.7z`
- Включить: бинарник, DLL, ресурсы, changelog
- Не включать: `deploy/windows/translations/` (уже удалены)

### 8.2 Linux — AppImage

CQtDeployer с флагом `-appimage` генерирует `.AppImage` автоматически.

**Дополнительно**:
- Переименовать: `ProcessPinner_<version>_linux_x86_64.AppImage`
- Проверить права выполнения: `chmod +x`
- (Опционально) подпись: `linuxdeploy-plugin-appimage --sign`

### 8.3 macOS — бандл + DMG

CQtDeployer с флагом `-macBundle` создаёт `.app`.

**Дополнительно**:
- Использовать `macdeployqt` для финализации бандла:
  ```
  macdeployqt ProcessPinner.app -dmg
  ```
- Переименовать: `ProcessPinner_<version>_macos.dmg`
- (Опционально) кодовая подпись:
  ```
  codesign --deep --sign <identity> ProcessPinner.app
  notarize ...  # если нужно для Gatekeeper
  ```

---

## 9. Обработка ошибок и проверки

### 9.1 Критические ошибки (остановка скрипта)

- Не найдена платформа
- Не найден qmake
- Не найден компилятор
- Не найден CQtDeployer
- Ошибка выполнения qmake / компиляции
- Ошибка выполнения CQtDeployer

### 9.2 Предупреждения (продолжаем, но логируем)

- Не найден 7-Zip (Windows) / linuxdeploy (Linux) / macdeployqt (macOS) — предупреждение, но продолжаем
- Не найден git — предупреждение, пропускаем авто-версионирование
- Не найден последний тег — предупреждение, используем текущую версию из `.pro`

### 9.3 Фолбэки

| Зависимость | Фолбэк |
|-------------|--------|
| 7-Zip | zip (если 7z не найден) |
| ninja | make |
| mkbundle | macdeployqt |

---

## 10. Поток выполнения (псевдокод)

```
# ── Инициализация ──
config = parse_config()
logger = Logger(config.lang, config.verbose)
platform = detect_platform()
compiler = detect_compiler(platform)
toolchain = resolve_toolchain(platform, compiler)

# ── Проверка зависимостей ──
check_required_tools(config, platform, compiler)

# ── Версия и changelog ──
if not config.only_build:
    fix, add, change, release = get_git_log()
    version = get_version()
    new_ver = increment_version(version, fix, add, change, release)
    
    if changelog_text and new_ver:
        logger.info(tr("Changelog:"))
        logger.info(changelog_text)
        
        if config.changelog_only:
            exit()
        
        update_changelog_file(new_ver, changelog_text)
        write_version_to_project(new_ver)
        version = new_ver
    else:
        logger.info(tr("No significant changes"))

# ── Фаза сборки ──
build(config, platform, compiler, toolchain)
if config.stage == 'build':
    exit()

# ── Фаза деплоя ──
deploy(config, platform)
cleanup_translations(config)
copy_changelog(config)
if config.stage == 'deploy':
    exit()

# ── Фаза упаковки ──
package(config, platform, version)
if config.stage == 'package':
    exit()
```

---

## 11. Имена выходных файлов

| Платформа | Формат | Пример имени |
|-----------|--------|-------------|
| Windows | `.7z` | `ProcessPinner_1.4.0_windows.7z` |
| Linux | `.AppImage` | `ProcessPinner_1.4.0_linux_x86_64.AppImage` |
| macOS | `.dmg` | `ProcessPinner_1.4.0_macos.dmg` |

Расположение: `deploy/` (корень проекта).

---

## 12. Файлы, которые будут затронуты

| Файл | Действие |
|------|----------|
| `build.py` | **Создать** — новый единый скрипт |
| `buildWin.bat` | Заменить / пометить как устаревший |
| `deployWin.bat` | Заменить / пометить как устаревший |
| `doRelease.py` | Заменить / пометить как устаревший |
| `PLAN_build_tool.md` | Этот файл |

---

## 13. Порядок реализации (этапы)

### Этап 1: Скелет скрипта
- [ ] Imports, constants
- [ ] Config class + parse_config()
- [ ] Localization (trs словари + tr())
- [ ] Logger (цветной, локализованный)
- [ ] Platform detection

### Этап 2: Проверка зависимостей
- [ ] Автопоиск qmake
- [ ] Автопоиск компилятора
- [ ] Проверка CQtDeployer
- [ ] Проверка пакетов упаковки (7z / linuxdeploy / macdeployqt)

### Этап 3: Версия и changelog
- [ ] getGitLog() (анализ префиксов)
- [ ] getVersion() / incrementVersion() / writeVersionToProject()
- [ ] build_changelog()
- [ ] Обновление changelog-файла

### Этап 4: Сборка
- [ ] qmake конфигурация
- [ ] Компиляция (make/ninja/msbuild)
- [ ] Вывод пути к бинарнику

### Этап 5: Деплой
- [ ] CQtDeployer вызов (платформенно-специфичный)
- [ ] Чистка переводов (en по умолчанию + языки из .ts файлов проекта)
- [ ] Копирование changelog

### Этап 6: Упаковка
- [ ] Windows: 7z архив
- [ ] Linux: AppImage (из результата CQtDeployer)
- [ ] macOS: DMG из бандла

### Этап 7: Финализация
- [ ] Тестирование на Linux (текущая машина)
- [ ] Тестирование на Windows (коллега / CI)
- [ ] Тестирование на macOS (коллега)
- [ ] Удаление старых bat-скриптов
