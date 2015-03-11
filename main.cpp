#include <QCoreApplication>

#include <TelepathyQt/BaseConnectionManager>
#include <TelepathyQt/Constants>
#include <TelepathyQt/Debug>

#include "protocol.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QLatin1String("telepathy-simplecm"));

    Tp::registerTypes();
    Tp::enableDebug(true);
    Tp::enableWarnings(true);

    Tp::BaseProtocolPtr proto = Tp::BaseProtocol::create<SimpleProtocol>(QLatin1String("simplecmFB"));
    Tp::BaseConnectionManagerPtr cm = Tp::BaseConnectionManager::create(QLatin1String("simplecmFB"));

    proto->setEnglishName(QLatin1String("SimpleCMFB"));
    proto->setIconName(QLatin1String("simplecmFB"));

    cm->addProtocol(proto);
    cm->registerObject();

    return app.exec();
}
