#ifndef SUBSCRIBER_H
#define SUBSCRIBER_H

#include <QObject>
#include <nzmqt/nzmqt.hpp>

class Subscriber : public QObject
{
    Q_OBJECT
public:
    explicit Subscriber(const QString& address,
                        const QString& topic,
                        QObject *parent = 0);

signals:
    void pingReceived(const QList<QByteArray>& message);

public slots:
    void messageReceived(const QList<QByteArray>& message);

private:
    nzmqt::ZMQContext* context_;

    QString address_;
    QString topic_;

    nzmqt::ZMQSocket* socket_;
};

#endif // SUBSCRIBER_H
