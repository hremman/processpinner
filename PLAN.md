# План доработки doRelease.py

## Цель

Доработать скрипт `doRelease.py` до полноценного комбайна сборки/деплоя/упаковки,
заменяющего `buildWin.bat` + `deployWin.bat`, и провести реструктуризацию кода.

## Затронутые файлы

- `doRelease.py` — основной и единственный изменяемый файл
- `buildWin.bat` — старый скрипт сборки (будет удалён)
- `deployWin.bat` — старый скрипт деплоя (будет удалён)

---

## Фаза 1: Исправление дефектов ВЫПОЛНЕНО

### 1.1 Баг: config.MAKE получает значение qmake
**Файл**: `doRelease.py`  
**Проблема**: В блоке `if not PLATFORM == WINDOWS` при поиске `make` через `which` значение записывается в `config.QMAKE` вместо `config.MAKE`.  
**Исправление**: Заменить `config.QMAKE = result.stdout.strip()` на `config.MAKE = result.stdout.strip()`

### 1.2 bool(value) принимает любую непустую строку как True
**Файл**: `doRelease.py`  
**Проблема**: `bool("False")` и `bool("0")` дают `True` в Python.  
**Исправление**: Заменить на `value.lower() in ('true', '1', 'yes')`

### 1.3 KNOWN_PLATFORMS не включает Linux
**Файл**: `doRelease.py`  
**Проблема**: `KNOWN_PLATFORMS = [WINDOWS]` — Linux и macOS считаются неподдерживаемыми.  
**Исправление**: Добавить `LINUX` и `MACOS` в `KNOWN_PLATFORMS`

### 1.4 CONFIGURE_CMD, BUILD_CMD не содержат записей для Linux/macOS
**Файл**: `doRelease.py`  
**Проблема**: Словари `CONFIGURE_CMD`, `BUILD_CMD` имеют ключ только `WINDOWS`.  
**Исправление**: Добавить ключи `LINUX` и `MACOS` с подходящими значениями:
```python
CONFIGURE_CMD = {
    WINDOWS: ['CONFIG-=debug_and_release', 'CONFIG-=debug', 'CONFIG+=release', PROJ_PATH],
    LINUX:    ['CONFIG-=debug_and_release', 'CONFIG-=debug', 'CONFIG+=release', PROJ_PATH],
    MACOS:    ['CONFIG-=debug_and_release', 'CONFIG-=debug', 'CONFIG+=release', PROJ_PATH],
}
BUILD_CMD = {
    WINDOWS: ['-j'],
    LINUX:    ['-j'],
    MACOS:    ['-j'],
}
```

---

## Фаза 2: Реструктуризация

### 2.1 Убрать неиспользуемый импорт ✅ ВЫПОЛНЕНО
**Файл**: `doRelease.py`  
**Действие**: Удалить `import shutil`
**Статус**: Выполнено — `shutil` нигде не используется, строка удалена.

### 2.2 Убрать глобальные переменные-словари ✅ ВЫПОЛНЕНО
**Файл**: `doRelease.py`  
**Проблема**: `CONFIGURE_CMD`, `BUILD_CMD`, `DEPLOY_CMD`, `PACKAGE_CMD` объявлены как `global` в `parse_config()`, но они не определены до вызова `parse_config()` — это работает только потому, что `parse_config()` вызывается в `__main__` и словарь создаётся в том же блоке.  
**Действие**: Вынести определение словарей в модульный уровень (после `PE_ELF`), убрать `global`-декларации
**Статус**: Выполнено — словари уже были на модульном уровне, убраны `global CONFIGURE_CMD`, `global BUILD_CMD`, `global DEPLOY_CMD`, `global PACKAGE_CMD` из `parse_config()`.

### 2.3 Привести порядок функций
**Файл**: `doRelease.py`  
**Действие**: Перестроить порядок:
1. Imports
2. Constants & Config
3. Localization (`tr`, `trs`)
4. Utility functions (`eprint`, `critical`, `run_sub`, `mkdir`)
5. Platform detection & cmd dicts
6. `getGitLog()`
7. Version functions (`getVersion`, `incrementVersion`, `writeVersionToProject`)
8. `build_changelog()`
9. `parse_config()`
10. `build()`
11. `deploy()` (заглушка)
12. `package()` (заглушка)
13. `__main__`

### 2.4 Добавить type hints
**Файл**: `doRelease.py`  
**Действие**: Добавить аннотации типов где не хватает:
- `Config.__init__` — не нужен (self)
- `getGitLog() -> tuple[list[str], list[str], list[str], bool]`
- `incrementVersion(old: str, fix: list[str], add: list[str], change: list[str], release: bool) -> str | None`
- `writeVersionToProject(version: str | None) -> None`
- `build_changelog(fix: list[str], add: list[str], change: list[str]) -> str`
- `parse_config() -> None`
- `build() -> None`

---

## Фаза 3: Доработка функционала

### 3.1 Реализовать deploy()
**Файл**: `doRelease.py`  
**Аналог**: `deployWin.bat`  
**Действие**: 
```python
def deploy():
    """Деплой через CQtDeployer (аналог deployWin.bat)."""
    # Определить путь к бинарнику в зависимости от платформы
    # Запустить CQtDeployer
    # Удалить translations из результата
    # Скопировать changelog в deploy-dir
```

### 3.2 Реализовать package()
**Файл**: `doRelease.py`  
**Аналог**: `deployWin.bat` (вторая часть)  
**Действие**:
```python
def package():
    """Упаковка в 7z (аналог deployWin.bat)."""
    # Прочитать VERSION из .pro
    # Запустить 7z a <archive>.7z <deploy-dir>
    # Удалить временные папки
```

### 3.3 Добавить флаг --no-deploy
**Файл**: `doRelease.py`  
**Требование**: Собрать приложение без деплоя.  
**Действие**:
- Добавить аргумент `-nd / --no-deploy`
- Добавить `NO_DEPLOY = False` в `Config`
- В `__main__`: если `NO_DEPLOY` → пропустить `deploy()` и `package()`

### 3.4 Удалить bat-скрипты
**Файлы**: `buildWin.bat`, `deployWin.bat`  
**Действие**: Удалить, так как заменены `doRelease.py`

---

## План тестирования

### Синтаксическая проверка
```bash
python3 -c "import py_compile; py_compile.compile('doRelease.py', doraise=True)"
```

### Фаза 1: дефекты
```bash
# Проверка бага make
grep -n 'config.MAKE = result' doRelease.py  # должно быть только config.MAKE

# Проверка булевого парсинга
# Создать config-файл с ONLY_BUILD=0 и убедиться, что флаг не ставится

# Проверка Linux
python3 doRelease.py --only-build  # не должно падать с "Unsupported platform"
```

### Фаза 2: реструктуризация
```bash
# Проверка что код не сломался
python3 doRelease.py --help
python3 doRelease.py --only-build  # должен работать как раньше
```

### Фаза 3: функционал
```bash
# Только сборка (требование 2)
python3 doRelease.py --no-deploy

# Полный комбайн (требование 1)
python3 doRelease.py

# Только changelog
python3 doRelease.py --changelog-only
```
