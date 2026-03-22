#include <QDir>

#include "pythonexecutor.h"

const QString PythonExecutor::extension = "py";

const std::pair<QString, QString (*)(const QFileInfo &)> PythonExecutor::pair = std::make_pair (
    extension, PythonExecutor::runString
);

const std::vector<QString> PythonExecutor::venvs = {
    ".env/bin/python", ".venv/bin/python", "env/bin/python", "venv/bin/python"
};


bool PythonExecutor::detector(const QFileInfo & file){
    return file.suffix() == extension;
}

QString PythonExecutor::runString(const QFileInfo & file){
    QDir dir = file.absoluteDir();
    QString executor = "python";
    for( const QString & venv: venvs){
        if (dir.exists(venv)){
            executor = dir.absoluteFilePath(venv);
        }
    }
    return executor + " " + file.absoluteFilePath();
}

QStringList PythonExecutor::envs(const QFileInfo &){
    return QStringList{"PYTHONUNBUFFERED=1"};
}
