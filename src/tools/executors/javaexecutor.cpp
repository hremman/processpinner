#include <QDir>

#include "javaexecutor.h"

const QString JavaExecutor::extension{"jar"};

const std::pair<QString, QString (*)(const QFileInfo &)> JavaExecutor::pair = std::make_pair (
    extension, JavaExecutor::runString
);

bool JavaExecutor::detector(const QFileInfo & file){
    return file.suffix() == extension;
}

QString JavaExecutor::runString(const QFileInfo & file){
    return "java -jar " + file.absoluteFilePath();
}
