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

TRS = {
    'ru': {
        #help
        'HELP_DESCRIPTION': "Скрипт сборки ProcessPinner.",
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
        'HELP_DEPLOYER': 'Инструмент извлечения файлов приложения. Необходим {what}. Если не указано, пробует искать',
        'HELP_DOWNLOAD_TOOL': 'Разрешает скачать linuxdeploy и appimagetool при их отсутствии',
        'HELP_PACKAGER': 'Упаковщик. Необходим {what}. Если не указано, пробует искать',
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
        'BUILD_SCRIPT': 'ProcessPinner Build Script',

        #tr ошибки
        UNKNOWN_KEY: 'Unknown output key "{key:d}"',
        TRANSLATE_ERROR: 'Translation error :({t})  {error}'
    }
}.get(LANG, 'en')

