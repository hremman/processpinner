import locale
import platform

PROJECT_NAME = "ProcPinn"
WIN = 'windows'
MAC = 'darwin'
LINUX = 'linux'
PLATFORM = platform.system().lower()

LANG = locale.getlocale()[0].split('_')[0].lower() if locale.getlocale() else 'en'
LANG = {
    'russian': 'ru',
    'ru': 'ru'
}.get(LANG, 'en')

UNKNOWN_KEY = 'UNKNOWN_KEY'
TRANSLATE_ERROR = 'TRANSLATE_ERROR'
ERROR = 'ERROR'
DOCKER = '"docker build . -t qt6-appimage-builder" "docker run --rm -u $(id -u):$(id -g) -v "$(pwd)":/workspace:z qt6-appimage-builder"'

TRS = {
    'ru': {
        #help
        'HELP_DESCRIPTION': ("Скрипт сборки ProcessPinner." + f' Если AppImage не запускается с SEGFAULT, то используйте докер образ.{DOCKER}' if PLATFORM == LINUX else ''),
        'HELP_VERSION':'Вывести версию скрипта и выйти',
        'HELP_HELP':'Вывести справку и выйти',
        'HELP_QMAKE': 'Указать расположение qmake. Если не указано, пробует искать',
        'HELP_MAKE': 'Указать располежение make. Если не указано, пробует искать',
        'HELP_BUILD_DIR': 'Каталог сборки.',
        'HELP_DEPLOY_DIR': 'Каталог для размещения дистрибутива',
        'HELP_STAGE': 'Остановиться после этапа',
        'HELP_VERBOSE': 'Расширеный вывод',
        'HELP_QUIET': 'Тихий режим. Перекрывает --verbose',
        'HELP_GLOB_GROUP': 'Глобальные',
        'HELP_ENV_GROUP': 'Окружение',
        'HELP_MODE_GROUP': 'Поведение',
        'HELP_SHOW_CL': 'Показать текущие изменения и выйти',
        'HELP_TAG': 'Зафиксировать релиз в git',
        'HELP_CONFIG': 'Расположение конфигурационного файла',
        'HELP_MSVC': 'Использовать компилятор Microsoft Visual Studio. Доступно только для Windows.\n\t\tПри указании инструментов вручную, qmake должен быть соответствующим. Ключи --make и --compiler будут проигнорированы',
        'HELP_MSVC_TOOLS': 'Путь расположения сборочной оснастки. Пример: "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community". Если не указано, пробует искать',
        'HELP_GIT': 'Расположение git. Если не указано, пробует искать',
        'HELP_QT': ('Расположение Qt. Типичное: ' + ('C:\\Qt' if PLATFORM == WIN else '$HOME/Qt. Если установлено через пакетный менеджер, то можно не указывать')),
        'HELP_DEPLOYER': 'Инструмент извлечения файлов приложения. Необходим {what}',
        'HELP_DOWNLOAD_TOOL': 'Разрешает скачать linuxdeploy и appimagetool при их отсутствии',
        'HELP_PACKAGER': 'Упаковщик. Необходим {what}',
        #Критические
        'UNSUPPORTED_PLATFORM': 'Неподдерживаемая платформа "{platform}"',
        'EXC_SEMVER_PARSE': 'Неверный формат семантической версии "{ver}"',
        #Остальное
        'WARN_SEMVER_PARSE': 'Частей семантической версии больше 4 "{ver}"',
        ERROR: 'Ошибка: {text}',
        'VERSION_CHANGED': 'Версия обновлена: {old} -> {new}',
        'VERSION_NOT_FOUND': 'Не наёдена строка с версией в {path}',
        'NO_CHANGE': 'Нет значимых изменений',
        'GIT_INACCESSIBLE': 'git не найден',
        'LINDEP_INACCESSIBLE': 'linuxdeploy не найден',
        'LINDEPQT_INACCESSIBLE': 'Плагин linuxdeploy для qt не найден',
        'APPIMG_INACCESSIBLE': 'appimagetool не найден',
        'QT_INACCESSIBLE': 'Qt или удовлетворяющая версия не найдена',
        'MSVC_INACCESSIBLE': 'Оснастка MS VC не найдена',
        'CONF_NOT_EXISTS': 'Файл конфигурации ({path}) не существует',
        'QMAKE_INACCESSIBLE': 'qmake не найден',
        'MAKE_INACCESSIBLE': 'make не найден',
        'FILE_NOT_FOUND': 'Файл "{path}" не найден',
        'UNBUILDABLE': 'Сборка невозможна из-за неполноты окружения',
        'UNDEPLOYABLE': 'Извлечение невозможно из-за отсутствия утилиты',
        'UNPKG': 'Упаковка невозможна из-за отсутствия утилиты',
        'CQTD_INACCESSIBLE': 'cqtdeployer не найден',
        'BUILD_SUCCESS': 'Сборка успешно завершена в {path}',
        'DEPLOY_SUCCESS': 'Успешно извлечено в {path}',
        'PKG_SUCCESS': 'Успешно упаковано в {path}',
        'RM_FAULT': 'Удаление {path} закончилось ошибкой: {msg}',
        'QMAKE_ERROR': 'Конфигурирование проекта завершилось ошибкой',
        'MAKE_ERROR': 'Сборка завершилось ошибкой',
        'PROJ_VERSION': f'Версия {PROJECT_NAME}: {{ver}}',
        #tr ошибки
        UNKNOWN_KEY: 'Неизвестный ключ вывода "{key}"',
        TRANSLATE_ERROR: 'Ошибка перевода:({t})  {error}'
    },
    'en': {
        #help
        'HELP_DESCRIPTION': ("ProcessPinner build script." + f' If the AppImage fails to start with a SEGFAULT, use the Docker image.{DOCKER}' if PLATFORM == LINUX else ''),
        'HELP_VERSION': 'Print the script version and exit',
        'HELP_HELP': 'Print help and exit',
        'HELP_QMAKE': 'Specify qmake location. If not specified, it will try to find it',
        'HELP_MAKE': 'Specify make location. If not specified, it will try to find it',
        'HELP_BUILD_DIR': 'Build directory.',
        'HELP_DEPLOY_DIR': 'Directory for placing the distribution',
        'HELP_STAGE': 'Stop after the stage',
        'HELP_VERBOSE': 'Verbose output',
        'HELP_QUIET': 'Quiet mode. Overrides --verbose',
        'HELP_GLOB_GROUP': 'Global',
        'HELP_ENV_GROUP': 'Environment',
        'HELP_MODE_GROUP': 'Behavior',
        'HELP_SHOW_CL': 'Show current changes and exit',
        'HELP_TAG': 'Tag the release in git',
        'HELP_CONFIG': 'Configuration file location',
        'HELP_MSVC': 'Use the Microsoft Visual Studio compiler. Available only on Windows.\n\t\tWhen specifying tools manually, qmake must be the corresponding one. The --make and --compiler options will be ignored',
        'HELP_MSVC_TOOLS': 'Build tools path. Example: "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community". If not specified, it will try to find it',
        'HELP_GIT': 'git location. If not specified, it will try to find it',
        'HELP_QT': ('Qt location. Typical: ' + ('C:\\Qt' if PLATFORM == WIN else '$HOME/Qt. If installed via a package manager, it can be omitted')),
        'HELP_DEPLOYER': 'Application file extraction tool. Required {what}',
        'HELP_DOWNLOAD_TOOL': 'Allows downloading linuxdeploy and appimagetool if they are missing',
        'HELP_PACKAGER': 'Packager. Required {what}',
        #Critical
        'UNSUPPORTED_PLATFORM': 'Unsupported platform "{platform}"',
        'EXC_SEMVER_PARSE': 'Invalid semantic version format "{ver}"',
        #Other
        'WARN_SEMVER_PARSE': 'Semantic version has more than 4 parts "{ver}"',
        ERROR: 'Error: {text}',
        'VERSION_CHANGED': 'Version updated: {old} -> {new}',
        'VERSION_NOT_FOUND': 'Version line not found in {path}',
        'NO_CHANGE': 'No significant changes',
        'GIT_INACCESSIBLE': 'git not found',
        'LINDEP_INACCESSIBLE': 'linuxdeploy not found',
        'LINDEPQT_INACCESSIBLE': 'linuxdeploy Qt plugin not found',
        'APPIMG_INACCESSIBLE': 'appimagetool not found',
        'QT_INACCESSIBLE': 'Qt or a satisfying version not found',
        'MSVC_INACCESSIBLE': 'MS VC toolchain not found',
        'CONF_NOT_EXISTS': 'Configuration file ({path}) does not exist',
        'QMAKE_INACCESSIBLE': 'qmake not found',
        'MAKE_INACCESSIBLE': 'make not found',
        'FILE_NOT_FOUND': 'File "{path}" not found',
        'UNBUILDABLE': 'Build impossible due to incomplete environment',
        'UNDEPLOYABLE': 'Extraction impossible due to missing utility',
        'UNPKG': 'Packaging impossible due to missing utility',
        'CQTD_INACCESSIBLE': 'cqtdeployer not found',
        'BUILD_SUCCESS': 'Build completed successfully in {path}',
        'DEPLOY_SUCCESS': 'Successfully extracted to {path}',
        'PKG_SUCCESS': 'Successfully packaged to {path}',
        'RM_FAULT': 'Deleting {path} failed: {msg}',
        'QMAKE_ERROR': 'Project configuration failed',
        'MAKE_ERROR': 'Build failed',
        'PROJ_VERSION': f'Version of {PROJECT_NAME}: {{ver}}',
        #tr ошибки
        UNKNOWN_KEY: 'Unknown output key "{key}"',
        TRANSLATE_ERROR: 'Translation error :({t})  {error}'
    }
}.get(LANG, 'en')