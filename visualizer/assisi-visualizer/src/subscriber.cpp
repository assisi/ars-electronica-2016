#include "subscriber.h"
#include "dev_msgs.pb.h"

using namespace nzmqt;

Subscriber::Subscriber(const QList<QString>& addresses,
                     const QList<QString>& topics,
                     QObject *parent)
    : QObject(parent),
      addresses_(addresses),
      topics_(topics),
      socket_(NULL)
{
    context_ = createDefaultContext(this);
    context_->start();

    socket_ = context_->createSocket(ZMQSocket::TYP_SUB, this);
    socket_->setObjectName("Subscriber.Socket.socket(SUB)");
    connect(socket_, &ZMQSocket::messageReceived, this, &Subscriber::messageReceived);

    for (unsigned i = 0; i < topics_.length(); i++)
    {
        socket_->subscribeTo(topics_.at(i));
        qDebug() << "Subscribed to " << topics_.at(i);
    }

    for (unsigned i = 0; i < addresses_.length(); i++)
    {
        socket_->connectTo(addresses_.at(i));
        qDebug() << "Connected socket to address " << addresses_.at(i);
    }

    casu_data["casu-001"] = CasuData();
    // Set thresholds
    casu_data["casu-001"].ir_thresholds[0] = 12000;
    casu_data["casu-001"].ir_thresholds[1] = 12000;
    casu_data["casu-001"].ir_thresholds[2] = 12000;
    casu_data["casu-001"].ir_thresholds[3] = 12000;
    casu_data["casu-001"].ir_thresholds[4] = 12000;
    casu_data["casu-001"].ir_thresholds[5] = -12000;

    casu_data["casu-002"] = CasuData();
    // Set thresholds
    casu_data["casu-002"].ir_thresholds[0] = 12000;
    casu_data["casu-002"].ir_thresholds[1] = 12000;
    casu_data["casu-002"].ir_thresholds[2] = 12000;
    casu_data["casu-002"].ir_thresholds[3] = 12000;
    casu_data["casu-002"].ir_thresholds[4] = -12000;
    casu_data["casu-002"].ir_thresholds[5] = 12000;
}

void Subscriber::messageReceived(const QList<QByteArray>& message)
{
    std::string name(message.at(0).constData(), message.at(0).length());
    if (casu_data.find(name) != casu_data.end())
    {
        // Received message is from one of the CASUs
        std::string device(message.at(1).constData(), message.at(1).length());
        std::string data(message.at(3).constData(), message.at(3).length());
        if (device == "Temp")
        {
            // CASU temperature measurements
            AssisiMsg::TemperatureArray temps;
            temps.ParseFromString(data);
            casu_data[name].temp = temps.temp(7); // TEMP_WAX is #7
            //qDebug() << "Temperature> " << casu_data[name].temp;
        }
        else if (device == "Peltier")
        {
            // CASU temperature setpoint
            AssisiMsg::Temperature temp;
            temp.ParseFromString(data);
            casu_data[name].temp_ref = temp.temp();
            //qDebug() << "Peltier> " << temp;
        }
        else if (device == "IR")
        {
            // CASU IR readings
            AssisiMsg::RangeArray ranges;
            ranges.ParseFromString(data);
            for (unsigned i = 0; i < ranges.raw_value_size(); i++)
            {
                if (i >= casu_data[name].ir_ranges.size()) break;
                double raw = ranges.raw_value(i);
                if (raw > casu_data[name].ir_thresholds[i])
                {
                    casu_data[name].ir_ranges[i] = 2.0;
                }
                else
                {
                    casu_data[name].ir_ranges[i] = 0.0;
                }
            }
        }
        emit pingReceived(message);
    }
}

Subscriber::CasuData::CasuData(void)
    : temp(27),
      temp_ref(27),
      ir_ranges(6),
      ir_thresholds(6)
{
    for (unsigned i = 0; i < ir_ranges.size(); i++)
    {
        ir_ranges[i] = 0.0;
    }
}

Subscriber::FishData::FishData(void)
    : x(1),
      y(1),
      direction(1)
{

}
