#include "subscriber.h"

using namespace nzmqt;

Subscriber::Subscriber(const QString& address,
                     const QString& topic,
                     QObject *parent)
    : QObject(parent),
      address_(address),
      topic_(topic),
      socket_(NULL)
{
    context_ = createDefaultContext(this);
    context_->start();

    socket_ = context_->createSocket(ZMQSocket::TYP_SUB, this);
    socket_->setObjectName("Subscriber.Socket.socket(SUB)");
    //connect(socket_, &ZMQSocket::messageReceived, this, &Subscriber::messageReceived);
    connect(socket_, SIGNAL(messageReceived(const QList<QByteArray>&)), SLOT(messageReceived(const QList<QByteArray>&)));

    socket_->subscribeTo(topic_);
    qDebug() << "Subscribed to " << topic_;

    socket_->connectTo(address_);
    qDebug() << "Connected socket to address " << address_;
}

void Subscriber::messageReceived(const QList<QByteArray>& message)
{
    qDebug() << "Subscriber> " << message;
    emit pingReceived(message);
}
