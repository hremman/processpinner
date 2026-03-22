#include "binexecutor.h"

#ifdef Q_OS_LINUX
const qint64 BinExecutor::offset = 0;
const QByteArray BinExecutor::magic = "ELF";
#else //Q_OS_WINDOWS
const qint64 BinExecutor::offset = 0;
const QByteArray BinExecutor::magic = "PE";
#endif

bool BinExecutor::detector(const QFileInfo & file){
    QFile fio(file.absoluteFilePath());
    fio.open(QFile::ReadOnly);
    fio.seek(offset);
    QByteArray test = fio.read(magic.size());
    return test == magic;
}
