import os
import platform
import shutil
import sys
import argparse
import subprocess
import shlex
import locale
from datetime import date


START_DIR = os.path.abspath(os.path.curdir)
PROJ_DIR = os.path.abspath(os.path.split(__file__)[0])
PROJ_PATH = os.path.join(PROJ_DIR, "ProcessPinner.pro")
class Config:
    def __init__(self):
        self.BUILD_DIR = os.path.join(os.path.curdir,'build')
        self.DEPLOY_DIR = os.path.join(os.path.curdir, 'deploy')
        self.QMAKE = "qmake"
        self.MAKE = "make"
        self.MAKE_THREADS = (os.cpu_count() - 1)
        if self.MAKE_THREADS <= 0:
            self.MAKE_THREADS = 1
        self.ONLY_BUILD = False
        self.NO_PACKAGE = False
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

lang = locale.getlocale()[0].split('_')[0].lower() if locale.getlocale() else 'en'
lang = {
    'russian': 'ru',
    'ru': 'ru'
}.get(lang, 'en')
trs = {
    "ru" : {
        'Util qmake not found. Set manually' : 'Не найдена утилита qmake, укажите вручную',
        'Util make not found. Set manually' : 'Не найдена утилита make, укажите вручную',
        'Current platform: ' : 'Платформа: ',
        'Unsupported platfom: ': 'Неподдерживаемая платформа: ',
        ' failed!': ' неуспешно!',
        "Configure qmake build" : 'Конфигруирование qmake сборки',
        "Make build" : 'Сборка make',
        'Built executable: ' : 'Собран исполняемый файл: ',
        "Leave dir " : 'Выход из дирректории ',
        "Get commits": 'Получение коммитов',
        'Process Pinner builder script' : 'Скрипто сборки Process Pinner',
        'Path to qmake' : 'Расположение qmake',
        'Path to make' : 'Расположение make',
        'make threads (default: ' : 'число потоков в make (по умолчанию: ',
        'build dir (default: ' : 'каталог сборки (по умолчанию: ',
        'dir where will be placed deployed or packaged result (default: ' : 'каталог для расположения деплоя или готового пакета (по умолчанию: ',
        'config file' : 'конфигурационный файл',
        'just build' : 'только собрать',
        'do build, do deploy, no packaging' : 'собрать, подготовить к упаковке, но не упаковывать',
        'show changelog only. Work only with git-branche' : 'показать только changelog. Работает толко в git-ветке',
        "Path " : 'Путь ',
        " not exists!" : ' не существует!',
        "Get last tag name" : "Получение имени последнего тега",
        "Version changed " : 'Версия изменена ',
        "No significant changes": 'Нет значимых изменений',
    }
}

def tr(msg: str) -> str:
    return trs.get(lang, 'en').get(msg, msg)

def eprint(*args, **kwargs):
    print(*args, **kwargs, file=sys.stderr )

def critical(msg: str):
    os.chdir(START_DIR)
    eprint(msg)
    exit(-1)

def run_sub(cmd: list, msg: str, text: bool = True, crit: bool = True):
    print(f"**{msg}**")
    if(IS_DEBUG):
        print(shlex.join(cmd))
    result = subprocess.run(cmd, capture_output=True, text=text)

    if result.returncode != 0:
        eprint(result.stdout)
        eprint(result.stderr)
        if(crit):
            critical(msg + tr(' failed!'))
        else:
            eprint(msg + tr(' failed!'))
    elif IS_DEBUG:
        if text:
            print(result.stdout)
        else:
            print(result.stdout.decode())
    return result

#Платформа
WINDOWS = 'windows'
LINUX = 'linux'
MACOS = 'darwin'
KNOWN_PLATFORMS = [
    WINDOWS,
]
PLATFORM = platform.system().lower()
if PLATFORM not in KNOWN_PLATFORMS:
    eprint(tr('Unsupported platfom: ') + PLATFORM)
    exit(-1)
else:
    print(tr('Current platform: ') + PLATFORM)

if not PLATFORM == WINDOWS:
    result = run_sub(['which', 'qmake'])
    if result.returncode == 0:
        config.QMAKE = result.stdout.strip()
    else:
        critical(tr('Util qmake not found. Set manually'))
    result = run_sub(['which', 'make'])
    if result.returncode == 0:
        config.QMAKE = result.stdout.strip()
    else:
        critical(tr('Util make not found. Set manually'))

    CONFIGURE_CMD = {
        WINDOWS: ['CONFIG-=debug_and_release', 'CONFIG-=debug', 'CONFIG+=release', PROJ_PATH]
    }
    BUILD_CMD = {
        WINDOWS: ['-j']
    }
    DEPLOY_CMD = {

    }
    PACKAGE_CMD = {

    }
PE_ELF = {
    WINDOWS: 'ProcessPinner.exe'
}

def mkdir(path: str):
     if not os.path.exists(path):
        os.mkdir(path)

def build():
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

def getGitLog():
    result = run_sub(['git', 'describe', '--tags', '--abbrev=0'], tr("Get last tag name"), False)
    result = run_sub(
        [
            'git',
            'log',
            f'{result.stdout.decode().strip()}..HEAD',
            '--oneline',
            '--no-merges',
            '--pretty=format:"%s"'
        ],
        tr("Get commits"), False)
    fix = []
    add = []
    change = []
    release = False
    messages = result.stdout.decode('utf-8').split('\n')
    for commit in messages:
        #commit = 'c: '+commit
        maby_release = False
        if commit.startswith('!'):
            commit = commit[1:]
            maby_release = True
        if commit.strip().startswith('f:'):
            _, _, msg = commit.partition('f:')
            fix.append(msg.strip())
            release = maby_release
        if commit.strip().startswith('c:'):
            _, _, msg = commit.partition('c:')
            change.append(msg.strip())
            release = maby_release
        if commit.strip().startswith('a:'):
            _, _, msg = commit.partition('a:')
            add.append(msg.strip())
            release = maby_release
    return (fix, add, change, release)

def getVersion() -> str:
    version = "0.0.0"
    with open(PROJ_PATH, 'r') as proj:
        for line in proj.read().split('\n'):
            if line.strip().startswith('VERSION = '):
                version = line.split(' = ')[-1].strip('"')
    return version

def incrementVersion(old: str, fix: list, add: list, change: list, release: bool) -> str:
    old_major, old_minor, old_patch = [int(x) for x in old.split('.')]
    major, minor, patch = [int(x) for x in old.split('.')]
    if len(fix) > 0:
        patch = patch + 1
    if len(add) > 0 or len(change) > 0:
        minor = minor + 1
        patch = 0
    if release:
        major = major + 1
        minor = 0
        patch = 0
    if old_major == major and old_minor == minor and old_patch == patch:
        return None
    return f'{major}.{minor}.{patch}'

def writeVersionToProject(version: str):
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
            print(tr("Version changed ") +f'{old_version} -> {version}' )

def build_changelog(fix: list, add: list, change: list) -> str:
    text = ""
    if len(change) > 0:
        minor = 0
        text = "Изменено:\n"
        for line in change:
            text = f'{text} - {line}\n'
    if len(add) > 0:
        text = "Добавлено:\n"
        for line in add:
            text = f'{text} - {line}\n'

    if len(fix) > 0:
        text = "Исправлено:\n"
        for line in fix:
            text = f'{text} - {line}\n'

    return text

def parse_config():
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
                        help=tr('dir where will be placed deployed or packaged result (default: ') + config.DEPLOY_DIR + ')')
    parser.add_argument('-c', '--config', help=tr('config file'))
    parser.add_argument('-o', '--only-build', action='store_true',
                        help=tr('just build'))
    parser.add_argument('-n', '--no-package', action='store_true',
                        help=tr('do build, do deploy, no packaging'))
    parser.add_argument('-l', '--changlog-only', action='store_true',
                        help=tr('show changelog only'))

    args = parser.parse_args()

    

    if args.config is not None:
        if not os.path.exists(args.config):
            critical(tr("Path ") + args.config + tr(" not exists!"))
        with open(args.config, 'r') as cf:
            cf_data = cf.read().split('\n')
            for line in cf_data:
                if line.strip().startswith('#'):
                    continue
                pair = line.split('=', maxsplit=1)
                if len(pair) != 2:
                    continue
                if pair[0].strip().upper() == 'BUILD_DIR':
                    config.BUILD_DIR = pair[1].split('#')[0].strip()
                if pair[0].strip().upper() == 'DEPLOY_DIR':
                    config.DEPLOY_DIR = pair[1].split('#')[0].strip()
                if pair[0].strip().upper() == 'QMAKE':
                    config.QMAKE = pair[1].split('#')[0].strip()
                if pair[0].strip().upper() == 'MAKE':
                    config.MAKE = pair[1].split('#')[0].strip()
                if pair[0].strip().upper() == 'MAKE_THREADS':
                    config.MAKE_THREADS = int(pair[1].split('#')[0].strip())
                if pair[0].strip().upper() == 'ONLY_BUILD':
                    config.ONLY_BUILD = bool(pair[1].split('#')[0].strip())
                if pair[0].strip().upper() == 'NO_PACKAGE':
                    config.NO_PACKAGE = bool(pair[1].split('#')[0].strip())
                if pair[0].strip().upper() == 'CHANGELOG':
                    config.CHANGELOG = bool(pair[1].split('#')[0].strip())
    
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
    if args.changlog_only is not None and not config.ONLY_BUILD:
        config.CHANGELOG = args.changlog_only
        

    config.BUILD_DIR = os.path.abspath(config.BUILD_DIR)
    config.DEPLOY_DIR = os.path.abspath(config.DEPLOY_DIR)
    config.QMAKE = os.path.abspath(config.QMAKE)
    config.MAKE = os.path.abspath(config.MAKE)

    if(not os.path.exists(config.QMAKE)):
        critical(tr("Path ") + config.QMAKE + tr(" not exists!"))
    if(not os.path.exists(config.MAKE)):
        critical(tr("Path ") + config.MAKE + tr(" not exists!"))
    




if __name__ == '__main__':
    parse_config()
    print(config)
    print(f'CUR_DIR={START_DIR}')
    print(f'PROJ_DIR={PROJ_DIR}')
    print()

    


    if not config.ONLY_BUILD:
        fix, add, change, release = getGitLog()
        changelog = build_changelog(fix, add, change).strip()
        version = getVersion()
        newVer = incrementVersion(version, fix, add, change, release)
        
        if not len(changelog) == 0 and newVer:
            print(tr("****\nChangelog:"))
            print(changelog)
            if (config.CHANGELOG):
                exit()
            cl_path = os.path.join(PROJ_DIR, "changelog")
            with open(cl_path, 'r', encoding='utf-8') as cl_file:
                cl_content = cl_file.read()
            with open(cl_path, 'w', encoding='utf-8') as cl_file:
                cl_file.write(f'{newVer}({str(date.today())}):\n')
                cl_file.write(changelog)
                if len(cl_content) != 0:
                    cl_file.write('\n\n/**********************************************************************************************************/\n')
                    cl_file.write(cl_content)
            
            writeVersionToProject(newVer)
            version = newVer
        else:
            print(tr("No significant changes"))
        

    build()
    if config.ONLY_BUILD:
        os.chdir(START_DIR)
        exit()

    deploy()
    if config.NO_PACKAGE:
        os.chdir(START_DIR)
        exit()

    package()



















    os.chdir(START_DIR)