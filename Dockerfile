FROM ubuntu:22.04

# Отключаем интерактивные диалоги apt
ENV DEBIAN_FRONTEND=noninteractive

# Устанавливаем базовые утилиты, компилятор и зависимости Qt6
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    build-essential \
    qmake6 \
    qt6-base-dev \
    qt6-l10n-tools \
    qt6-tools-dev-tools \
    wget \
    file \
    binutils \
    patchelf \
    ca-certificates \
    git && \
    apt install -y software-properties-common && \
    add-apt-repository ppa:deadsnakes/ppa -y && \
    apt update && \
    apt install -y python3.12 \
    && rm -rf /var/lib/apt/lists/*


# Создаем папку для инструментов упаковки
RUN mkdir -p /opt/appimages
WORKDIR /opt/appimages

# Скачиваем современный linuxdeploy и плагин для Qt
RUN wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage && \
    wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage && \
    wget https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage && \
    chmod +x *.AppImage

# Настраиваем переменные окружения для сборщика
ENV PATH="/opt/appimages:${PATH}"
ENV QMAKE="/usr/bin/qmake6"

# Назначаем рабочую директорию для исходников вашего проекта
WORKDIR /workspace

ENV APPIMAGE_EXTRACT_AND_RUN=1

# Команда по умолчанию: сборка проекта и упаковка в AppImage
#CMD echo BUILD && \
#    mkdir -p build && rm -rf ./build/* && cd build && \
#    qmake6 CONFIG-=debug_and_release CONFIG-=debug CONFIG+=release ../ProcessPinner.pro && \
#    make && \
#    echo DEPLOY && \
#    cd ../ && mkdir -p deploy && rm -rf ./deploy/* && \
#    cp resources/logo-512.png deploy/ProcessPinner.png && \
#    linuxdeploy-x86_64.AppImage --appdir deploy/ProcessPinner \
#        --executable=build/ProcessPinner \
#        --plugin=qt --desktop-file=resources/ProcessPinner.desktop \
#        --icon-file=deploy/ProcessPinner.png && \
#    cd deploy && \
#    echo PACKAGING && \
#    appimagetool-x86_64.AppImage ./ProcessPinner && \
#    rm -rf ../build && rm -rf ./ProcessPinner && \
#    echo DONE
CMD LANG=ru_RU python3.12 build.py \
        --deployer=/opt/appimages/linuxdeploy-x86_64.AppImage \
        --packager=/opt/appimages/appimagetool-x86_64.AppImage \