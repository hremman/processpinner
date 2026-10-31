#!/usr/bin/env python3

from __future__ import annotations
import argparse
import locale# type: ignore[attr-defined]
import logging
import os
import platform
import shlex
import shutil
import subprocess
import sys

from dataclasses import dataclass
from datetime import date
from enum import Enum
from pathlib import Path
from subprocess import CompletedProcess
from typing import Any, NoReturn

from trs import *
import urllib.request

__version__ = "2.0"

#===============================================
#журналирование

#SUCCESS
logging.SUCCESS = 21 # type: ignore[attr-defined]
# noinspection PyUnresolvedReferences
logging.addLevelName(logging.SUCCESS, "SUCCESS")
def success(self, message, *args, **kwargs):
    if self.isEnabledFor(logging.SUCCESS): # type: ignore[attr-defined]
        # noinspection PyUnresolvedReferences
        self._log(logging.SUCCESS, message, args, **kwargs) # type: ignore[attr-defined]
logging.Logger.success = success # type: ignore[attr-defined]

#HEAD
logging.HEAD = 22 # type: ignore[attr-defined]
# noinspection PyUnresolvedReferences
logging.addLevelName(logging.HEAD, "HEAD")
def head(self, message, *args, **kwargs):
    # noinspection PyUnresolvedReferences
    if self.isEnabledFor(logging.HEAD):# type: ignore[attr-defined]
        # noinspection PyUnresolvedReferences
        self._log(logging.HEAD, message, args, **kwargs)# type: ignore[attr-defined]
logging.Logger.head = head # type: ignore[attr-defined]

#цвета
class ColorizedFormatter(logging.Formatter):
    RESET = "\033[0m"; CYAN = "\033[36m"; GREEN = "\033[32m"; YELLOW = "\033[33m"; RED = "\033[31m"; WHITE = "\033[37m"; MAGENTA = "\033[35m"; BG_RED = "\033[41m"; BOLD = "\033[1m"
    def __init__(self):
        super().__init__()

    def format(self, record):
        match record.levelno:
            case logging.DEBUG:
                return f'{ColorizedFormatter.CYAN}[D] {record.msg}{ColorizedFormatter.RESET}'
            case logging.INFO:
                return record.msg
            case logging.SUCCESS: # type: ignore[attr-defined]
                return f'{ColorizedFormatter.GREEN}[+] {record.msg}{ColorizedFormatter.RESET}'
            case logging.WARNING:
                return f'{ColorizedFormatter.BOLD}{ColorizedFormatter.YELLOW}[!] {record.msg}{ColorizedFormatter.RESET}'
            case logging.ERROR:
                return f'{ColorizedFormatter.RED}[E] {record.msg}{ColorizedFormatter.RESET}'
            case logging.CRITICAL:
                return f'{ColorizedFormatter.BOLD}{ColorizedFormatter.WHITE}{ColorizedFormatter.BG_RED}[F] {record.msg}{ColorizedFormatter.RESET}'
            case logging.HEAD: # type: ignore[attr-defined]
                border = '-' * (len(record.msg) + 2)
                msg = '+' + border + '+' + f'\n| {record.msg} |\n' + '+' + border + '+'
                return f'{ColorizedFormatter.BOLD}{ColorizedFormatter.MAGENTA}{msg}{ColorizedFormatter.RESET}'
            case _:
                return record.msg


#logger
fmt = ColorizedFormatter()
handler = logging.StreamHandler(sys.stdout)
handler.setFormatter(fmt)
handler.setLevel(logging.DEBUG)
handler.addFilter(lambda record: record if record.levelno < logging.ERROR else None)

err_handler = logging.StreamHandler()
err_handler.setFormatter(fmt)
err_handler.setLevel(logging.ERROR)

logger = logging.getLogger()
logger.addHandler(handler)
logger.setLevel(logging.INFO)
logger.addHandler(err_handler)

#=====================================================
# транслятор

def tr(msg: str, **kwargs) -> str:
    """Translate a message string to the current language.
    Supports simple format-style substitutions: tr("key", value=x) → "key: {value}".
    """
    logger.debug(f'tr: {msg}')
    text = TRS.get(msg, None)
    if text is None:
        logger.warning(tr(UNKNOWN_KEY, key=msg))
        return msg
    if kwargs:
        try:
            text = text.format(**kwargs)
        except (KeyError, IndexError, ValueError) as e:
            logger.error(tr(TRANSLATE_ERROR, error=str(e), t = type(e)))
    return text

#=====================================================
#константы 1
WIN = 'windows'
MAC = 'darwin'
LINUX = 'linux'
PLATFORM = platform.system().lower()
PATH_SEP = ':' if PLATFORM != WIN else ';'

PROJECT_NAME = 'ProcessPinner'

START_DIR = Path.cwd().absolute()
PROJECT_DIR = Path(__file__).resolve().absolute().parent
PROJECT_PATH = PROJECT_DIR / f'{PROJECT_NAME}.pro'
RESOURCES_DIR = PROJECT_DIR / 'resources'
CH_LOG_PATH = PROJECT_DIR / 'changelog'
CH_LOG_RES_PATH = RESOURCES_DIR / 'changelog.txt'

#=====================================================
#Вспомогательные функции

def read(filepath: Path) -> str: 
    logger.debug(f'read {filepath}')
    with open(filepath, 'r') as inf:
        return inf.read()

def write(filepath: Path, content: str): 
    logger.debug(f'write {filepath}')
    with open(filepath, 'w') as outf:
        outf.write(content)

def write_proj(project: str):
    logger.debug('write_proj')
    write(PROJECT_PATH, project)

def read_proj() -> str:
    logger.debug('read_proj')
    return read(PROJECT_PATH)

def critical(msg: str) -> NoReturn:
    """Вывести критическую ошибку в журнал, вернуться в исходный каталог и завершиться."""
    logger.debug('critical')
    os.chdir(START_DIR)
    logger.critical(msg)
    exit(-1)

def run_sub(cmd: list, cwd: Path, text: bool = True, crit: bool = True, error_msg: str | None = None) -> \
CompletedProcess[Any] | None:

    logger.debug(f'run_sub: {shlex.join([str(x) for x in cmd])}')
    if not error_msg:
        error_msg = tr(ERROR, text=cmd[0])
    try:
        result = subprocess.run(cmd, capture_output=True, text=text, cwd= cwd, env=os.environ)
    except Exception as e:
        if crit:
            logger.critical(tr(ERROR, text= e))
            critical(error_msg)
        else:
            logger.error(tr(ERROR, text= e))
            logger.error(error_msg)
            return None

    if result.returncode != 0:
        logger.error(f'stdout: {result.stdout}')
        logger.error(f'stderr: {result.stderr}')
        if crit:
            critical(error_msg)
        else:
            logger.error(error_msg)
    else:
        logger.debug(result.stdout if text else result.stdout.decode())

    return result

def git(params: list) -> str:
    return run_sub([config.git] + params, PROJECT_DIR, False).stdout.decode('utf-8')

#=====================================================
# константы 2

if PLATFORM not in [WIN, MAC, LINUX]:
    critical(tr('UNSUPPORTED_PLATFORM', platform= PLATFORM))
QMAKE_ARGS = ["CONFIG-=debug_and_release", "CONFIG-=debug", "CONFIG+=release", PROJECT_PATH]
MAKE_ARGS = ['-j', '4']

EXEC_FILE = f'{PROJECT_NAME}.exe' if PLATFORM == WIN else PROJECT_NAME

#=====================================================
#Вспомогательные типы
class Stage(Enum):
    NO = "no"
    BUILD = "build"
    DEPLOY = "deploy"
    PACKAGE = "pkg"

    def __str__(self):
        return self.value

@dataclass
class Changelog:
    c: list[str]
    a: list[str]
    f: list[str]
    r: bool = False

    def format(self, version: SemVer | None = None, epilog: bool = True) -> str | None:
        """
        Сформировать текст changelog на русском по списку коммитов.
        Порядок секций: Изменено → Добавлено → Исправлено.
        """
        logger.debug('Changelog::format')
        if version is not None:
            text = f'{version} ({str(date.today())}):\n'
        else:
            text = ""
        content = False
        if len(self.c) > 0:
            content += True
            text += "Изменено:\n"
            for line in self.c:
                text = f'{text} - {line}\n'
            text = f'{text}\n'
        if len(self.a) > 0:
            content += True
            text += "Добавлено:\n"
            for line in self.a:
                text = f'{text} - {line}\n'
            text = f'{text}\n'
        if len(self.f) > 0:
            content += True
            text += "Исправлено:\n"
            for line in self.f:
                text = f'{text} - {line}\n'
            text = f'{text}\n'
        if not content:
            return None
        if epilog:
            text += '/**********************************************************************************************************/\n'
        return text


class SemVer:
    def __init__(self, major: int = 0, minor: int = 0, patch: int = 0):
        self.major = major
        self.minor = minor
        self.patch = patch

    def __eq__(self, other) -> bool:
        logger.debug(f'SemVer::__eq__: {self}, {other}')
        if not isinstance(other, SemVer):
            return False
        return self.major == other.major and self.minor == other.minor and self.patch == other.patch
    
    @classmethod
    def from_str(cls, string: str) -> SemVer:
        logger.debug(f'SemVer::from_str: {string}')
        parts = string.split('.')
        if len(parts) > 3:
            logger.warning(tr('WARN_SEMVER_PARSE', ver = string))
        try:
            major = int(parts[0]); minor = int(parts[1]); patch = int(parts[2])
            return cls(major, minor, patch)
        except ValueError:
            critical(tr('EXC_SEMVER_PARSE', ver = string))

    def __str__(self) -> str:
        return f'{self.major}.{self.minor}.{self.patch}'
    
    def increment(self, cl: Changelog):
        logger.debug('SemVer::increment')
        """
        Вычислить новую версию на основе типа коммитов.
        Правила (семантическое версионирование):
        fix    → X.Y.Z+1  (инкремент патча)
        add/c  → X.Y+1.0  (инкремент минора, сброс патча)
        release → X+1.0.0 (инкремент мажора, сброс минора и патча)
        Если версия не изменилась, возвращает None.
        """

        if len(cl.f) > 0:
            self.patch += 1
        if len(cl.a) > 0 or len(cl.c) > 0:
            self.minor += 1; self.patch = 0
        if cl.r:
            self.major += 1; self.minor = 0; self.patch = 0

@dataclass
class Stages:
    build = False
    deploy = False
    package = False
    tag = False

    @classmethod
    def from_stage(cls, stage: Stage) -> Stages:
        s = cls()
        match stage:
            case Stage.NO:
                pass
            case Stage.BUILD:
                s.build = True
            case Stage.DEPLOY:
                s.build = True
                s.deploy = True
            case _:
                s.build = True
                s.deploy = True
                s.package = True
        return s

@dataclass
class Configuration:
    msvc = False
    msvc_tools = None
    qmake = 'qmake6'
    make = 'make'
    git = 'git'
    qt_path = Path('c:/Qt') if PLATFORM == WIN else Path(os.environ['HOME']) / 'Qt'
    qt_ver_path = None
    build_dir = PROJECT_DIR / 'build'
    deploy_dir = PROJECT_DIR / 'deploy'
    deployer: Path | None= None
    packager: Path | None = None
    stages = Stages()
    verbose = False
    qt_no = False
    version = SemVer()
    download_tools = False

    def from_file(self, file: Path):
        bools = ['verbose', 'tag', 'msvc', 'download_tools']
        if not file.exists():
            critical(tr('CONF_NOT_EXISTS', path=file))
        pairs = dict()
        with open(file, 'r') as inf:
            for line in inf.readlines():
                key, sep, value = line.strip().partition('=')
                if len(sep) == 0 or len(value) == 0:
                    continue
                if key.strip().lower() in bools:
                    value = value.strip().lower() in ("1", "true", "yes")
                elif key.strip().lower() == 'stages':
                    value = Stage(value.strip().lower())
                else:
                    value = Path(value.strip())
                pairs[key.strip().lower()] = value
        self.from_dict(pairs)


    def from_dict(self, pairs: dict):
        self.msvc = pairs.get('msvc') if pairs.get('msvc') else self.msvc
        self.msvc_tools = pairs.get('msvc_tools') if pairs.get('msvc_tools') else self.msvc_tools
        self.qmake = pairs.get('qmake') if pairs.get('qmake') else self.qmake
        self.make = pairs.get('make') if pairs.get('make') else self.make
        self.git = pairs.get('git') if pairs.get('git') else self.git
        self.qt_path = pairs.get('qt_path') if pairs.get('qt_path') else self.qt_path
        self.build_dir = pairs.get('build_dir') if pairs.get('build_dir') else self.build_dir
        self.deploy_dir = pairs.get('deploy_dir') if pairs.get('deploy_dir') else self.deploy_dir
        stage = pairs.get('stages')
        if stage:
            self.stages = Stages.from_stage(stage)
        self.stages.tag = pairs.get('tag') if pairs.get('tag') else self.stages.tag
        self.verbose = pairs.get('verbose') if pairs.get('verbose') else self.verbose
        self.download_tools = pairs.get('download_tools') if pairs.get('download_tools') else self.download_tools
        self.packager = pairs.get('packager') if pairs.get('packager') else self.packager
        self.deployer = pairs.get('deployer') if pairs.get('deployer') else self.deployer

config = Configuration()

#=====================================================
#Обнаружение зависимостей
def choose_qt():
    logger.debug('choose_qt')
    if config.qt_no:
        return False
    if not config.qt_no and config.qt_ver_path and config.qt_ver_path.exists():
        return True
    if not config.qt_path.exists():
        config.qt_no = True
        return False
    qt_version = SemVer(0, 2, 0)
    for entry in config.qt_path.iterdir():
        if entry.name.startswith('6'):
            try:
                qt_ver = SemVer.from_str(entry.name)
                if qt_ver.minor >= qt_version.minor:
                    qt_version.minor = qt_ver.minor
                    qt_version.major = 6
            except ValueError:
                pass
    if qt_version.major != 6:
        config.qt_no = True
        return False
    config.qt_ver_path = config.qt_path / str(qt_version)
    config.qt_no = False
    return True


def qt_arch():
    arch = platform.machine().lower()
    if "amd64" in arch or "x86_64" in arch:
        return "_64"  # Стандартные 64-битные процессоры Intel/AMD
    elif "arm64" in arch or "aarch64" in arch:
        return "_arm64"  # Для современных ARM-ноутбуков
    else:
        return ""

def add_in_path(path: Path):
    os.environ['PATH'] = f"{os.environ['PATH']}{PATH_SEP}{path}"

def detect_tool(tool: str | Path, tool_paths: list[Path]) -> bool:
    if type(tool) != str:
        if tool.exists():
            add_in_path(tool.parent)
            return True
    if not shutil.which(tool):
        for tool_path in tool_paths:
            if tool_path.exists():
                add_in_path(tool_path.parent)
                return True
        else:
            return False
    return True

def detect_git():
    logger.debug('detect_git')
    result = detect_tool('git', {
            MAC: [
                Path('/opt/homebrew/bin/git'),
                Path('/usr/bin/git'),
                Path('/opt/git/bin/git'),
                Path('/usr/local/bin/git'),

            ],
            LINUX: [
                Path('/usr/bin/git'),
                Path('/opt/git/bin/git'),
                Path('/usr/local/bin/git'),
                Path(os.environ.get('HOME', '')) / '.local/bin/git'
            ],
            WIN: [
                Path('C:/Program Files/Git/cmd/git.exe'),
                Path('C:/Program Files (x86)/Git/cmd/git.exe'),
                Path('C:/ProgramData/chocolatey/bin/git.exe'),
                Path(os.environ.get('USERPROFILE', '')) / 'AppData/Local/Programs/Git/cmd/git.exe'
            ]
        }[PLATFORM])
    if not result:
        config.git = None
        logger.warning(tr('GIT_INACCESSIBLE'))

def no_qmake():
    config.qmake = None
    logger.warning(tr('QMAKE_INACCESSIBLE'))

def no_make():
    config.qmake = None
    logger.warning(tr('MAKE_INACCESSIBLE'))

def detect_unix_toolchain():
    logger.debug('detect_unix_toolchain')
    additional = []
    if choose_qt():
        if PLATFORM == LINUX:
            additional.append(config.qt_ver_path / f'gcc{qt_arch()}/bin/qmake')
        else:
            additional.append(config.qt_ver_path / f'macos/bin/qmake')
            additional.append(config.qt_ver_path / f'clang_64/bin/qmake')
    result = detect_tool('qmake6', {
            MAC: [
                Path('/opt/homebrew/opt/qt/bin/qmake'),
                Path('/usr/local/opt/qt/bin/qmake'),
                Path('/opt/local/libexec/qt6/bin/qmake'),
            ] ,
            LINUX: [
                Path('/usr/bin/qmake6'),
                Path('/usr/lib/qt6/bin/qmake'),
            ]
        }[PLATFORM]+ additional)
    if not result:
        no_qmake()

    result = detect_tool('make', {
            MAC: [
                Path('/usr/bin/make'),
                Path('/opt/homebrew/bin/gmake'),
            ],
            LINUX: [
                Path('/usr/bin/make'),
                Path('/bin/make'),
                Path('/usr/local/bin/make'),
            ]
        }[PLATFORM])
    if not result:
        no_make()
def download_deploy_linux():
    tools = [
        ('https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage', 'linuxdeploy-x86_64.AppImage'),
        ('https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage', 'linuxdeploy-plugin-qt-x86_64.AppImage'),
        ('https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage', 'appimagetool-x86_64.AppImage'),
    ]
    config.deploy_dir.mkdir(exist_ok=True)
    for tool in tools:
        urllib.request.urlretrieve(tool[0], config.deploy_dir / tool[1])
        os.chmod(config.deploy_dir / tool[1], 0o755)
    config.deployer = config.deploy_dir / 'linuxdeploy-x86_64.AppImage'
    config.packager = config.deploy_dir / 'appimagetool-x86_64.AppImage'
    add_in_path(config.deploy_dir)

def detect_deploy():
    if PLATFORM == LINUX:
        if not (config.deployer and (config.deployer.name.startswith('linuxdeploy') and config.deployer.exists())):
            if not config.download_tools:
                logger.warning('LINDEP_INACCESSIBLE')
            config.deployer = None
        else:
            add_in_path(config.deployer.parent)

        _,head,tail = config.deployer.name.partition('linuxdeploy')
        plugin = f'{head}-plugin-qt{tail}'
        plugin_path = config.deployer.parent / plugin
        if not plugin_path.exists():
            if not config.download_tools:
                logger.warning('LINDEPQT_INACCESSIBLE')
            config.deployer = None

    elif PLATFORM == WIN:
        if not (config.deployer and config.deployer.exists()):
            logger.warning('CQTD_INACCESSIBLE')
            config.deployer = None
    elif PLATFORM == MAC:
        pass
    else:
        logger.warning('UNDEPLOYABLE')
        config.deployer = None

def detect_packager():
    if PLATFORM == LINUX:
        if not (config.packager and (config.deployer.name.startswith('appimagetool') and config.packager.exists())):
            if not config.download_tools:
                logger.warning('APPIMG_INACCESSIBLE')
            config.deployer = None
        else:
            add_in_path(config.deployer.parent)

def detect_win_toolchain():
    logger.debug('detect_win_toolchain')
    if not choose_qt():
        logger.warning(tr('QT_INACCESSIBLE'))
        no_qmake()
        no_make()
        return

    qt_arch_suf = qt_arch()
    #qmake
    system = 'msvc' if config.msvc else 'mingw'
    qmake_matches = list(config.qt_ver_path.glob(f"{system}*{qt_arch_suf}/bin/qmake.exe"))
    if not qmake_matches:
        logger.debug(tr('FILE_NOT_FOUND', path='qmake.exe'))
        no_qmake()

    add_in_path(qmake_matches[-1].parent)
    config.qmake = 'qmake.exe'

    #make
    def no_msvc():
        logger.warning(tr('MSVC_INACCESSIBLE'))
        no_make()

    if config.msvc:
        msvc_arch = {"_64": "amd64", '_arm64': "arm64", "": "x86"}[qt_arch_suf]
        if not config.msvc_tools:
            vs_root = Path("C:/Program Files/Microsoft Visual Studio") if Path(
                "C:/Program Files/Microsoft Visual Studio").exists() else Path(
                "C:/Program Files (86)/Microsoft Visual Studio")
            vs_matches = list(vs_root.glob("*/*/VC/Auxiliary/Build/vcvarsall.bat"))
            if not vs_matches:
                no_msvc()
                return
            vcvarsall = sorted(vs_matches)[-1]
        else:
            if not config.msvc_tools.exists():
                no_msvc()
                return
            vcvarsall = config.msvc_tools / 'VC/Auxiliary/Build/vcvarsall.bat'
        logger.debug(f'set_msvc_env: {vcvarsall}')
        set_msvc_env(vcvarsall, msvc_arch)
        config.make = 'nmake.exe'
    else:
        make_matches = sorted(list(config.qt_path.glob(f"Tools/mingw*{qt_arch_suf}/bin/mingw*-make.exe")))
        if not make_matches:
            no_make()
        add_in_path(make_matches[-1].parent)
        config.make = make_matches[-1].name


#=====================================================
#версионирвоание

def git_log() -> Changelog:
    """
    Получить список коммитов от последнего тега до HEAD.
    Парсит префиксы коммитов для авто-версионирования:
      f: -> исправления (fix)
      a: -> новые фичи (add)
      c: -> изменения (change)
      !  -> флаг релиза (инкремент мажорной версии)
    Возвращает: (fix_list, add_list, change_list, is_release)
    """
    # Получаем имя последнего тега
    logger.debug('git_log')
    result = git(['describe', '--tags', '--abbrev=0'])
    last_tag = result.strip()

    # Получаем логи коммитов от последнего тега до HEAD
    result = git([ 'log', f'{last_tag}..HEAD', '--oneline', '--no-merges', '--pretty=format:"%s"'])

    fix = []; add = []; change = []; release = False
    commits = result.split('\n')

    for commit in commits:
        maby_release = False
        # Префикс '!' — флаг релиза (инкремент мажорной версии)
        if commit.startswith('!'):
            commit = commit[1:]
            maby_release = True
        if commit.strip().startswith('f:'): # Префикс 'f:' — исправление (fix)
            fix.append(commit.partition('f:')[-1])
        elif commit.strip().startswith('c:'): # Префикс 'c:' — изменение (change)
            change.append(commit.partition('c:')[-1])
        elif commit.strip().startswith('a:'): # Префикс 'a:' — новая фича (add)
            add.append(commit.partition('a:')[-1])
        else:
            continue
        release += maby_release
        
    result = Changelog(c=list(set(change)), a=list(set(add)), f=list(set(fix)), r=release)
    logger.debug(f'changelog: {result}')
    return result

def git_tag(name: str):
    logger.debug('git_tag')
    git(['add', str(PROJECT_PATH), str(CH_LOG_PATH)])
    git(['commit', '-m', 'semantic release work'])
    git(['tag', name])
    git(['push', 'origin', name])
    
def extract_version() -> SemVer | None:
    """
    Прочитать текущую версию из файла ProcessPinner.pro.
    """
    logger.debug(f'extract_version')
    
    for line in read_proj().split('\n'):
        if line.strip().startswith('VERSION = '):
            return SemVer.from_str(line.split(' = ')[-1].strip('"'))

    logger.error(tr('VERSION_NOT_FOUND', path= PROJECT_PATH))
    return None

def update_version(new: SemVer | None):
    """Записать новую версию в ProcessPinner.pro, заменив строку 'VERSION = ...'."""
    if new is None:
        return False
    logger.debug(f'update_version')
    proj_content = read_proj().split('\n')
    old = SemVer()
    for i in range(len(proj_content)):
        if proj_content[i].strip().startswith("VERSION = "):
            old = SemVer.from_str(proj_content[i].split(' = ')[1].strip().strip('"'))
            proj_content[i] = f'VERSION = "{new}"'
            break
    else:
        logger.error(tr('VERSION_NOT_FOUND', path= PROJECT_PATH))
        return False
    if new == old:
        logger.debug('version not changed')
        return True
    write_proj("\n".join(proj_content))
    # noinspection PyUnresolvedReferences
    logger.success(tr('VERSION_CHANGED', old= old, new= new))
    return True

#=====================================================
#Функции сборки, деплоя, упаковки
def set_msvc_env(vcvarsall: Path, arch: str):
    logger.debug(f'set_envs_win')
    marker = "===ENV_START==="
    full_cmd = f'call "{vcvarsall}" {arch} && echo {marker} && set'
    output = subprocess.check_output(full_cmd, shell=True, text=True, errors="ignore")
    if marker in output:
        _, env_block = output.split(marker, 1)
    else:
        env_block = output
    current_key = None
    current_value = []
    for line in env_block.splitlines():
        if not line.strip() and not current_key:
            continue
        if "=" in line:
            potential_key, potential_val = line.split("=", 1)
            if " " not in potential_key.strip():
                # Сохраняем предыдущую переменную, если она была
                if current_key:
                    os.environ[current_key] = "\n".join(current_value)

                # Windows-специфика: приводим PATH к верхнему регистру для стабильности
                current_key = potential_key.upper() if potential_key.upper() == "PATH" else potential_key
                current_value = [potential_val]
                continue

        # Если строка не является новым ключом, значит это продолжение многострочного значения
        if current_key:
            current_value.append(line)

        # Не забываем сохранить самую последнюю переменную из цикла
    if current_key:
        os.environ[current_key] = "\n".join(current_value)

def build():
    logger.debug('build')
    config.build_dir.mkdir(parents=True, exist_ok=True)
    os.chdir(config.build_dir)
    for item in config.build_dir.iterdir():
        try:
            if not item.is_dir():
                item.unlink()  # Удаляет файл
            else:
                shutil.rmtree(item)
        except Exception as e:
            logger.error(tr('RM_FAULT', path=config.build_dir, msg=str(e)))
    try:
        run_sub([config.qmake] + QMAKE_ARGS, Path(), error_msg= tr('QMAKE_ERROR'))
    except Exception as e:
        critical(str(e))
    make_cmd = [config.make]
    if not config.msvc:
        make_cmd = make_cmd + MAKE_ARGS
    try:
        run_sub(make_cmd, Path(), error_msg=tr('MAKE_ERROR'))
    except Exception as e:
        critical(str(e))
    os.chdir(START_DIR)
    logger.success(tr('BUILD_SUCCESS', path= config.build_dir))

def deploy():
    logger.debug('deploy')
    config.deploy_dir.mkdir(exist_ok=True)
    if PLATFORM == LINUX:
        target = config.deploy_dir / PROJECT_NAME
        for item in config.deploy_dir.iterdir():
            try:
                if not item.is_dir():
                    item.unlink()  # Удаляет файл
                else:
                    shutil.rmtree(item)
            except Exception as e:
                logger.error(tr('RM_FAULT', path=item, msg=str(e)))
        try:
            shutil.copy(RESOURCES_DIR / 'logo-512.png', RESOURCES_DIR / f'{PROJECT_NAME}.png')
            run_sub([
                config.deployer,
                '--appdir', target,
                '--executable', config.build_dir / EXEC_FILE,
                '--plugin=qt',
                '--desktop-file', RESOURCES_DIR / f'{PROJECT_NAME}.desktop',
                '--icon-file', RESOURCES_DIR / f'{PROJECT_NAME}.png'
            ], config.deploy_dir)
        except Exception as e:
            critical(str(e))
    elif PLATFORM == WIN:
        target = config.deploy_dir / PROJECT_NAME
        target.mkdir(exist_ok=True)
        try:
            run_sub([
                config.deployer,
                '-bin', config.build_dir / EXEC_FILE,
                '-qmake', config.qmake,
                '-targetDir', target
            ], config.deploy_dir)
        except Exception as e:
            critical(str(e))
        for entry in (target / 'translations').iterdir():
            entry.unlink()
        cl_file = CH_LOG_RES_PATH if CH_LOG_RES_PATH.exists() else CH_LOG_PATH
        shutil.copy(cl_file, target / f'{os.path.splitext(cl_file)[0]}.txt')

    logger.success(tr('DEPLOY_SUCCESS', path=target))
    try:
        shutil.rmtree(config.build_dir)
    except Exception as e:
        logger.error(tr('RM_FAULT', path= config.build_dir, msg= str(e)))


def package():
    logger.debug('package')
    target_file_name = f'{PROJECT_NAME}_{config.version}-{platform.machine()}'

    #package magic
    if PLATFORM == LINUX:
        target_file_name = target_file_name + '.AppImage'
        try:
            run_sub([config.packager, PROJECT_NAME, config.deploy_dir / target_file_name], config.deploy_dir)
        except Exception as e:
            critical(str(e))

    logger.success(tr('PKG_SUCCESS', path=config.deploy_dir / target_file_name))
    for item in config.deploy_dir.iterdir():
        # Пропускаем файл, который нужно оставить
        if item.name == target_file_name:
            continue
        try:
            if not item.is_dir():
                item.unlink()  # Удаляет файл
            else:
                shutil.rmtree(item)
        except Exception as e:
            logger.error(tr('RM_FAULT', path=config.build_dir, msg=str(e)))

#=====================================================
if __name__ != '__main__':
    raise RuntimeError()
else:
    parser = argparse.ArgumentParser(
        description=tr('HELP_DESCRIPTION'),
        add_help=False,
    )
    arg_group = parser.add_argument_group(tr('HELP_ENV_GROUP'))
    arg_group.add_argument("--qmake", type=Path, help=tr('HELP_QMAKE'))
    arg_group.add_argument("--make", type=Path, help=tr('HELP_MAKE'))
    arg_group.add_argument("--build-dir", type=Path, help=tr('HELP_BUILD_DIR'))
    arg_group.add_argument("--deploy-dir", type=Path, help=tr('HELP_DEPLOY_DIR'))
    arg_group.add_argument("--git", type=Path, help=tr('HELP_GIT'))
    if PLATFORM == WIN:
        arg_group.add_argument("--msvc", action="store_true", default=False, help=tr('HELP_MSVC'))
        arg_group.add_argument("--msvc-tools", type=Path, help=tr('HELP_MSVC_TOOLS'))
    if PLATFORM != MAC:
        arg_group.add_argument("--deployer", type=Path, help=tr('HELP_DEPLOYER', what='cqtdeployer' if PLATFORM == WIN else 'linuxdeploy + linuxdeploy-plugin-qt'))
        arg_group.add_argument("--packager", type=Path, help=tr('HELP_PACKAGER', what= '7Zip или Zip' if PLATFORM == WIN else 'appimagetool'))
    if PLATFORM == LINUX:
        arg_group.add_argument("--download-tools", action="store_true", default=False, help=tr('HELP_DOWNLOAD_TOOL'))
    arg_group.add_argument("--qt", type=Path, help=tr('HELP_QT'))
    arg_group = parser.add_argument_group(tr('HELP_MODE_GROUP'))
    arg_group.add_argument("--stages", "-s", type=Stage, default=Stage.PACKAGE, choices=[Stage.NO, Stage.BUILD, Stage.DEPLOY], help=tr('HELP_STAGE'))
    arg_group.add_argument("--changes", action="store_true", default=False, help=tr('HELP_SHOW_CL'))
    arg_group.add_argument("--tag", '-t', action="store_true", default=False, help=tr('HELP_TAG'))
    arg_group = parser.add_argument_group(tr('HELP_GLOB_GROUP'))
    arg_group.add_argument("--config", "-c", type=Path, default=None, help=tr('HELP_CONFIG'))
    arg_group.add_argument("--verbose", action="store_true", default=False, help=tr('HELP_VERBOSE'))
    arg_group.add_argument("--quiet", action="store_true", default=False, help=tr('HELP_QUIET'))
    arg_group.add_argument('--version', '-v', action="version", version=f"%(prog)s {__version__}", help=tr('HELP_VERSION'))
    arg_group.add_argument('--help', '-h', action="help", help=tr('HELP_HELP'))

    cmd_args = parser.parse_args()

    cmd_args.config = Path(PROJECT_DIR) / 'examples/build.conf'

    if cmd_args.config and cmd_args.config.exists():
        config.from_file(cmd_args.config)
    config.from_dict(vars(cmd_args))


    if config.verbose:
        logger.setLevel(logging.DEBUG)

    logger.debug(f'{cmd_args}')

    detect_git()

    if cmd_args.changes:
        if config.git:
            cl = git_log()
            version = extract_version()
            version.increment(cl)
            cl = cl.format(version, False)
            logger.info(tr('NO_CHANGE') if cl is None else cl)
            exit(0)
        else:
            critical(tr('GIT_INACCESSIBLE'))
    
    if cmd_args.quiet:
        logger.setLevel(logging.ERROR)

    if PLATFORM == WIN:
        detect_win_toolchain()
    else:
        detect_unix_toolchain()

    detect_deploy()

    config.version = extract_version()
    if config.git:
        cl = git_log()
        config.version.increment(cl)
        cl = cl.format(config.version)
        if cl:
            root_cl = read(CH_LOG_PATH)
            root_cl = f'{cl}\n{root_cl}'
            write(CH_LOG_RES_PATH, root_cl)
            update_version(config.version)

    logger.info(tr('PROJ_VERSION', ver= config.version))
    if config.stages.build:
        if not config.qmake or not config.make:
            critical(tr('UNBUILDABLE'))
        build()

    if config.stages.deploy:
        if not config.deployer:
            critical(tr('UNDEPLOYABLE'))
        deploy()


    if config.stages.package:
        if not config.packager:
            critical(tr('UNPKG'))
        package()



    if config.git and config.stages.tag:
        if cl:
            write(CH_LOG_PATH, root_cl)
            git_tag(str(config.version))
    
    

os.chdir(START_DIR)