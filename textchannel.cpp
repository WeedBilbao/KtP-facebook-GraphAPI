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

#include "textchannel.h"

#include <TelepathyQt/Constants>
#include <TelepathyQt/RequestableChannelClassSpec>
#include <TelepathyQt/RequestableChannelClassSpecList>
#include <TelepathyQt/Types>

#include <QLatin1String>
#include <QVariantMap>
#include <QtWebKit>
#include <QtWebKitWidgets>
#include <QMouseEvent> 	
#include <QApplication>

#include <QDebug>

SimpleTextChannel::SimpleTextChannel(Tp::BaseChannel *baseChannel, uint targetHandle, const QString &identifier)
    : Tp::BaseChannelTextType(baseChannel),
      m_identifier(identifier)
{
    QStringList supportedContentTypes = QStringList() << QLatin1String("text/plain");
    Tp::UIntList messageTypes = Tp::UIntList() << Tp::ChannelTextMessageTypeNormal;

    uint messagePartSupportFlags = 0;
    uint deliveryReportingSupport = 0;

    m_messagesIface = Tp::BaseChannelMessagesInterface::create(this,
                      supportedContentTypes,
                      messageTypes,
                      messagePartSupportFlags,
                      deliveryReportingSupport);

    baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(m_messagesIface));

    m_messagesIface->setSendMessageCallback(Tp::memFun(this, &SimpleTextChannel::sendMessageCallback));
}

SimpleTextChannelPtr SimpleTextChannel::create(Tp::BaseChannel *baseChannel, uint targetHandle, const QString &identifier)
{
    return SimpleTextChannelPtr(new SimpleTextChannel(baseChannel, targetHandle, identifier));
}

SimpleTextChannel::~SimpleTextChannel()
{
}

QString SimpleTextChannel::sendMessageCallback(const Tp::MessagePartList &messageParts, uint flags, Tp::DBusError *error)
{
    QString content;
    for (Tp::MessagePartList::const_iterator i = messageParts.begin()+1; i != messageParts.end(); ++i) {
        if(i->count(QLatin1String("content-type"))
                && i->value(QLatin1String("content-type")).variant().toString() == QLatin1String("text/plain")
                && i->count(QLatin1String("content")))
        {
            content = i->value(QLatin1String("content")).variant().toString();
            break;
        }
    }
   
    QWebView::connect(&qWebView, SIGNAL(loadFinished(bool)), this, SLOT(on_pageLoad_finished(bool)));
    QUrl url("https://www.facebook.com");
    qWebView.load(url);
    qWebView.show();
    
    
//     QWebElementCollection collection = view->findAllElements("textarea[name=message_body]");
//     foreach (QWebElement element, collection)
//         element.setAttribute("value", "Prueba de mandar mensajesss!! Vamos, que funciona! :D");
//     // find search button
//     QWebElementCollection sendButton = view->findAllElements("input[id=u_0_r]");
//     QMouseEvent event0(QEvent::MouseButtonPress, sendButton.at(0).geometry().center(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    

    emit messageReceived(m_identifier, content);

    return QString();
}

void SimpleTextChannel::on_pageLoad_finished(bool ok){
    
    QWebElementCollection email = qWebView.page()->currentFrame()->findAllElements("input[id=\"email\"]");
    email[0].setAttribute("value", ""); //email
    QWebElementCollection pass = qWebView.page()->currentFrame()->findAllElements("input[id=\"pass\"]");
    pass[0].setAttribute("value", ""); //password
    int pixelsToScrolRight=300;
    qWebView.page()->currentFrame()->setScrollBarValue(Qt::Horizontal,pixelsToScrolRight);
    QWebElementCollection sendButton = qWebView.page()->currentFrame()->findAllElements("type[id=submit");
    QMouseEvent pressEvent(QEvent::MouseButtonPress, sendButton[0].geometry().center(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    sendButton[0].evaluateJavaScript("this.click()");
    
    
}

void SimpleTextChannel::on_pageLoad_finished2(bool ok){
    
    QWebElementCollection collection = qWebView.page()->currentFrame()->findAllElements("input[name=message_body]");
    foreach (QWebElement element, collection)
        element.setAttribute("value", "Prueba de mandar mensajesss!! Vamos, que funciona! :D");
    // find search button
    QWebElementCollection sendButton = qWebView.page()->currentFrame()->findAllElements("input[id=u_0_r]");
    QMouseEvent event0(QEvent::MouseButtonPress, sendButton.at(0).geometry().center(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
}
