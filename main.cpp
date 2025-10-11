#include "mainwindow.h"
#include <QLoggingCategory>

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QLoggingCategory::setFilterRules("qt.multimedia.ffmpeg.encoder=false");

    MainWindow ai;
    ai.show();

    return app.exec();
}
