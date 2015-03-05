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

    Tp::BaseProtocolPtr proto = Tp::BaseProtocol::create<SimpleProtocol>(QLatin1String("simplecm"));
    Tp::BaseConnectionManagerPtr cm = Tp::BaseConnectionManager::create(QLatin1String("simplecm"));

    proto->setEnglishName(QLatin1String("SimpleCM"));
    proto->setIconName(QLatin1String("simplecm"));

    cm->addProtocol(proto);
    cm->registerObject();

    return app.exec();
}
