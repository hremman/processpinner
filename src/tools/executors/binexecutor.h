#ifndef BINEXECUTOR_H
#define BINEXECUTOR_H

#include <QFileInfo>


class BinExecutor
{
public:
    static bool detector(const QFileInfo & file);
private:
    static const qint64 offset;
    static const QByteArray magic;
};

#endif // BINEXECUTOR_H
