#include "tools.h"

#include <QFileInfo>
#include <QDir>
#include <map>

#include <set>
#include <src/tools/executors/binexecutor.h>
#include <src/tools/executors/javaexecutor.h>
#include <src/tools/executors/pythonexecutor.h>

QString tools::buildAbsolutePath(const QString & app, const QString &project_dir, const QString &cwd){
    qDebug() << "QString buildAbsolutePath(" << app <<", "<< project_dir << ", " << cwd << ")";
    QFileInfo appFile(app);
    if ( appFile.isAbsolute()){
        qDebug() << "    retVal: " << app;
        return app;
    }
    if (app.contains(QDir::separator())){ //relative
        if (!project_dir.isEmpty()){
            QString ret = project_dir + QDir::separator() + app;
            qDebug() << "    retVal: " << ret;
            return ret;
        }
        if (!cwd.isEmpty()){
            QString ret = cwd + QDir::separator() + app;
            qDebug() << "    retVal: " << ret;
            return ret;
        }
        QString ret = appFile.absoluteFilePath();
        qDebug() << "    retVal: " << ret;
        return ret;
    }
    qDebug() << "    retVal: " << app;
    return app;

}


QStringList empty;

std::pair<QString, QStringList> tools::selectExecutor(const QString & app){
    static const std::map<QString, QString (*)(const QFileInfo &)> extsExexutors{
        PythonExecutor::pair,
        JavaExecutor::pair
    };

    static const std::map<QString (*)(const QFileInfo &), QStringList (*)(const QFileInfo &)> envs{
        {PythonExecutor::runString, PythonExecutor::envs}
    };

    static const std::vector<std::pair<bool (*)(const QFileInfo &), QString (*)(const QFileInfo &)>> detectExecutors{
        //добавлять по мере появления
    };
    QFileInfo file(app);
    QString extension = file.suffix();
    if ( 0 == extsExexutors.count(extension)){
        if (BinExecutor::detector(file))
            return std::make_pair(app, empty);
        for(size_t i = 0; i < detectExecutors.size(); ++i)
            if ((detectExecutors[i].first)(file)){
                if (envs.count(detectExecutors[i].second)){
                    return std::make_pair(
                        (detectExecutors[i].second)(file),
                        (envs.at(detectExecutors[i].second))(file)
                    );
                }
                return std::make_pair(
                    (detectExecutors[i].second)(file),
                    empty
                    );
            }
        return std::make_pair(app, empty);
    }
    auto funcRunString = extsExexutors.at(extension);

    if (envs.count(funcRunString)){
        return std::make_pair(
            funcRunString(file),
            (envs.at(funcRunString))(file)
        );
    } else {
        return std::make_pair(
            funcRunString(file),
            empty
        );
    }
}

QString tools::tipShortcut(const QKeySequence &ks){
    return QString(" (%1)").arg(ks.toString(QKeySequence::NativeText));
}

void tools::buildToolTip(const QString& tip, QAbstractButton *button,  QAction *action){
    if (button == nullptr) return;
    QKeySequence ks = button->shortcut();
    if (action != nullptr) {
        action->setToolTip(tip);
        ks = action->shortcut();
    }
    if(!ks.isEmpty()) button->setToolTip(tip + tipShortcut(ks));
    else button->setToolTip(tip);
}
