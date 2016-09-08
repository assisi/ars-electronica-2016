#ifndef SUBSCRIBER_H
#define SUBSCRIBER_H

#include <QObject>
#include <QRectF>
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
         *
         * w, h are width and height values for rendering the obect.
         * defaults are for fish.
         */
        void appendPos(double xk, double yk, double w = 100, double h = 30);

        QList<double> x;
        QList<double> y;
        //! +1 is CCW, -1 is CW
        double direction;
        int buff_max;
        // Rectangle for rendering the fish pose
        QRectF pose;

        double tank_scale_x;
        double tank_scale_y;
        int tank_offset_x;
        int tank_offset_y;
    };
    typedef std::map<QString,FishData> FishMap;

    //! Stores fish data
    /*! Everything is public, but
        Only the subscriber is supposed to write!
     */
    FishMap fish_data;
    FishMap ribot_data;

    struct CasuMsg
    {
        CasuMsg(int kx0, int ky, int kw = 95, int kh = 95);
        void incoming(int m);
        void update(void);
        int count;
        double x0, x, y, w, h;
        double dx, x_min;
        bool active;
        QRectF pose;
    };
    CasuMsg msg_top;
    CasuMsg msg_bottom;

    struct CatsMsg
    {
        CatsMsg(int kx0, int ky0, int kw = 160, int kh = 80);
        void incoming(QString fish_dir, QString ribot_dir);
        void update(void);
        int dir_to_int(QString dir);
        int fish_direction;
        int ribot_direction;
        double x0, y0, x, y_top, y_bot, rot_fish, rot_ribot, w, h;
        double dx, dy, drot, x_mid, x_max;
        bool active;
        QRectF pose_top;
        QRectF pose_bot;
        QRectF ribot_dir_top;
        QRectF ribot_dir_bot;
        QRectF fish_dir_top;
        QRectF fish_dir_bot;
    };
    CatsMsg msg_cats;

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
