/*
    Copyright (C) 2014 Alexandr Akulich <akulichalexander@gmail.com>

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
    LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
    OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include "connection.h"

#include "textchannel.h"

#include <TelepathyQt/Constants>
#include <TelepathyQt/BaseChannel>

#include <QDateTime>

#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <iostream>
using namespace std;

#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QJsonArray>
#include <QThread>


#include <QDebug>

Tp::SimpleStatusSpecMap SimpleConnection::getSimpleStatusSpecMap()
{
    //Presence
    Tp::SimpleStatusSpec spAvailable;
    spAvailable.type = Tp::ConnectionPresenceTypeAvailable;
    spAvailable.maySetOnSelf = false;
    spAvailable.canHaveMessage = true;

    Tp::SimpleStatusSpec spOffline;
    spOffline.type = Tp::ConnectionPresenceTypeOffline;
    spOffline.maySetOnSelf = false;
    spOffline.canHaveMessage = false;

    Tp::SimpleStatusSpec spUnknown;
    spUnknown.type = Tp::ConnectionPresenceTypeUnknown;
    spUnknown.maySetOnSelf = false;
    spUnknown.canHaveMessage = false;

    Tp::SimpleStatusSpecMap specs;
    specs.insert(QLatin1String("available"), spAvailable);
    specs.insert(QLatin1String("offline"), spOffline);
    specs.insert(QLatin1String("unknown"), spUnknown);
    return specs;
}

SimpleConnection::SimpleConnection(const QDBusConnection &dbusConnection, const QString &cmName, const QString &protocolName, const QVariantMap &parameters) :
    Tp::BaseConnection(dbusConnection, cmName, protocolName, parameters)
{
    /* Connection.Interface.Contacts */
    contactsIface = Tp::BaseConnectionContactsInterface::create();
    contactsIface->setGetContactAttributesCallback(Tp::memFun(this, &SimpleConnection::getContactAttributes));
    contactsIface->setContactAttributeInterfaces(QStringList()
            << TP_QT_IFACE_CONNECTION
            << TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST
            << TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE
            << TP_QT_IFACE_CONNECTION_INTERFACE_ALIASING);
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(contactsIface));

    /* Connection.Interface.SimplePresence */
    simplePresenceIface = Tp::BaseConnectionSimplePresenceInterface::create();
    simplePresenceIface->setSetPresenceCallback(Tp::memFun(this,&SimpleConnection::setPresence));
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(simplePresenceIface));

    /* Connection.Interface.ContactList */
    contactListIface = Tp::BaseConnectionContactListInterface::create();
    contactListIface->setGetContactListAttributesCallback(Tp::memFun(this, &SimpleConnection::getContactListAttributes));
//    contactListIface->setRequestSubscriptionCallback(Tp::memFun(this, &SimpleConnection::requestSubscription));
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(contactListIface));

    aliasingIface = Tp::BaseConnectionAliasingInterface::create();
    aliasingIface->setGetAliasesCallback(Tp::memFun(this, &SimpleConnection::getAliases));
    //aliasingIface->setSetAliasesCallback(Tp::memFun(this, &SimpleConnection::setAliases));
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(aliasingIface));

    /* Connection.Interface.Requests */
    requestsIface = Tp::BaseConnectionRequestsInterface::create(this);
    /* Fill requestableChannelClasses */
    Tp::RequestableChannelClass text;
    text.fixedProperties[TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")] = TP_QT_IFACE_CHANNEL_TYPE_TEXT;
    text.fixedProperties[TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType")]  = Tp::HandleTypeContact;
    text.allowedProperties.append(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"));
    text.allowedProperties.append(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"));
    requestsIface->requestableChannelClasses << text;
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(requestsIface));

    if (parameters.contains(QLatin1String("self_id"))) {
        m_selfId = parameters.value(QLatin1String("self_id")).toString();
    }

    setSelfHandle(addContact(m_selfId));

    setConnectCallback(Tp::memFun(this, &SimpleConnection::connectCallback));
    setInspectHandlesCallback(Tp::memFun(this, &SimpleConnection::inspectHandles));
    setCreateChannelCallback(Tp::memFun(this, &SimpleConnection::createChannel));
    setRequestHandlesCallback(Tp::memFun(this, &SimpleConnection::requestHandles));
}

void SimpleConnection::onResult(QNetworkReply* reply)
{
    QString data = (QString) reply->readAll();

    QJsonDocument jsonResponse = QJsonDocument::fromJson(data.toUtf8());
    QJsonObject jsonObject = jsonResponse.object();

    myContanctsList.clear();
    myContanctsListAliases.clear();


    QJsonArray jsonArray = jsonObject["data"].toArray();

    foreach (const QJsonValue & value, jsonArray) {
        QJsonObject obj = value.toObject();

        QJsonObject participants = obj["participants"].toObject();
        QJsonArray participants_data = participants["data"].toArray();

        foreach (const QJsonValue &data_element, participants_data) {
	    QString inner_id = data_element.toObject()["id"].toString();
            QString inner_name = data_element.toObject()["name"].toString();
            if (inner_id != selfID)
            {
                myContanctsList.append(inner_id);
                myContanctsListAliases.append(inner_name);
            }
        }
    }

    QList<uint> handles;
    
    for (int i = 0; i < myContanctsList.count(); ++i) {
        handles.append(ensureContact(myContanctsList.at(i)));
    }

    setSubscriptionState(myContanctsList, handles, Tp::SubscriptionStateYes);
    updateContactsState(myContanctsList);
    contactListIface->setContactListState(Tp::ContactListStateSuccess);
}

void SimpleConnection::onSelfResult(QNetworkReply* reply)
{
    QString data = (QString) reply->readAll();

    QJsonDocument jsonResponse = QJsonDocument::fromJson(data.toUtf8());
    QJsonObject jsonObject = jsonResponse.object();
    selfID = jsonObject["id"].toString();
    qDebug() << "MI ID" << selfID;

}

SimpleConnection::~SimpleConnection()
{
}

void SimpleConnection::connectCallback(Tp::DBusError *error)
{
    setStatus(Tp::ConnectionStatusConnecting, Tp::ConnectionStatusReasonRequested);

    simplePresenceIface->setStatuses(getSimpleStatusSpecMap());

    Tp::SimpleContactPresences presences;
    Tp::SimplePresence presence;
    presence.status = QLatin1String("available");
    presence.statusMessage = QString();
    presence.type = Tp::ConnectionPresenceTypeAvailable;
    presences[selfHandle()] = presence;
    simplePresenceIface->setPresences(presences);
    qDebug() << "Paso por el connectCallback";
    setContactList(myContanctsList);

    setStatus(Tp::ConnectionStatusConnected, Tp::ConnectionStatusReasonRequested);

    /* Set ContactList status */
    contactListIface->setContactListState(Tp::ContactListStateSuccess);
}

QStringList SimpleConnection::inspectHandles(uint handleType, const Tp::UIntList &handles, Tp::DBusError *error)
{
    qDebug() << Q_FUNC_INFO;

    if (handleType != Tp::HandleTypeContact) {
        error->set(TP_QT_ERROR_INVALID_ARGUMENT, QLatin1String("Unsupported handle type"));
        return QStringList();
    }

    QStringList result;

    foreach (uint handle, handles) {
        if (!m_handles.contains(handle)) {
            return QStringList();
        }

        result.append(m_handles.value(handle));
    }

    return result;
}

Tp::BaseChannelPtr SimpleConnection::createChannel(const QString &channelType, uint targetHandleType, uint targetHandle, const QVariantMap &request, Tp::DBusError *error)
{
    qDebug() << "SimpleConnection::createChannel " << channelType
             << " " << targetHandleType
             << " " << targetHandle;

    if ((targetHandleType != Tp::HandleTypeContact) || (targetHandle == 0)) {
        error->set(TP_QT_ERROR_INVALID_HANDLE, QLatin1String("createChannel error"));
        return Tp::BaseChannelPtr();
    }

    Tp::BaseChannelPtr baseChannel = Tp::BaseChannel::create(this, channelType, targetHandle, targetHandleType);

    QString identifier = m_handles.value(targetHandle);

    if (channelType == TP_QT_IFACE_CHANNEL_TYPE_TEXT) {
        SimpleTextChannelPtr textType = SimpleTextChannel::create(baseChannel.data(), targetHandle, identifier);
        qDebug() << "Text interface is called " << textType->interfaceName();
        baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(textType));
        connect(textType.data(), SIGNAL(messageReceived(QString,QString)), SIGNAL(messageReceived(QString,QString)));
    }

    return baseChannel;
}

Tp::UIntList SimpleConnection::requestHandles(uint handleType, const QStringList &identifiers, Tp::DBusError *error)
{
    qDebug() << Q_FUNC_INFO << identifiers;

    Tp::UIntList result;

    if (handleType != Tp::HandleTypeContact) {
        error->set(TP_QT_ERROR_INVALID_ARGUMENT, QLatin1String("SimpleConnection::requestHandles - Handle Type unknown"));
        return result;
    }

    foreach(const QString &identify, identifiers) {
        uint handle = m_handles.key(identify, 0);
        if (handle) {
            result.append(handle);
        } else {
            result.append(addContact(identify));
        }
    }

    return result;
}

Tp::ContactAttributesMap SimpleConnection::getContactListAttributes(const QStringList &interfaces, bool hold, Tp::DBusError *error)
{
    qDebug() << Q_FUNC_INFO << "ATR";

    Tp::ContactAttributesMap contactAttributes;

    foreach (const uint handle, m_handles.keys()) {
        if (handle == selfHandle()) {
            continue;
        }
        QVariantMap attributes;
        attributes[TP_QT_IFACE_CONNECTION + QLatin1String("/contact-id")] = m_handles.value(handle);
        attributes[TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST + QLatin1String("/subscribe")] = Tp::SubscriptionStateYes;
        attributes[TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST + QLatin1String("/publish")] = Tp::SubscriptionStateYes;
        attributes[TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE + QLatin1String("/presence")] = QVariant::fromValue(getPresence(handle));
        contactAttributes[handle] = attributes;
    }
    return contactAttributes;

//     Q_UNUSED(hold);
//
//     Tp::UIntList handles = m_handles.keys();
//     handles.removeOne(selfHandle());
//
//     return getContactAttributes(handles, interfaces, error);

}

Tp::ContactAttributesMap SimpleConnection::getContactAttributes(const Tp::UIntList &handles, const QStringList &interfaces, Tp::DBusError *error)
{
//    Connection.Interface.Contacts
//    http://telepathy.freedesktop.org/spec/Connection_Interface_Contacts.html#Method:GetContactAttributes
    qDebug() << Q_FUNC_INFO << handles << interfaces;

    Tp::ContactAttributesMap contactAttributes;

    foreach (const uint handle, handles) {
        if (m_handles.contains(handle)) {
            QVariantMap attributes;
            attributes[TP_QT_IFACE_CONNECTION + QLatin1String("/contact-id")] = m_handles.value(handle);

            if (interfaces.contains(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST)) {
                attributes[TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST + QLatin1String("/subscribe")] = Tp::SubscriptionStateYes;
                attributes[TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST + QLatin1String("/publish")] = Tp::SubscriptionStateYes;
            }

            if (interfaces.contains(TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE)) {
                attributes[TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE + QLatin1String("/presence")] = QVariant::fromValue(getPresence(handle));
            }

            if (interfaces.contains(TP_QT_IFACE_CONNECTION_INTERFACE_ALIASING)) {
                attributes[TP_QT_IFACE_CONNECTION_INTERFACE_ALIASING + QLatin1String("/alias")] = QVariant::fromValue(getAlias(handle-1));
            }

            contactAttributes[handle] = attributes;
        }
    }
    return contactAttributes;
}

Tp::SimplePresence SimpleConnection::getPresence(uint handle)
{
    if (!m_presences.contains(handle)) {
        return Tp::SimplePresence();
    }

    return m_presences.value(handle);
}

uint SimpleConnection::setPresence(const QString &status, const QString &message, Tp::DBusError *error)
{
    qDebug() << Q_FUNC_INFO << "not implemented";
    return 0;
}

uint SimpleConnection::ensureContact(const QString &identifier)
{
    uint handle = getHandle(identifier);
    if (!handle) {
        handle = addContact(identifier);
    }
    return handle;
}

uint SimpleConnection::addContacts(const QStringList &identifiers)
{
    qDebug() << Q_FUNC_INFO;
    uint handle = 0;

    if (!m_handles.isEmpty()) {
        handle = m_handles.keys().last();
    }

    QList<uint> newHandles;
    foreach(const QString &identifier, identifiers) {
        ++handle;
        m_handles.insert(handle, identifier);
        newHandles << handle;
    }

    setPresenceState(newHandles, QLatin1String("available"));

    setSubscriptionState(identifiers, newHandles, Tp::SubscriptionStateYes);
    updateContactsState(identifiers);

    contactListIface->setContactListState(Tp::ContactListStateSuccess);

    return handle;
}

uint SimpleConnection::addContact(const QString &identifier)
{
    qDebug() << Q_FUNC_INFO;
    return addContacts(QStringList() << identifier);
}

void SimpleConnection::setPresenceState(const QList<uint> &handles, const QString &status)
{
    qDebug() << Q_FUNC_INFO;
    Tp::SimpleContactPresences newPresences;
    const static Tp::SimpleStatusSpecMap statusSpecMap = getSimpleStatusSpecMap();
    foreach (uint handle, handles) {
        uint type = 0;
        if (statusSpecMap.contains(status)) {
            type = statusSpecMap.value(status).type;
        }

        Tp::SimplePresence presence;
        presence.status = status;
//        presence.statusMessage;
        presence.type = type;
        m_presences[handle] = presence;
        newPresences[handle] = presence;
    }
    simplePresenceIface->setPresences(newPresences);
}

void SimpleConnection::setSubscriptionState(const QStringList &identifiers, const QList<uint> &handles, uint state)
{
    qDebug() << Q_FUNC_INFO;
    Tp::ContactSubscriptionMap changes;
    Tp::HandleIdentifierMap identifiersMap;

    for(int i = 0; i < identifiers.size(); ++i) {
        Tp::ContactSubscriptions change;
        change.publish = Tp::SubscriptionStateYes;
        change.publishRequest = QString();
        change.subscribe = state;
        changes[handles[i]] = change;
        identifiersMap[handles[i]] = identifiers[i];
        m_contactsSubscription[handles[i]] = state;
    }
    Tp::HandleIdentifierMap removals;
    contactListIface->contactsChangedWithID(changes, identifiersMap, removals);
}


//Clase que verifica el estado de cada contacto
void SimpleConnection::updateContactsState(const QStringList &identifiers)
{
    qDebug() << Q_FUNC_INFO;
    Tp::SimpleContactPresences newPresences;
    foreach (const QString &fbID, identifiers) {
        uint handle = ensureContact(fbID);

        Tp::SimplePresence presence;


        presence.status = QLatin1String("available");
        presence.type = Tp::ConnectionPresenceTypeAvailable;


        m_presences[handle] = presence;
        newPresences[handle] = presence;
    }
    simplePresenceIface->setPresences(newPresences);
}

/* Receive message from someone to ourself */
void SimpleConnection::receiveMessage(const QString &sender, const QString &message)
{
    uint senderHandle, targetHandle;

    Tp::HandleType handleType = Tp::HandleTypeContact;
    senderHandle = targetHandle = ensureContact(sender);

    //TODO: initiator should be group creator
    Tp::DBusError error;
    bool yours;
    Tp::BaseChannelPtr channel = ensureChannel(TP_QT_IFACE_CHANNEL_TYPE_TEXT, handleType, targetHandle, yours,
                                 senderHandle,
                                 false, QVariantMap(), &error);
    if (error.isValid()) {
        qWarning() << "ensureChannel failed:" << error.name() << " " << error.message();
        return;
    }

    Tp::BaseChannelTextTypePtr textChannel = Tp::BaseChannelTextTypePtr::dynamicCast(channel->interface(TP_QT_IFACE_CHANNEL_TYPE_TEXT));
    if (!textChannel) {
        qDebug() << "Error, channel is not a textChannel??";
        return;
    }

    uint timestamp = QDateTime::currentMSecsSinceEpoch() / 1000;

    Tp::MessagePartList body;
    Tp::MessagePart text;
    text[QLatin1String("content-type")] = QDBusVariant(QLatin1String("text/plain"));
    text[QLatin1String("content")]      = QDBusVariant(message);
    body << text;

    Tp::MessagePartList partList;
    Tp::MessagePart header;
    header[QLatin1String("message-received")]  = QDBusVariant(timestamp);
    header[QLatin1String("message-sender")]    = QDBusVariant(senderHandle);
    header[QLatin1String("message-sender-id")] = QDBusVariant(sender);
    header[QLatin1String("message-type")]      = QDBusVariant(Tp::ChannelTextMessageTypeNormal);

    partList << header << body;
    textChannel->addReceivedMessage(partList);
}

void SimpleConnection::setContactList(const QStringList &identifiers)
{
    QUrl url("https://graph.facebook.com/v2.2/me/conversations?access_token=CAAXDBLOkIkIBAIuDENAHRdldNhDZA2ZBDHeyVvKEXmRBFQRHXsXJiT5KHDBKxIXakZCq7p0WfZB28f64jZCZAnbiG9PSdAm2wd0PZBQ492ZByzwDXNpp7qagSZAww6DqbmlsErqZCSvzvCiAONy1W4AZABZBchke1Bu0tvD9ZCBhZAhvpFeXIbanED7YZB1pqv0spCFwcSo3JDLkGFA73jfiQyZC9f6r");
    QUrl selfUrl("https://graph.facebook.com/v2.2/me?access_token=CAAXDBLOkIkIBAIuDENAHRdldNhDZA2ZBDHeyVvKEXmRBFQRHXsXJiT5KHDBKxIXakZCq7p0WfZB28f64jZCZAnbiG9PSdAm2wd0PZBQ492ZByzwDXNpp7qagSZAww6DqbmlsErqZCSvzvCiAONy1W4AZABZBchke1Bu0tvD9ZCBhZAhvpFeXIbanED7YZB1pqv0spCFwcSo3JDLkGFA73jfiQyZC9f6r");
    QNetworkRequest request;
    QNetworkRequest selfRequest;
    request.setUrl(url);
    selfRequest.setUrl(selfUrl);

    QNetworkReply* selfInfoReply = networkManager.get(selfRequest);
    connect(&networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onSelfResult(QNetworkReply*)));
    //sleep(100);
    QNetworkReply* currentReply = networkManager2.get(request);  // GET
    connect(&networkManager2, SIGNAL(finished(QNetworkReply*)), this, SLOT(onResult(QNetworkReply*)));

    qDebug() << "ESTOY AQIUIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII";


    // Actually it don't clear previous list (not implemented yet)
    addContacts(myContanctsList);
}

void SimpleConnection::setContactPresence(const QString &identifier, const QString &presence)
{
    uint handle = ensureContact(identifier);
    setPresenceState(QList<uint>() << handle, presence);

    // Let it be here until proper subscription implementation
    if (handle != selfHandle()) {
        setSubscriptionState(QStringList() << identifier, QList<uint>() << handle, Tp::SubscriptionStateYes);
    }
}

uint SimpleConnection::getHandle(const QString &identifier) const
{
    foreach (uint key, m_handles.keys()) {
        if (m_handles.value(key) == identifier) {
            return key;
        }
    }

    return 0;
}

Tp::AliasMap SimpleConnection::getAliases(const Tp::UIntList &handles, Tp::DBusError *error)
{
    qDebug() << Q_FUNC_INFO << handles;

    Tp::AliasMap aliases;

    foreach (uint handle, handles) {
        aliases[handle] = getAlias(handle);
    }

    return aliases;
}

QString SimpleConnection::getAlias(uint handle)
{
    const QString fbID = m_handles.value(handle);
    
    if (fbID.isEmpty()) {
        return QString();
    }
    
    if (handle > myContanctsListAliases.size())
        return "";
    return myContanctsListAliases.at(handle-1);
}
