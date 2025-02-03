#include "speechtranslate.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    SpeechTranslate ai;
    ai.show();

    return app.exec();
}
