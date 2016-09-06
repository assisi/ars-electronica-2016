#ifndef SUBSCRIBER_H
#define SUBSCRIBER_H

#include <QObject>
#include <nzmqt/nzmqt.hpp>

#include <map>
class Subscriber : public QObject
{
    Q_OBJECT
public:
    explicit Subscriber(const QString& address,
                        const QString& topic,
                        QObject *parent = 0);

    //! Struct for hodling CASU data
    struct CasuData
    {
        //! Initialize all values to reasonable defaults
        CasuData();

        double temp;
        double temp_ref;
        std::vector<double> ir_ranges;
        std::vector<double> ir_thresholds;
    };
    typedef std::map<std::string,CasuData> CasuMap;

    //! Stores Casu data
    /*! Everything is public, but
        Only the Subscriber is supposed to write!
     */
    CasuMap casu_data;

signals:
    void pingReceived(const QList<QByteArray>& message);

public slots:
    void messageReceived(const QList<QByteArray>& message);

private:

    // ZMQ connection details
    nzmqt::ZMQContext* context_;

    QString address_;
    QString topic_;

    nzmqt::ZMQSocket* socket_;

};

#endif // SUBSCRIBER_H
