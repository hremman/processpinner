#!/usr/bin/env python3
"""
build.py — единый скрипт сборки, деплоя и упаковки ProcessPinner.

Заменяет buildWin.bat + deployWin.bat и дорабатывает doRelease.py.
Поддерживаемые платформы: Windows (MSVC/GCC), Linux (GCC), macOS (Clang).

Платформы:
  Windows 10  — MSVC / MinGW → 7z-архив
  Linux         — GCC         → AppImage
  macOS         — Clang       → .app bundle + .dmg

Требования: только стандартная библиотека Python.
"""

# ──────────────────────────────────────────────────────────────
# Imports (stdlib only)
# ──────────────────────────────────────────────────────────────
import argparse
import ctypes
import datetime
import glob
import json
import os
import platform
import re
import shutil
import subprocess
import sys
from enum import IntEnum
from pathlib import Path
from typing import List, Optional, Tuple

# ──────────────────────────────────────────────────────────────
# Constants
# ──────────────────────────────────────────────────────────────

# Supported platforms
PLATFORM_WINDOWS = "windows"
PLATFORM_LINUX = "linux"
PLATFORM_DARWIN = "darwin"

# Compiler names
COMPILER_MSCV = "msvc"
COMPILER_GCC = "gcc"
COMPILER_CLANG = "clang"

# Color codes (ANSI)
COLORS = {
    "RESET": "\033[0m",
    "BOLD": "\033[1m",
    "GREEN": "\033[92m",
    "YELLOW": "\033[93m",
    "RED": "\033[91m",
    "CYAN": "\033[96m",
    "WHITE": "\033[97m",
}

# Log level names for colorization
LOG_LEVELS = {
    "DEBUG": "",
    "INFO": "",
    "WARN": "YELLOW",
    "ERROR": "RED",
    "SUCCESS": "GREEN",
    "HEADER": "CYAN",
    "BOLD": "BOLD",
}

# Project root (directory containing this script)
SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR

# Qt project file
PRO_FILE = PROJECT_ROOT / "ProcessPinner.pro"

# Changelog file
CHANGELOG_FILE = PROJECT_ROOT / "changelog"

# Separator used in changelog
CHANGELOG_SEPARATOR = "/**********************************************************************************************************/"

# Translation source directory
TRANSLATIONS_DIR = PROJECT_ROOT / "resouces" / "translations"

# ──────────────────────────────────────────────────────────────
# Localization (trs dictionaries + tr() function)
# ──────────────────────────────────────────────────────────────

trs: dict = {
    "ru": {
        # Header
        "BUILD_SCRIPT": "Скрипт сборки ProcessPinner",
        "VERSION_INFO": "Версия {version}",
        "PLATFORM_INFO": "Платформа: {platform}",

        # Config
        "CONFIG_LOADED": "Конфигурация загружена из {path}",
        "CONFIG_NOT_FOUND": "Файл конфигурации не найден: {path}",

        # Platform detection
        "DETECTED_PLATFORM": "Обнаружена платформа: {platform}",
        "DETECTED_COMPILER": "Обнаружен компилятор: {compiler}",
        "DETECTED_TOOLCHAIN": "Определён инструмент сборки: {tool}",
        "PLATFORM_UNSUPPORTED": "Неподдерживаемая платформа: {platform}",

        # Dependencies
        "CHECKING_DEPS": "Проверка зависимостей...",
        "DEPS_OK": "Все зависимости найдены",
        "MISSING_TOOL": "Не найден обязательный инструмент: {tool}",
        "MISSING_PACKAGING_TOOL": "Не найден инструмент упаковки: {tool} ({hint})",
        "TRYING_FALLBACK": "Попытка использовать {fallback} вместо {tool}",
        "FALLBACK_NOT_FOUND": "Не найден fallback: {fallback}",
        "CQTDEPLOYER_HINT": "Установите cqtdeployer: pip install cqtdeployer или скачайте с https://github.com/QuasarApp/CQtDeployer",
        "7ZIP_HINT": "Установите 7-Zip с https://www.7-zip.org/",
        "LINUXDEPLOY_HINT": "Установите linuxdeploy: https://github.com/linuxdeploy/linuxdeploy",
        "MACDEPLOYQT_HINT": "Установите Qt для macOS; macdeployqt входит в комплект Qt",
        "PACKAGING_WARN": "Инструмент упаковки не найден — предупреждение, продолжение работы",
        "CQTDEPLOYER_WARN": "CQtDeployer не найден — не удастся автоматически собрать дистрибутив",

        # Version
        "VERSION_CURRENT": "Текущая версия: {version}",
        "VERSION_NEW": "Новая версия: {new_version}",
        "VERSION_INCREMENTS": "Инкремент: {increment_type}",
        "NO_GIT": "Git не найден, пропускаем авто-версионирование",
        "NO_TAGS": "Теги не найдены, используем текущую версию",
        "NO_SIGNIFICANT_CHANGES": "Нет значимых изменений, версия не меняется",

        # Changelog
        "CHANGELOG_TITLE": "Changelog:",
        "CHANGELOG_SAVED": "Changelog сохранён в {path}",
        "CHANGELOG_ONLY": "Показан changelog (флаг --changelog-only)",

        # Build
        "BUILD_START": "Начало сборки...",
        "BUILD_CONFIGURING": "Конфигурация qmake...",
        "BUILD_COMPILING": "Компиляция ({tool})...",
        "BUILD_SUCCESS": "Сборка завершена успешно",
        "BUILD_OUTPUT": "Результат сборки: {path}",
        "BUILD_FAILED": "Ошибка сборки: {error}",
        "QMAKE_FAILED": "qmake завершился с ошибкой",
        "MAKE_FAILED": "Компилятор завершился с ошибкой",

        # Deploy
        "DEPLOY_START": "Начало деплоя...",
        "DEPLOY_CQT": "Вызов CQtDeployer...",
        "DEPLOY_CLEAN_TRANSLATIONS": "Чистка переводов...",
        "DEPLOY_COPY_CHANGELOG": "Копирование changelog...",
        "DEPLOY_SUCCESS": "Деплой завершён успешно",
        "DEPLOY_OUTPUT": "Результат деплоя: {path}",
        "DEPLOY_FAILED": "Ошибка деплоя: {error}",
        "CQTDEPLOYER_MISSING": "CQtDeployer не найден",

        # Package
        "PACKAGE_START": "Начало упаковки...",
        "PACKAGE_7Z": "Создание 7z-архива...",
        "PACKAGE_APPIMAGE": "Генерация AppImage...",
        "PACKAGE_DMG": "Создание DMG...",
        "PACKAGE_SUCCESS": "Упаковка завершена: {path}",
        "PACKAGE_FAILED": "Ошибка упаковки: {error}",
        "7ZIP_MISSING": "7-Zip не найден, архив не создан",
        "LINUXDEPLOY_MISSING": "linuxdeploy не найден",
        "MACDEPLOYQT_MISSING": "macdeployqt не найден",

        # Stage
        "STAGE_BUILD": "Остановка после фазы сборки",
        "STAGE_DEPLOY": "Остановка после фазы деплоя",
        "STAGE_PACKAGE": "Остановка после фазы упаковки",

        # General
        "ERROR_ABORT": "Критическая ошибка, скрипт завершён",
        "USAGE_HINT": "Используйте --help для справки",
        "PROGRESS": "Выполнено",
        "WARNING": "Предупреждение",
        "ERROR": "Ошибка",
        "INFO": "Информация",
        "OK": "OK",

        # Help texts for argparse
        "HELP_DESCRIPTION": "Скрипт сборки ProcessPinner",
        "HELP_QMAKE_PATH": "Путь к qmake",
        "HELP_MAKE_TOOL": "Инструмент сборки (make/ninja/msbuild/auto)",
        "HELP_MAKE_ARGS": "Аргументы для параллельной сборки",
        "HELP_BUILD_DIR": "Директория сборки",
        "HELP_DEPLOY_DIR": "Директория деплоя",
        "HELP_CONFIG": "Путь к файлу конфигурации",
        "HELP_STAGE": "Остановиться после этапа (build/deploy/package)",
        "HELP_ONLY_BUILD": "Только сборка",
        "HELP_NO_PACKAGE": "Без упаковки",
        "HELP_CHANGELOG_ONLY": "Только показать changelog",
        "HELP_VERBOSE": "Подробный вывод",
        "HELP_LANG": "Язык вывода (ru/en/auto)",
        "HELP_PLATFORM": "Принудительная платформа (windows/linux/darwin)",
        "HELP_COMPILER": "Принудительный компилятор (msvc/gcc/clang)",
    },
    "en": {
        # Header
        "BUILD_SCRIPT": "ProcessPinner Build Script",
        "VERSION_INFO": "Version {version}",
        "PLATFORM_INFO": "Platform: {platform}",

        # Config
        "CONFIG_LOADED": "Configuration loaded from {path}",
        "CONFIG_NOT_FOUND": "Configuration file not found: {path}",

        # Platform detection
        "DETECTED_PLATFORM": "Detected platform: {platform}",
        "DETECTED_COMPILER": "Detected compiler: {compiler}",
        "DETECTED_TOOLCHAIN": "Resolved build tool: {tool}",
        "PLATFORM_UNSUPPORTED": "Unsupported platform: {platform}",

        # Dependencies
        "CHECKING_DEPS": "Checking dependencies...",
        "DEPS_OK": "All dependencies found",
        "MISSING_TOOL": "Required tool not found: {tool}",
        "MISSING_PACKAGING_TOOL": "Packaging tool not found: {tool} ({hint})",
        "TRYING_FALLBACK": "Trying {fallback} as fallback for {tool}",
        "FALLBACK_NOT_FOUND": "Fallback not found: {fallback}",
        "CQTDEPLOYER_HINT": "Install cqtdeployer: pip install cqtdeployer or download from https://github.com/QuasarApp/CQtDeployer",
        "7ZIP_HINT": "Install 7-Zip from https://www.7-zip.org/",
        "LINUXDEPLOY_HINT": "Install linuxdeploy: https://github.com/linuxdeploy/linuxdeploy",
        "MACDEPLOYQT_HINT": "Install Qt for macOS; macdeployqt comes with Qt",
        "PACKAGING_WARN": "Packaging tool not found — warning, continuing",
        "CQTDEPLOYER_WARN": "CQtDeployer not found — unable to build a distribution automatically",

        # Version
        "VERSION_CURRENT": "Current version: {version}",
        "VERSION_NEW": "New version: {new_version}",
        "VERSION_INCREMENTS": "Increment: {increment_type}",
        "NO_GIT": "Git not found, skipping auto-versioning",
        "NO_TAGS": "No tags found, using current version",
        "NO_SIGNIFICANT_CHANGES": "No significant changes, version unchanged",

        # Changelog
        "CHANGELOG_TITLE": "Changelog:",
        "CHANGELOG_SAVED": "Changelog saved to {path}",
        "CHANGELOG_ONLY": "Changelog displayed (--changelog-only flag)",

        # Build
        "BUILD_START": "Starting build...",
        "BUILD_CONFIGURING": "Configuring qmake...",
        "BUILD_COMPILING": "Compiling ({tool})...",
        "BUILD_SUCCESS": "Build completed successfully",
        "BUILD_OUTPUT": "Build output: {path}",
        "BUILD_FAILED": "Build error: {error}",
        "QMAKE_FAILED": "qmake failed",
        "MAKE_FAILED": "Compiler failed",

        # Deploy
        "DEPLOY_START": "Starting deploy...",
        "DEPLOY_CQT": "Invoking CQtDeployer...",
        "DEPLOY_CLEAN_TRANSLATIONS": "Cleaning translations...",
        "DEPLOY_COPY_CHANGELOG": "Copying changelog...",
        "DEPLOY_SUCCESS": "Deploy completed successfully",
        "DEPLOY_OUTPUT": "Deploy output: {path}",
        "DEPLOY_FAILED": "Deploy error: {error}",
        "CQTDEPLOYER_MISSING": "CQtDeployer not found",

        # Package
        "PACKAGE_START": "Starting packaging...",
        "PACKAGE_7Z": "Creating 7z archive...",
        "PACKAGE_APPIMAGE": "Generating AppImage...",
        "PACKAGE_DMG": "Creating DMG...",
        "PACKAGE_SUCCESS": "Packaging complete: {path}",
        "PACKAGE_FAILED": "Packaging error: {error}",
        "7ZIP_MISSING": "7-Zip not found, archive not created",
        "LINUXDEPLOY_MISSING": "linuxdeploy not found",
        "MACDEPLOYQT_MISSING": "macdeployqt not found",

        # Stage
        "STAGE_BUILD": "Stopping after build phase",
        "STAGE_DEPLOY": "Stopping after deploy phase",
        "STAGE_PACKAGE": "Stopping after package phase",

        # General
        "ERROR_ABORT": "Critical error, script aborted",
        "USAGE_HINT": "Use --help for usage information",
        "PROGRESS": "Done",
        "WARNING": "Warning",
        "ERROR": "Error",
        "INFO": "Info",
        "OK": "OK",

        # Help texts for argparse
        "HELP_DESCRIPTION": "ProcessPinner Build Script",
        "HELP_QMAKE_PATH": "Path to qmake",
        "HELP_MAKE_TOOL": "Build tool (make/ninja/msbuild/auto)",
        "HELP_MAKE_ARGS": "Arguments for parallel build",
        "HELP_BUILD_DIR": "Build directory",
        "HELP_DEPLOY_DIR": "Deploy directory",
        "HELP_CONFIG": "Path to config file",
        "HELP_STAGE": "Stop after stage (build/deploy/package)",
        "HELP_ONLY_BUILD": "Only build",
        "HELP_NO_PACKAGE": "No packaging",
        "HELP_CHANGELOG_ONLY": "Only show changelog",
        "HELP_VERBOSE": "Verbose output",
        "HELP_LANG": "Output language (ru/en/auto)",
        "HELP_PLATFORM": "Force platform (windows/linux/darwin)",
        "HELP_COMPILER": "Force compiler (msvc/gcc/clang)",
    },
}


HELP_MAP = {
    "description": "HELP_DESCRIPTION",
    "qmake_path": "HELP_QMAKE_PATH",
    "make_tool": "HELP_MAKE_TOOL",
    "make_args": "HELP_MAKE_ARGS",
    "build_dir": "HELP_BUILD_DIR",
    "deploy_dir": "HELP_DEPLOY_DIR",
    "config": "HELP_CONFIG",
    "stage": "HELP_STAGE",
    "only_build": "HELP_ONLY_BUILD",
    "no_package": "HELP_NO_PACKAGE",
    "changelog_only": "HELP_CHANGELOG_ONLY",
    "verbose": "HELP_VERBOSE",
    "lang": "HELP_LANG",
    "platform": "HELP_PLATFORM",
    "compiler": "HELP_COMPILER",
}


def tr(msg: str, **kwargs) -> str:
    """Translate a message string to the current language.

    Falls back to English, then returns the original string.
    Supports simple format-style substitutions: tr("key", value=x) → "key: {value}".
    """
    lang = getattr(tr, "_lang", "en")
    lang_dict = trs.get(lang, trs.get("en", {}))
    text = lang_dict.get(msg, trs.get("en", {}).get(msg, msg))
    if kwargs:
        try:
            text = text.format(**kwargs)
        except (KeyError, IndexError, ValueError):
            pass
    return text


def trs_get(key: str) -> str:
    """Get a raw translation string for the current language (no formatting)."""
    lang = getattr(tr, "_lang", "en")
    return trs.get(lang, trs.get("en", {})).get(key, key)


# ──────────────────────────────────────────────────────────────
# Logger (colored, localized)
# ──────────────────────────────────────────────────────────────


class _Logger:
    """Colored, localized logger."""

    def __init__(self, lang: str, verbose: bool = False):
        self.lang = lang
        self.verbose = verbose
        self._use_ctypes = False

        # Detect if ANSI colors are supported
        if platform.system().lower() == PLATFORM_WINDOWS:
            try:
                # Windows 10 v1511+ supports ANSI by default
                handle = ctypes.windll.kernel32.GetStdHandle(-11)
                mode = ctypes.c_ulong()
                if ctypes.windll.kernel32.GetConsoleMode(handle, ctypes.byref(mode)):
                    self._use_ctypes = True
            except Exception:
                self._use_ctypes = True
        else:
            # Assume ANSI support on Unix-like systems
            self._use_ctypes = False

    def _colorize(self, level: str, msg: str) -> str:
        """Return a colorized string for the given log level."""
        if not self._use_ctypes:
            color_name = LOG_LEVELS.get(level, "")
            color = COLORS.get(color_name, "")
            if color:
                return f"{color}{msg}{COLORS['RESET']}"
            return msg

        # Windows ctypes coloring
        color_map = {
            "YELLOW": 6,   # FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_RED
            "RED": 4,      # FOREGROUND_RED
            "GREEN": 2,    # FOREGROUND_GREEN
            "CYAN": 3,     # FOREGROUND_BLUE | FOREGROUND_GREEN
            "WHITE": 7,    # FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
            "BOLD": 15,    # FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
        }
        color_code = color_map.get(color_name, 7)
        handle = ctypes.windll.kernel32.GetStdHandle(-11)
        ctypes.windll.kernel32.SetConsoleTextAttribute(handle, color_code)
        return msg

    def _level_prefix(self, level: str) -> str:
        """Get the localized prefix for a log level."""
        prefix_map = {
            "DEBUG": "D",
            "INFO": "I",
            "WARN": "!",
            "ERROR": "!",
            "SUCCESS": "+",
            "HEADER": "=",
            "BOLD": "*",
        }
        return prefix_map.get(level, "?")

    def _log(self, level: str, msg: str, **kwargs):
        """Internal log method."""
        translated = tr(msg, **kwargs)
        prefix = f"[{self._level_prefix(level)}]"

        if level == "HEADER" or level == "BOLD":
            print(f"{translated}")
        elif level == "SUCCESS":
            print(f"{COLORS['GREEN']}{prefix} {translated}{COLORS['RESET']}")
        elif level == "WARN":
            print(f"{COLORS['YELLOW']}{prefix} {translated}{COLORS['RESET']}")
        elif level == "ERROR":
            print(f"{COLORS['RED']}{prefix} {translated}{COLORS['RESET']}", file=sys.stderr)
        elif level == "DEBUG":
            if self.verbose:
                print(f"{prefix} {translated}")
        else:
            print(f"{prefix} {translated}")

    def info(self, msg: str, **kwargs):
        self._log("INFO", msg, **kwargs)

    def warn(self, msg: str, **kwargs):
        self._log("WARN", msg, **kwargs)

    def error(self, msg: str, **kwargs):
        self._log("ERROR", msg, **kwargs)

    def success(self, msg: str, **kwargs):
        self._log("SUCCESS", msg, **kwargs)

    def debug(self, msg: str, **kwargs):
        self._log("DEBUG", msg, **kwargs)

    def header(self, msg: str, **kwargs):
        self._log("HEADER", msg, **kwargs)

    def bold(self, msg: str, **kwargs):
        self._log("BOLD", msg, **kwargs)


# Global logger instance (set by _main)
_logger: Optional[_Logger] = None


def get_logger() -> _Logger:
    """Get the global logger instance."""
    return _logger


# ──────────────────────────────────────────────────────────────
# Utility functions
# ──────────────────────────────────────────────────────────────


def mkdir(path: Path) -> Path:
    """Create directory (and parents) if it doesn't exist."""
    path.mkdir(parents=True, exist_ok=True)
    return path


def rmdir(path: Path) -> None:
    """Recursively remove a directory if it exists."""
    if path.exists():
        shutil.rmtree(path)


def run_cmd(
    cmd: List[str],
    cwd: Optional[Path] = None,
    verbose: bool = False,
    capture: bool = False,
) -> subprocess.CompletedProcess:
    """Run a command and optionally capture output.

    Args:
        cmd: Command and arguments.
        cwd: Working directory.
        verbose: If True, print the command being run.
        capture: If True, capture stdout and stderr.

    Returns:
        CompletedProcess instance.
    """
    if verbose:
        logger = get_logger()
        if logger:
            logger.debug(f"Running: {' '.join(cmd)}")

    try:
        result = subprocess.run(
            cmd,
            cwd=str(cwd) if cwd else None,
            capture_output=capture,
            text=True,
            check=False,
        )
        return result
    except FileNotFoundError as e:
        result = subprocess.CompletedProcess(args=cmd, returncode=127, stdout="", stderr=str(e))
        return result
    except OSError as e:
        result = subprocess.CompletedProcess(args=cmd, returncode=1, stdout="", stderr=str(e))
        return result


def which(tool: str) -> Optional[Path]:
    """Find an executable in PATH, return its Path or None."""
    return shutil.which(tool)


# ──────────────────────────────────────────────────────────────
# Config class + parse_config()
# ──────────────────────────────────────────────────────────────


class Config:
    """Holds all configuration for the build script."""

    def __init__(self):
        # Paths
        self.qmake_path: Optional[Path] = None
        self.build_dir: Path = PROJECT_ROOT / "build"
        self.deploy_dir: Path = PROJECT_ROOT / "deploy"

        # Build tool
        self.make_tool: str = "auto"  # 'make', 'ninja', 'msbuild', 'auto'
        self.make_threads: int = 0  # 0 = auto (cpu_count - 1)
        self.make_args: str = ""

        # Compiler
        self.compiler: str = "auto"  # 'msvc', 'gcc', 'clang', 'auto'

        # Platform override
        self.platform: str = "auto"  # 'windows', 'linux', 'darwin', 'auto'

        # Behavior
        self.stage: Optional[str] = None  # 'build', 'deploy', 'package'
        self.only_build: bool = False
        self.no_package: bool = False
        self.changelog_only: bool = False

        # Output
        self.verbose: bool = False
        self.lang: str = "auto"  # 'ru', 'en', 'auto'


def _parse_config_file(config_path: Path) -> dict:
    """Parse a key=value config file, ignoring comments and blank lines."""
    result = {}
    try:
        with open(config_path, "r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("#"):
                    continue
                if "=" not in line:
                    continue
                key, _, value = line.partition("=")
                key = key.strip().upper()
                value = value.strip()
                result[key] = value
    except OSError:
        pass
    return result


def _resolve_lang(argv: Optional[List[str]]) -> str:
    """Resolve the output language from argv before any tr() is available."""
    # First, try to read --lang from argv
    source = argv if argv is not None else sys.argv[1:]
    for i, arg in enumerate(source):
        if arg in ("--lang", "-L") and i + 1 < len(source):
            val = source[i + 1]
            if val in ("ru", "en"):
                return val
        if arg.startswith("--lang="):
            val = arg.split("=", 1)[1]
            if val in ("ru", "en"):
                return val
    # Auto-detect from locale
    locale = os.environ.get("LANG", "")
    if locale.lower().startswith("ru"):
        return "ru"
    return "en"


def parse_config(argv: Optional[List[str]] = None) -> Config:
    """Parse configuration from CLI arguments and optional config file.

    Priority: CLI > config file > defaults.
    """
    config = Config()

    # Resolve language early so tr() works for help texts
    lang = _resolve_lang(argv)
    tr._lang = lang  # type: ignore[attr-defined]

    # Build help texts via tr()
    help_keys = [
        "qmake_path", "make_tool", "make_args", "build_dir",
        "deploy_dir", "config", "stage", "only_build",
        "no_package", "changelog_only", "verbose",
        "lang", "platform", "compiler",
    ]
    help_texts = {k: tr(trs_get(HELP_MAP[k])) for k in help_keys}
    description = tr(trs_get("HELP_DESCRIPTION"))

    # Parse config file first (lower priority)
    parser = argparse.ArgumentParser(
        description=description,
        add_help=True,
    )
    parser.add_argument("--qmake-path", "-q", type=str, default=None, help=help_texts["qmake_path"])
    parser.add_argument("--make-tool", "-m", type=str, default="auto", help=help_texts["make_tool"])
    parser.add_argument("--make-args", type=str, default="", help=help_texts["make_args"])
    parser.add_argument("--build-dir", "-b", type=str, default=None, help=help_texts["build_dir"])
    parser.add_argument("--deploy-dir", "-d", type=str, default=None, help=help_texts["deploy_dir"])
    parser.add_argument("--config", "-c", type=str, default=None, help=help_texts["config"])
    parser.add_argument("--stage", "-s", type=str, default=None, help=help_texts["stage"])
    parser.add_argument("--only-build", "-o", action="store_true", default=False, help=help_texts["only_build"])
    parser.add_argument("--no-package", "-n", action="store_true", default=False, help=help_texts["no_package"])
    parser.add_argument("--changelog-only", "-l", action="store_true", default=False, help=help_texts["changelog_only"])
    parser.add_argument("--verbose", "-v", action="store_true", default=False, help=help_texts["verbose"])
    parser.add_argument("--lang", type=str, default="auto", help=help_texts["lang"])
    parser.add_argument("--platform", type=str, default="auto", help=help_texts["platform"])
    parser.add_argument("--compiler", type=str, default="auto", help=help_texts["compiler"])

    args = parser.parse_args(argv)

    # Load config file
    if args.config:
        config_file_path = Path(args.config)
        file_vars = _parse_config_file(config_file_path)
        if file_vars:
            logger = get_logger()
            if logger:
                logger.info(tr("CONFIG_LOADED"), path=config_file_path)

        # Apply config file values (lower priority)
        if "QMAKE" in file_vars and args.qmake_path is None:
            config.qmake_path = Path(file_vars["QMAKE"])
        if "MAKE_TOOL" in file_vars:
            config.make_tool = file_vars["MAKE_TOOL"]
        if "MAKE_THREADS" in file_vars:
            config.make_threads = int(file_vars["MAKE_THREADS"])
        if "BUILD_DIR" in file_vars and args.build_dir is None:
            config.build_dir = Path(file_vars["BUILD_DIR"])
        if "DEPLOY_DIR" in file_vars and args.deploy_dir is None:
            config.deploy_dir = Path(file_vars["DEPLOY_DIR"])
        if "ONLY_BUILD" in file_vars:
            config.only_build = file_vars["ONLY_BUILD"].lower() in ("true", "1", "yes")
        if "NO_PACKAGE" in file_vars:
            config.no_package = file_vars["NO_PACKAGE"].lower() in ("true", "1", "yes")
        if "CHANGELOG" in file_vars:
            config.changelog_only = file_vars["CHANGELOG"].lower() in ("true", "1", "yes")
        if "PLATFORM" in file_vars and args.platform == "auto":
            config.platform = file_vars["PLATFORM"]
        if "COMPILER" in file_vars and args.compiler == "auto":
            config.compiler = file_vars["COMPILER"]

    # Apply CLI arguments (higher priority)
    if args.qmake_path:
        config.qmake_path = Path(args.qmake_path)
    if args.make_tool != "auto":
        config.make_tool = args.make_tool
    if args.build_dir:
        config.build_dir = Path(args.build_dir)
    if args.deploy_dir:
        config.deploy_dir = Path(args.deploy_dir)
    if args.stage:
        config.stage = args.stage
    if args.only_build:
        config.only_build = True
    if args.no_package:
        config.no_package = True
    if args.changelog_only:
        config.changelog_only = True
    if args.lang != "auto":
        config.lang = args.lang
    if args.platform != "auto":
        config.platform = args.platform
    if args.compiler != "auto":
        config.compiler = args.compiler

    # Set verbose from both
    if args.verbose:
        config.verbose = True

    return config


# ──────────────────────────────────────────────────────────────
# Platform detection & toolchain resolution
# ──────────────────────────────────────────────────────────────


def detect_platform(fallback: str = PLATFORM_LINUX) -> str:
    """Detect the current platform.

    Returns one of: 'windows', 'linux', 'darwin'.
    """
    forced = getattr(detect_platform, "_forced", None)
    if forced:
        return forced

    sys_name = platform.system().lower()
    if sys_name == PLATFORM_WINDOWS:
        return PLATFORM_WINDOWS
    elif sys_name == PLATFORM_LINUX:
        return PLATFORM_LINUX
    elif sys_name == PLATFORM_DARWIN:
        return PLATFORM_DARWIN
    else:
        logger = get_logger()
        if logger:
            logger.warn(tr("PLATFORM_UNSUPPORTED"), platform=sys_name)
        return fallback


def detect_compiler(platform: str) -> str:
    """Detect the active compiler for the given platform.

    Returns one of: 'msvc', 'gcc', 'clang'.
    """
    if platform == PLATFORM_WINDOWS:
        if which("cl"):
            return COMPILER_MSCV
        elif which("g++") or which("gcc"):
            return COMPILER_GCC
        elif which("clang-cl"):
            return COMPILER_CLANG
    elif platform == PLATFORM_LINUX:
        if which("gcc"):
            return COMPILER_GCC
        elif which("clang"):
            return COMPILER_CLANG
    elif platform == PLATFORM_DARWIN:
        if which("clang"):
            return COMPILER_CLANG

    # Default fallback
    if platform == PLATFORM_DARWIN:
        return COMPILER_CLANG
    elif platform == PLATFORM_LINUX:
        return COMPILER_GCC
    return COMPILER_MSCV


def resolve_toolchain(platform: str, compiler: str) -> dict:
    """Resolve the build toolchain for the given platform and compiler.

    Returns a dict with:
        - 'make_tool': name of the make tool
        - 'make_cmd': full command for the make tool
        - 'is_ninja': whether ninja is used
        - 'is_msvc': whether MSVC is the compiler
    """
    result = {
        "make_tool": "auto",
        "make_cmd": None,
        "is_ninja": False,
        "is_msvc": False,
        "is_gcc": False,
        "is_clang": False,
    }

    # Detect make tool
    if which("ninja"):
        result["make_tool"] = "ninja"
        result["make_cmd"] = "ninja"
        result["is_ninja"] = True
    elif platform == PLATFORM_WINDOWS and compiler == COMPILER_MSCV:
        if which("jom"):
            result["make_tool"] = "jom"
            result["make_cmd"] = "jom"
        elif which("nmake"):
            result["make_tool"] = "nmake"
            result["make_cmd"] = "nmake"
    elif platform == PLATFORM_WINDOWS and compiler == COMPILER_GCC:
        if which("mingw32-make"):
            result["make_tool"] = "mingw32-make"
            result["make_cmd"] = "mingw32-make"
        elif which("make"):
            result["make_tool"] = "make"
            result["make_cmd"] = "make"
    else:
        # Linux / macOS
        if which("make"):
            result["make_tool"] = "make"
            result["make_cmd"] = "make"
        elif which("gmake"):
            result["make_tool"] = "gmake"
            result["make_cmd"] = "gmake"

    if compiler == COMPILER_MSCV:
        result["is_msvc"] = True
    elif compiler == COMPILER_GCC:
        result["is_gcc"] = True
    elif compiler == COMPILER_CLANG:
        result["is_clang"] = True

    return result


def find_qmake() -> Optional[Path]:
    """Find qmake in PATH.

    Tries common Qt6 names first, then Qt5.
    """
    candidates = ["qmake", "qmake6", "qmake-qt6", "qmake5", "qmake-qt5"]
    for name in candidates:
        path = which(name)
        if path:
            return Path(path)

    # Try common install locations
    extra_paths = [
        "/opt/qt6/bin/qmake",
        "/opt/qt5/bin/qmake",
        "/usr/lib/qt6/bin/qmake",
        "/usr/lib/qt5/bin/qmake",
        "C:/Qt/6.x/msvc/bin/qmake.exe",
        "C:/Qt/6.x/mingw/bin/qmake.exe",
        "C:/Qt/5.x/msvc/bin/qmake.exe",
        "C:/Qt/5.x/mingw/bin/qmake.exe",
    ]
    for p in extra_paths:
        path = Path(p)
        if path.exists():
            return path

    return None


# ──────────────────────────────────────────────────────────────
# Dependency checking (Phase 2)
# ──────────────────────────────────────────────────────────────


def find_cqtdeployer() -> Optional[Path]:
    """Find CQtDeployer in PATH and common installation locations.

    Returns the Path to the executable or None if not found.
    """
    # Search PATH first
    path = shutil.which("cqtdeployer")
    if path:
        return Path(path)

    # Common installation locations (Windows)
    common_paths = [
        "C:/Program Files/CQtDeployer/1.6/CQtDeployer.exe",
        "C:/Program Files/CQtDeployer/1.7/CQtDeployer.exe",
        "C:/Program Files/CQtDeployer/1.8/CQtDeployer.exe",
        "C:/Program Files/CQtDeployer/1.9/CQtDeployer.exe",
        "C:/Program Files/CQtDeployer/1.10/CQtDeployer.exe",
        "C:/Qt/Tools/CQtDeployer/bin/cqtdeployer.exe",
    ]
    for p in common_paths:
        path = Path(p)
        if path.exists():
            return path

    return None


def find_7zip() -> Tuple[bool, Optional[Path]]:
    """Find 7-Zip in PATH and common installation locations.

    Returns:
        (found, path) — path is None if not found.
    """
    # Search PATH
    for name in ("7z", "7za", "7zip", "7z.exe"):
        path = shutil.which(name)
        if path:
            return (True, Path(path))

    # Common Windows installation locations
    common_paths = [
        "C:/Program Files/7-Zip/7z.exe",
        "C:/Program Files (x86)/7-Zip/7z.exe",
    ]
    for p in common_paths:
        path = Path(p)
        if path.exists():
            return (True, Path(p))

    return (False, None)


def find_linuxdeploy() -> Tuple[bool, Optional[Path]]:
    """Find linuxdeploy in PATH or common locations.

    Returns:
        (found, path) — path is None if not found.
    """
    path = shutil.which("linuxdeploy")
    if path:
        return (True, Path(path))

    # Common locations
    common_paths = [
        "/usr/bin/linuxdeploy",
        "/usr/local/bin/linuxdeploy",
        "/opt/linuxdeploy/linuxdeploy",
        Path.home() / ".linuxdeploy/linuxdeploy",
    ]
    for p in common_paths:
        path = Path(p)
        if path.exists():
            return (True, Path(p))

    return (False, None)


def find_macdeployqt() -> Tuple[bool, Optional[Path]]:
    """Find macdeployqt in PATH and common Qt installation locations.

    Returns:
        (found, path) — path is None if not found.
    """
    # Search PATH
    path = shutil.which("macdeployqt")
    if path:
        return (True, Path(path))

    # Common Qt installation locations on macOS
    home = Path.home()
    common_paths = [
        "/opt/qt6/bin/macdeployqt",
        "/opt/qt5/bin/macdeployqt",
        "/usr/local/qt6/bin/macdeployqt",
        "/usr/local/qt5/bin/macdeployqt",
        home / "Qt/6.x/macos/bin/macdeployqt",
        home / "Qt/5.x/macos/bin/macdeployqt",
        home / "Qt/6.7.0/macos/bin/macdeployqt",
        home / "Qt/6.6.3/macos/bin/macdeployqt",
    ]
    for p in common_paths:
        path = Path(p)
        if path.exists():
            return (True, Path(p))

    return (False, None)


def check_cqtdeployer() -> Tuple[bool, Optional[Path]]:
    """Check if CQtDeployer is available.

    Returns:
        (found, path) — path is None if not found.
    """
    path = find_cqtdeployer()
    return (path is not None, path)


def check_packaging_tools(platform: str) -> dict:
    """Check packaging tools availability for the given platform.

    Returns a dict with tool names as keys and (found, path) tuples as values.
    """
    result = {}

    if platform == PLATFORM_WINDOWS:
        found_7z, path_7z = find_7zip()
        result["7z"] = (found_7z, path_7z)

    elif platform == PLATFORM_LINUX:
        found_ld, path_ld = find_linuxdeploy()
        result["linuxdeploy"] = (found_ld, path_ld)

    elif platform == PLATFORM_DARWIN:
        found_mdqt, path_mdqt = find_macdeployqt()
        result["macdeployqt"] = (found_mdqt, path_mdqt)

    return result


# ──────────────────────────────────────────────────────────────
# Version management (placeholder — to be implemented in Phase 3)
# ──────────────────────────────────────────────────────────────


def get_version() -> str:
    """Read VERSION from the .pro file.

    TODO: Implement in Phase 3 — parse ProcessPinner.pro for
    `VERSION = \"X.Y.Z\"` and return the version string.
    """
    return "0.0.0"  # placeholder


# ──────────────────────────────────────────────────────────────
# _main() module-level entry point
# ──────────────────────────────────────────────────────────────
#
# NOTE: This is intentionally named _main() (not main()) so the script
# cannot be safely imported. The entry point is at module level below.
#


def _main():
    """Module-level entry point. Do not rename to main()."""
    # Parse config
    config = parse_config()

    # Determine language
    lang = config.lang
    if lang == "auto":
        # Auto-detect from locale
        locale = os.environ.get("LANG", "")
        if locale.lower().startswith("ru"):
            lang = "ru"
        else:
            lang = "en"
    tr._lang = lang  # type: ignore[attr-defined]

    # Initialize logger
    global _logger
    _logger = _Logger(lang, config.verbose)

    # Print header
    _logger.bold(trs_get("BUILD_SCRIPT"))
    _logger.info(tr("VERSION_INFO", version=get_version()))

    # Determine platform
    platform_str = config.platform
    if platform_str == "auto":
        platform_str = detect_platform()
    _logger.info(tr("DETECTED_PLATFORM", platform=platform_str))

    # Determine compiler
    compiler_str = config.compiler
    if compiler_str == "auto":
        compiler_str = detect_compiler(platform_str)
    _logger.info(tr("DETECTED_COMPILER", compiler=compiler_str))

    # Validate compiler against platform
    VALID_COMPILERS = {
        PLATFORM_WINDOWS: {COMPILER_MSCV, COMPILER_GCC, COMPILER_CLANG},
        PLATFORM_LINUX: {COMPILER_GCC, COMPILER_CLANG},
        PLATFORM_DARWIN: {COMPILER_GCC, COMPILER_CLANG},
    }
    allowed = VALID_COMPILERS.get(platform_str, set())
    if allowed and compiler_str not in allowed:
        _logger.error(
            f"Invalid compiler '{compiler_str}' for platform '{platform_str}'. "
            f"Allowed: {', '.join(sorted(allowed))}"
        )
        sys.exit(1)

    # Resolve toolchain
    toolchain = resolve_toolchain(platform_str, compiler_str)
    _logger.info(tr("DETECTED_TOOLCHAIN", tool=toolchain["make_tool"]))

    # Print config summary
    _logger.header("=== " + tr("INFO") + " ===")
    _logger.info(tr("CONFIG_LOADED", path="(CLI)"))
    _logger.info(tr("BUILD_OUTPUT", path=str(config.build_dir)))
    _logger.info(tr("DEPLOY_OUTPUT", path=str(config.deploy_dir)))
    if config.stage:
        _logger.info(tr("STAGE_" + config.stage.upper()))
    if config.only_build:
        _logger.info(tr("STAGE_BUILD"))
    if config.no_package:
        _logger.info(tr("STAGE_PACKAGE"))

    # ── Phase 1: Check dependencies (Phase 2 of PLAN_bt.md) ──
    _logger.header("=== " + tr("CHECKING_DEPS") + " ===")
    _has_critical_error = False

    # 1a. qmake (always required)
    qmake = config.qmake_path or find_qmake()
    if qmake:
        _logger.success(tr("DEPS_OK"))
        _logger.info(tr("BUILD_OUTPUT", path=str(qmake)))
    else:
        _logger.error(tr("MISSING_TOOL", tool="qmake"))
        _logger.error(tr("ERROR_ABORT"))
        sys.exit(1)

    # 1b. CQtDeployer (required for deploy + packaging)
    cqt_found, cqt_path = check_cqtdeployer()
    if cqt_found:
        _logger.success(tr("DEPLOY_CQT"))
        _logger.info(tr("BUILD_OUTPUT", path=str(cqt_path)))
    else:
        _logger.warn(tr("CQTDEPLOYER_WARN"))
        _logger.warn(tr("CQTDEPLOYER_HINT"))

    # 1c. Packaging tools (platform-specific)
    packaging = check_packaging_tools(platform_str)
    for tool_name, (found, tool_path) in packaging.items():
        if found:
            _logger.success(tr("PACKAGE_SUCCESS", path=tool_name))
            _logger.info(tr("BUILD_OUTPUT", path=str(tool_path)))
        else:
            hint_key = f"{tool_name.upper()}_HINT"
            hint_text = trs_get(hint_key) if hint_key in trs.get(lang, trs.get("en", {})) else ""
            _logger.warn(tr("MISSING_PACKAGING_TOOL", tool=tool_name, hint=hint_text))

    # Determine if we have any critical errors
    # CQtDeployer is critical if deploy or package stage is needed
    if not cqt_found and config.stage not in (None, "build"):
        _has_critical_error = True

    # If critical tools are missing, stop
    if _has_critical_error:
        _logger.error(tr("ERROR_ABORT"))
        _logger.error(tr("USAGE_HINT"))
        sys.exit(1)

    # ── Phase 2: Platform & toolchain summary ──
    _logger.header("=== " + tr("PLATFORM_INFO", platform=platform_str) + " ===")
    _logger.info(tr("DETECTED_COMPILER", compiler=compiler_str))
    _logger.info(tr("DETECTED_TOOLCHAIN", tool=toolchain["make_tool"]))

    # ── Phase 3: Build phase placeholder ──
    if not config.only_build and config.stage != "deploy" and config.stage != "package":
        _logger.header("=== " + tr("BUILD_START") + " ===")
        _logger.info(tr("BUILD_CONFIGURING"))
        _logger.info(tr("BUILD_COMPILING", tool=toolchain["make_tool"]))
        # TODO: actual qmake + make call
        _logger.success(tr("BUILD_SUCCESS"))

        if config.stage == "build":
            _logger.info(tr("STAGE_BUILD"))
            return

    # ── Phase 4: Deploy phase placeholder ──
    if not config.no_package and config.stage != "package":
        _logger.header("=== " + tr("DEPLOY_START") + " ===")
        _logger.info(tr("DEPLOY_CQT"))
        _logger.info(tr("DEPLOY_CLEAN_TRANSLATIONS"))
        _logger.info(tr("DEPLOY_COPY_CHANGELOG"))
        # TODO: actual CQtDeployer call
        _logger.success(tr("DEPLOY_SUCCESS"))

        if config.stage == "deploy":
            _logger.info(tr("STAGE_DEPLOY"))
            return

    # ── Phase 5: Package phase placeholder ──
    if not config.no_package:
        _logger.header("=== " + tr("PACKAGE_START") + " ===")
        if platform_str == PLATFORM_WINDOWS:
            _logger.info(tr("PACKAGE_7Z"))
        elif platform_str == PLATFORM_LINUX:
            _logger.info(tr("PACKAGE_APPIMAGE"))
        elif platform_str == PLATFORM_DARWIN:
            _logger.info(tr("PACKAGE_DMG"))
        # TODO: actual packaging
        _logger.success(tr("PACKAGE_SUCCESS", path=f"ProcessPinner_{get_version()}_{platform_str}"))

    _logger.bold("=" * 40)
    _logger.success(tr("PROGRESS"))


# Module-level execution (no main() function as per requirements)
_main()
