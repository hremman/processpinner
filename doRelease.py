import os
import platform
import shutil
import sys
import argparse
import subprocess



START_DIR = os.path.abspath(os.path.curdir)
PROJ_DIR = os.path.abspath(os.path.split(__file__)[0])
class Config:
    def __init__(self):
        self.BUILD_DIR = os.path.join(os.path.curdir,'build')
        self.DEPLOY_DIR = os.path.join(os.path.curdir, 'deploy')
        self.QMAKE = None
        self.MAKE = None
        self.MAKE_THREADS = (os.cpu_count() - 1)
        if self.MAKE_THREADS <= 0:
            self.MAKE_THREADS = 1
        self.ONLY_BUILD = False
        self.NO_PACKAGE = False
    def __str__(self):
        return '\n'.join([f'BUILD_DIR={self.BUILD_DIR}',
            f'DEPLOY_DIR={self.DEPLOY_DIR}',
            f'QMAKE={self.QMAKE}',
            f'MAKE={self.MAKE}',
            f'MAKE_THREADS={self.MAKE_THREADS}',
            f'ONLY_BUILD={self.ONLY_BUILD}',
            f'NO_PACKAGE={self.NO_PACKAGE}'])

config = Config()

def eprint(*args, **kwargs):
    print(*args, **kwargs, file=sys.stderr )

def critical(msg: str):
    eprint(msg)
    exit(-1)

#Платформа
WINDOWS = 'windows'
LINUX = 'linux'
MACOS = 'darwin'
KNOWN_PLATFORMS = [
    WINDOWS,
]
PLATFORM = platform.system().lower()
if PLATFORM not in KNOWN_PLATFORMS:
    eprint(f'Unsupported platfom: {PLATFORM}')
    exit(-1)
else:
    print(f'Current platform: {PLATFORM}')

CONFIGURE_CMD: dict = None
BUILD_CMD: dict = None
DEPLOY_CMD: dict = None
PE_ELF = {
    WINDOWS: 'ProcessPinner.exe'
}

def mkdir(path: str):
     if not os.path.exists(path):
        os.mkdir(path)

def build():
    conf_cmd = CONFIGURE_CMD.get(PLATFORM)
    build_cmd = BUILD_CMD.get(PLATFORM)
    
    mkdir(config.BUILD_DIR)
    os.chdir(config.BUILD_DIR)
    print("**Configure qmake build**")
    result = subprocess.run(conf_cmd, capture_output=True, text=True)
    if result.returncode != 0:
        eprint(result.stdout)
        eprint(result.stderr)
        critical("Configure qmake build failed!")
    print("**Make build**")
    result = subprocess.run(build_cmd, capture_output=True, text=True)
    if result.returncode != 0:
        eprint(result.stdout)
        eprint(result.stderr)
        critical("Make build failed!")
    print(f'Built executable {os.path.join(config.BUILD_DIR, PE_ELF[PLATFORM])}')


def parse_config():
    global config
    parser = argparse.ArgumentParser(description='Process Pinner builder script')
    parser.add_argument('-q', '--qmake-path', help='Path to qmake')
    parser.add_argument('-m', '--make-path', help='Path to make')
    parser.add_argument('-j', '--threads', type=int, default=config.MAKE_THREADS, 
                        help=f'make threads (default: {config.MAKE_THREADS})')
    parser.add_argument('-b', '--build-dir', default=config.BUILD_DIR, 
                        help=f'build dir (default: {config.BUILD_DIR})')
    parser.add_argument('-d', '--deploy-dir', default=config.DEPLOY_DIR, 
                        help=f'dir where will be placed deployed or packaged result (default: {config.DEPLOY_DIR})')
    parser.add_argument('-c', '--config', help='Config file')
    parser.add_argument('-o', '--only-build', action='store_true',
                        help='just build')
    parser.add_argument('-n', '--no-package', action='store_true',
                        help='do build, do deploy, no packaging')

    args = parser.parse_args()

    

    if args.config is not None and os.path.exists(args.config):
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

    config.BUILD_DIR = os.path.abspath(config.BUILD_DIR)
    config.DEPLOY_DIR = os.path.abspath(config.DEPLOY_DIR)
    config.QMAKE = os.path.abspath(config.QMAKE)
    config.MAKE = os.path.abspath(config.MAKE)

    if(not os.path.exists(config.QMAKE)):
        critical(f"Path {config.QMAKE} not exists!")
    if(not os.path.exists(config.MAKE)):
        critical(f"Path {config.MAKE} not exists!")





if __name__ == '__main__':
    parse_config()
    print(config)
    print(f'CUR_DIR={START_DIR}')
    print(f'PROJ_DIR={PROJ_DIR}')
    print()

    CONFIGURE_CMD = {
        WINDOWS: [config.QMAKE, 'CONFIG-=debug_and_release', 'CONFIG-=debug', 'CONFIG+=release', os.path.join(PROJ_DIR, "ProcessPinner.pro")]
    }
    BUILD_CMD = {
        WINDOWS: [config.MAKE, '-j', str(config.MAKE_THREADS)]
    }
    DEPLOY_CMD = {

    }

    build()





















    os.chdir(START_DIR)