#!/usr/bin/env python3
"""
doRelease.py — автоматизированный скрипт сборки, деплоя и упаковки Process Pinner.
Заменяет ручные bat-скрипты (buildWin.bat, deployWin.bat).

Поток выполнения:
  1. Парсинг конфигурации (CLI-аргументы + опциональный config-файл)
  2. Анализ git-логов с автоинкрементом версии (по префиксам коммитов: f:, a:, c:, !)
  3. Обновление версии в ProcessPinner.pro и файла changelog
  4. Сборка (qmake + make)
  5. Деплой (CQtDeployer) — TODO
  6. Упаковка (7-Zip) — TODO

Формат коммитов для авто-версионирования:
  f: message  — fix (инкрементирует патч)
  a: message  — add/new feature (инкрементирует минор, сбрасывает патч)
  c: message  — change (инкрементирует минор, сбрасывает патч)
  !message    — release-флаг (инкрементирует мажор)
  Можно комбинировать: !f: fixed something
"""

import os
import platform
import shutil
import sys
import argparse
import subprocess
import shlex
import locale
from datetime import date


# ─── Глобальные пути ───────────────────────────────────────────────────────────
# START_DIR — откуда был запущен скрипт (для возврата в конце)
# PROJ_DIR — каталог, где лежит сам doRelease.py (корень проекта)
# PROJ_PATH — путь к .pro-файлу Qt
START_DIR = os.path.abspath(os.path.curdir)
PROJ_DIR = os.path.abspath(os.path.split(__file__)[0])
PROJ_PATH = os.path.join(PROJ_DIR, "ProcessPinner.pro")


# ─── Конфигурация ──────────────────────────────────────────────────────────────
class Config:
    """Хранит все параметры сборки: пути, каталоги, флаги."""
    def __init__(self):
        self.BUILD_DIR = os.path.join(os.path.curdir, 'build')
        self.DEPLOY_DIR = os.path.join(os.path.curdir, 'deploy')
        self.QMAKE = "qmake"
        self.MAKE = "make"
        # Число потоков make: cpu_count - 1 (но не менее 1)
        self.MAKE_THREADS = (os.cpu_count() - 1)
        if self.MAKE_THREADS <= 0:
            self.MAKE_THREADS = 1
        # ONLY_BUILD — только собрать, без деплоя/упаковки
        self.ONLY_BUILD = False
        # NO_PACKAGE — собрать и сделать деплой, но не упаковывать в архив
        self.NO_PACKAGE = False
        # CHANGELOG — показать changelog и выйти, не собирая
        self.CHANGELOG = False

    def __str__(self):
        return '\n'.join([f'BUILD_DIR={self.BUILD_DIR}',
            f'DEPLOY_DIR={self.DEPLOY_DIR}',
            f'QMAKE={self.QMAKE}',
            f'MAKE={self.MAKE}',
            f'MAKE_THREADS={self.MAKE_THREADS}',
            f'ONLY_BUILD={self.ONLY_BUILD}',
            f'NO_PACKAGE={self.NO_PACKAGE}']
        )


config = Config()
IS_DEBUG = True


# ─── Локализация ───────────────────────────────────────────────────────────────
# Определяем язык из локали ОС (по умолчанию 'en').
# Словарь trs содержит пары сообщений (оригинал → перевод).
lang = locale.getlocale()[0].split('_')[0].lower() if locale.getlocale() else 'en'
lang = {
    'russian': 'ru',
    'ru': 'ru'
}.get(lang, 'en')
trs = {
    "ru": {
        'Util qmake not found. Set manually': 'Не найдена утилита qmake, укажите вручную',
        'Util make not found. Set manually': 'Не найдена утилита make, укажите вручную',
        'Current platform: ': 'Платформа: ',
        'Unsupported platfom: ': 'Неподдерживаемая платформа: ',
        ' failed!': ' неуспешно!',
        "Configure qmake build": 'Конфигурирование qmake сборки',
        "Make build": 'Сборка make',
        'Built executable: ': 'Собран исполняемый файл: ',
        "Leave dir ": 'Выход из директории ',
        "Get commits": 'Получение коммитов',
        'Process Pinner builder script': 'Скрипт сборки Process Pinner',
        'Path to qmake': 'Расположение qmake',
        'Path to make': 'Расположение make',
        'make threads (default: ': 'число потоков в make (по умолчанию: ',
        'build dir (default: ': 'каталог сборки (по умолчанию: ',
        'dir where will be placed deployed or packaged result (default: ':
            'каталог для расположения деплоя или готового пакета (по умолчанию: ',
        'config file': 'конфигурационный файл',
        'just build': 'только собрать',
        'do build, do deploy, no packaging': 'собрать, подготовить к упаковке, но не упаковывать',
        'show changelog only. Work only with git-branche':
            'показать только changelog. Работает только в git-ветке',
        "Path ": 'Путь ',
        " not exists!": ' не существует!',
        "Get last tag name": "Получение имени последнего тега",
        "Version changed ": 'Версия изменена ',
        "No significant changes": 'Нет значимых изменений',
    }
}


def tr(msg: str) -> str:
    """Перевести строку на текущий язык (или вернуть оригинал, если перевода нет)."""
    return trs.get(lang, 'en').get(msg, msg)


def eprint(*args, **kwargs):
    """Вывести сообщение в stderr."""
    print(*args, **kwargs, file=sys.stderr)


def critical(msg: str):
    """Вывести критическую ошибку в stderr, вернуться в исходный каталог и завершиться."""
    os.chdir(START_DIR)
    eprint(msg)
    exit(-1)


# ─── Запуск подпроцессов ───────────────────────────────────────────────────────
def run_sub(cmd: list, msg: str, text: bool = True, crit: bool = True):
    """
    Запустить внешнюю команду.
    msg — описание действия (используется в логах и переводах).
    crit — если True, при ошибке завершает работу скрипта.
    """
    print(f"**{msg}**")
    if IS_DEBUG:
        print(shlex.join(cmd))
    result = subprocess.run(cmd, capture_output=True, text=text)

    if result.returncode != 0:
        eprint(result.stdout)
        eprint(result.stderr)
        if crit:
            critical(msg + tr(' failed!'))
        else:
            eprint(msg + tr(' failed!'))
    elif IS_DEBUG:
        if text:
            print(result.stdout)
        else:
            print(result.stdout.decode())
    return result


# ─── Определение платформы ─────────────────────────────────────────────────────
# KNOWN_PLATFORMS — список поддерживаемых платформ.
# Сейчас только Windows; Linux/macOS частично заготовлены.
WINDOWS = 'windows'
LINUX = 'linux'
MACOS = 'darwin'
KNOWN_PLATFORMS = [WINDOWS, LINUX, MACOS]
PLATFORM = platform.system().lower()

if PLATFORM not in KNOWN_PLATFORMS:
    eprint(tr('Unsupported platfom: ') + PLATFORM)
    exit(-1)
else:
    print(tr('Current platform: ') + PLATFORM)


# ─── Автопоиск qmake и make (только для не-Windows) ────────────────────────────
# На Windows пути задаются вручную через аргументы или config-файл.
# На Linux/macOS пытаемся найти через 'which'.
if not PLATFORM == WINDOWS:
    result = run_sub(['which', 'qmake'])
    if result.returncode == 0:
        config.QMAKE = result.stdout.strip()
    else:
        critical(tr('Util qmake not found. Set manually'))

    result = run_sub(['which', 'make'])
    if result.returncode == 0:
        config.MAKE = result.stdout.strip()  # Было: config.QMAKE — опечатка
    else:
        critical(tr('Util make not found. Set manually'))


# ─── Команды для разных платформ ────────────────────────────────────────────────
# CONFIGURE_CMD — аргументы qmake (CONFIG-=debug CONFIG+=release и т.д.)
# BUILD_CMD — аргументы make (например, -j для параллельной сборки)
# DEPLOY_CMD / PACKAGE_CMD — заготовки для будущих функций деплоя и упаковки
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
DEPLOY_CMD = {}
PACKAGE_CMD = {}

# Имена выходных бинарников для каждой платформы
PE_ELF = {
    WINDOWS: 'ProcessPinner.exe',
    LINUX:    'ProcessPinner',
    MACOS:    'ProcessPinner',
}


def mkdir(path: str):
    """Создать каталог, если он ещё не существует."""
    if not os.path.exists(path):
        os.mkdir(path)


# ─── Сборка ─────────────────────────────────────────────────────────────────────
def build():
    """
    Выполнить qmake → make в каталоге сборки.
    Аналог buildWin.bat.
    """
    conf_cmd = [config.QMAKE] + CONFIGURE_CMD.get(PLATFORM)
    build_cmd = [config.MAKE] + BUILD_CMD.get(PLATFORM) + [str(config.MAKE_THREADS)]

    print(f"Go into {config.BUILD_DIR}")
    mkdir(config.BUILD_DIR)
    os.chdir(config.BUILD_DIR)
    run_sub(conf_cmd, tr("Configure qmake build"))
    run_sub(build_cmd, tr("Make build"))
    print(tr('Built executable: ') + os.path.join(config.BUILD_DIR, PE_ELF[PLATFORM]))
    print(tr("Leave dir ") + config.BUILD_DIR)
    os.chdir(START_DIR)


# ─── Анализ git-логов ───────────────────────────────────────────────────────────
def getGitLog():
    """
    Получить список коммитов от последнего тега до HEAD.
    Парсит префиксы коммитов для авто-версионирования:
      f: → исправления (fix)
      a: → новые фичи (add)
      c: → изменения (change)
      ! → флаг релиза (инкремент мажорной версии)
    Возвращает: (fix_list, add_list, change_list, is_release)
    """
    # Получаем имя последнего тега
    result = run_sub(['git', 'describe', '--tags', '--abbrev=0'],
                     tr("Get last tag name"), False)
    last_tag = result.stdout.decode().strip()

    # Получаем логи коммитов от последнего тега до HEAD
    result = run_sub([
        'git', 'log',
        f'{last_tag}..HEAD',
        '--oneline',
        '--no-merges',
        '--pretty=format:"%s"'
    ], tr("Get commits"), False)

    fix = []
    add = []
    change = []
    release = False
    messages = result.stdout.decode('utf-8').split('\n')

    for commit in messages:
        maby_release = False
        # Префикс '!' — флаг релиза (инкремент мажорной версии)
        if commit.startswith('!'):
            commit = commit[1:]
            maby_release = True
        # Префикс 'f:' — исправление (fix)
        if commit.strip().startswith('f:'):
            _, _, msg = commit.partition('f:')
            fix.append(msg.strip())
            release = maby_release
        # Префикс 'c:' — изменение (change)
        if commit.strip().startswith('c:'):
            _, _, msg = commit.partition('c:')
            change.append(msg.strip())
            release = maby_release
        # Префикс 'a:' — новая фича (add)
        if commit.strip().startswith('a:'):
            _, _, msg = commit.partition('a:')
            add.append(msg.strip())
            release = maby_release

    return (fix, add, change, release)


# ─── Работа с версией ───────────────────────────────────────────────────────────
def getVersion() -> str:
    """
    Прочитать текущую версию из файла ProcessPinner.pro.
    Возвращает строку вида "X.Y.Z".
    """
    version = "0.0.0"
    with open(PROJ_PATH, 'r') as proj:
        for line in proj.read().split('\n'):
            if line.strip().startswith('VERSION = '):
                version = line.split(' = ')[-1].strip('"')
    return version


def incrementVersion(old: str, fix: list, add: list, change: list, release: bool) -> str:
    """
    Вычислить новую версию на основе типа коммитов.
    Правила (семантическое версионирование):
      fix    → X.Y.Z+1  (инкремент патча)
      add/c  → X.Y+1.0  (инкремент минора, сброс патча)
      release → X+1.0.0 (инкремент мажора, сброс минора и патча)
    Если версия не изменилась, возвращает None.
    """
    old_major, old_minor, old_patch = [int(x) for x in old.split('.')]
    major, minor, patch = old_major, old_minor, old_patch

    if len(fix) > 0:
        patch += 1
    if len(add) > 0 or len(change) > 0:
        minor += 1
        patch = 0
    if release:
        major += 1
        minor = 0
        patch = 0

    if old_major == major and old_minor == minor and old_patch == patch:
        return None
    return f'{major}.{minor}.{patch}'


def writeVersionToProject(version: str):
    """Записать новую версию в ProcessPinner.pro, заменив строку 'VERSION = ...'."""
    if version is not None:
        proj_content = None
        with open(PROJ_PATH, 'r') as proj:
            proj_content = proj.readlines()
        old_version = ""
        for i in range(len(proj_content)):
            if proj_content[i].strip().startswith("VERSION = "):
                old_version = proj_content[i].split(' = ')[1].strip().strip('"')
                proj_content[i] = f'VERSION = "{version}"\n'
                break
        with open(PROJ_PATH, 'w') as proj:
            proj.writelines(proj_content)
            print(tr("Version changed ") + f'{old_version} -> {version}')


def build_changelog(fix: list, add: list, change: list) -> str:
    """
    Сформировать текст changelog на русском по списку коммитов.
    Порядок секций: Изменено → Добавлено → Исправлено.
    """
    text = ""
    if len(change) > 0:
        text = "Изменено:\n"
        for line in change:
            text = f'{text} - {line}\n'
    if len(add) > 0:
        text += "Добавлено:\n"
        for line in add:
            text = f'{text} - {line}\n'
    if len(fix) > 0:
        text += "Исправлено:\n"
        for line in fix:
            text = f'{text} - {line}\n'
    return text


# ─── Парсинг конфигурации ──────────────────────────────────────────────────────
def parse_config():
    """
    Загрузить конфигурацию из:
      1. CLI-аргументов (--qmake-path, --make-path, --threads, --build-dir, --deploy-dir и т.д.)
      2. Опционального config-файла (ключ=значение, строки с # игнорируются)
    CLI-аргументы переопределяют config-файл.
    """
    global config
    global CONFIGURE_CMD
    global BUILD_CMD
    global DEPLOY_CMD
    global PACKAGE_CMD

    parser = argparse.ArgumentParser(description=tr('Process Pinner builder script'))
    parser.add_argument('-q', '--qmake-path', help=tr('Path to qmake'))
    parser.add_argument('-m', '--make-path', help=tr('Path to make'))
    parser.add_argument('-j', '--threads', type=int, default=config.MAKE_THREADS,
                        help=tr('make threads (default: ') + str(config.MAKE_THREADS) + ')')
    parser.add_argument('-b', '--build-dir', default=config.BUILD_DIR,
                        help=tr('build dir (default: ') + config.BUILD_DIR + ')')
    parser.add_argument('-d', '--deploy-dir', default=config.DEPLOY_DIR,
                        help=tr('dir where will be placed deployed or packaged result (default: ')
                             + config.DEPLOY_DIR + ')')
    parser.add_argument('-c', '--config', help=tr('config file'))
    parser.add_argument('-o', '--only-build', action='store_true',
                        help=tr('just build'))
    parser.add_argument('-n', '--no-package', action='store_true',
                        help=tr('do build, do deploy, no packaging'))
    parser.add_argument('-l', '--changelog-only', action='store_true',
                        help=tr('show changelog only'))

    args = parser.parse_args()

    # ── Загрузка из config-файла ────────────────────────────────────────────────
    if args.config is not None:
        if not os.path.exists(args.config):
            critical(tr("Path ") + args.config + tr(" not exists!"))
        with open(args.config, 'r') as cf:
            for line in cf.read().split('\n'):
                if line.strip().startswith('#'):
                    continue
                pair = line.split('=', maxsplit=1)
                if len(pair) != 2:
                    continue
                key = pair[0].strip().upper()
                value = pair[1].split('#')[0].strip()
                if key == 'BUILD_DIR':
                    config.BUILD_DIR = value
                elif key == 'DEPLOY_DIR':
                    config.DEPLOY_DIR = value
                elif key == 'QMAKE':
                    config.QMAKE = value
                elif key == 'MAKE':
                    config.MAKE = value
                elif key == 'MAKE_THREADS':
                    config.MAKE_THREADS = int(value)
                elif key == 'ONLY_BUILD':
                    config.ONLY_BUILD = value.lower() in ('true', '1', 'yes')
                elif key == 'NO_PACKAGE':
                    config.NO_PACKAGE = value.lower() in ('true', '1', 'yes')
                elif key == 'CHANGELOG':
                    config.CHANGELOG = value.lower() in ('true', '1', 'yes')

    # ── Переопределение из CLI-аргументов (приоритет выше) ──────────────────────
    if args.qmake_path is not None:
        config.QMAKE = args.qmake_path
    if args.make_path is not None:
        config.MAKE = args.make_path
    if args.threads is not None:
        config.MAKE_THREADS = args.threads
    if args.build_dir is not None:
        config.BUILD_DIR = args.build_dir
    if args.deploy_dir is not None:
        config.DEPLOY_DIR = args.deploy_dir
    if args.only_build is not None:
        config.ONLY_BUILD = args.only_build
    if args.no_package is not None:
        config.NO_PACKAGE = args.no_package
    if args.changelog_only is not None and not config.ONLY_BUILD:
        config.CHANGELOG = args.changelog_only

    # ── Нормализация путей ──────────────────────────────────────────────────────
    config.BUILD_DIR = os.path.abspath(config.BUILD_DIR)
    config.DEPLOY_DIR = os.path.abspath(config.DEPLOY_DIR)
    config.QMAKE = os.path.abspath(config.QMAKE)
    config.MAKE = os.path.abspath(config.MAKE)

    # ── Проверка существования путей ────────────────────────────────────────────
    if not os.path.exists(config.QMAKE):
        critical(tr("Path ") + config.QMAKE + tr(" not exists!"))
    if not os.path.exists(config.MAKE):
        critical(tr("Path ") + config.MAKE + tr(" not exists!"))


# ─── Точка входа ─────────────────────────────────────────────────────────────────
if __name__ == '__main__':
    # Шаг 0: загрузка конфигурации
    parse_config()
    print(config)
    print(f'CUR_DIR={START_DIR}')
    print(f'PROJ_DIR={PROJ_DIR}')
    print()

    # Шаг 1: анализ git-логов и автоинкремент версии
    # (пропускается при --only-build)
    if not config.ONLY_BUILD:
        fix, add, change, release = getGitLog()
        changelog = build_changelog(fix, add, change).strip()
        version = getVersion()
        newVer = incrementVersion(version, fix, add, change, release)

        if changelog and newVer:
            print(tr("****\nChangelog:"))
            print(changelog)

            # --changelog-only: показать и выйти
            if config.CHANGELOG:
                exit()

            # Обновить файл changelog: новая запись в начало, старое — после разделителя
            cl_path = os.path.join(PROJ_DIR, "changelog")
            with open(cl_path, 'r', encoding='utf-8') as cl_file:
                cl_content = cl_file.read()
            with open(cl_path, 'w', encoding='utf-8') as cl_file:
                cl_file.write(f'{newVer}({str(date.today())}):\n')
                cl_file.write(changelog)
                if cl_content:
                    cl_file.write('\n\n'
                                  '/**********************************************************************************************************/\n')
                    cl_file.write(cl_content)

            # Записать новую версию в ProcessPinner.pro
            writeVersionToProject(newVer)
            version = newVer
        else:
            print(tr("No significant changes"))

    # Шаг 2: сборка (qmake → make)
    build()

    # --only-build: выйти после сборки
    if config.ONLY_BUILD:
        os.chdir(START_DIR)
        exit()

    # Шаг 3: деплой (CQtDeployer) — TODO: реализовать
    deploy()

    # --no-package: выйти после деплоя, без упаковки
    if config.NO_PACKAGE:
        os.chdir(START_DIR)
        exit()

    # Шаг 4: упаковка (7-Zip) — TODO: реализовать
    package()

    os.chdir(START_DIR)
