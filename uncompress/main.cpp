#include <QCoreApplication>
#include <QFile>
#include <QByteArray>
#include <iostream>

int main(int argc, char *argv[])
{
    if (argc < 2)
        return 1;
    QFile in(argv[1]);
    in.open(QIODevice::ReadOnly);
    std::cout << qUncompress(in.readAll()).data();
    in.close();
    return 0;
}
