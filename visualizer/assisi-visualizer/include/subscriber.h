#ifndef SUBSCRIBER_H
#define SUBSCRIBER_H

#include <QObject>
#include <nzmqt/nzmqt.hpp>

#include <map>
class Subscriber : public QObject
{
    Q_OBJECT
public:
    explicit Subscriber(const QList<QString>& addresses,
                        const QList<QString>& topics,
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

    //! Struct for holding fish data
    struct FishData
    {
        //! Initialize all values to reasonable defaults
        FishData();

        //! Append position
        /*!
         * Automatically updates the circular buffer of positions
         * and computes the motion direction.
         */
        void appendPos(double xk, double yk);

        QList<double> x;
        QList<double> y;
        //! +1 is CCW, -1 is CW
        double direction;
        int buff_max;
        // Rectangle for rendering the fish pose
        QRectF pose;
    };
    typedef std::map<QString,FishData> FishMap;

    //! Stores fish data
    /*! Everything is public, but
        Only the subscriber is supposed to write!
     */
    FishMap fish_data;
    FishMap ribot_data;

signals:
    void pingReceived(const QList<QByteArray>& message);

public slots:
    void messageReceived(const QList<QByteArray>& message);

private:

    // ZMQ connection details
    nzmqt::ZMQContext* context_;

    QList<QString> addresses_;
    QList<QString> topics_;

    nzmqt::ZMQSocket* socket_;

};

#endif // SUBSCRIBER_H
