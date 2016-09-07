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

    for (int i = 0; i < topics_.length(); i++)
    {
        socket_->subscribeTo(topics_.at(i));
        qDebug() << "Subscribed to " << topics_.at(i);
    }

    for (int i = 0; i < addresses_.length(); i++)
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

    fish_data["fish-001"] = FishData();
    fish_data["fish-002"] = FishData();
    fish_data["fish-003"] = FishData();
    fish_data["fish-004"] = FishData();
    fish_data["fish-005"] = FishData();

    fish_data["ribot-001"] = FishData();
    fish_data["ribot-002"] = FishData();
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
            for (int i = 0; i < ranges.raw_value_size(); i++)
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
    else if (name == "FishPosition")
    {
        QString id("fish-00");
        id.append(message.at(1));
        if (fish_data.find(id) != fish_data.end())
        {
            fish_data[id].appendPos(message.at(2).toDouble(),message.at(3).toDouble());
        }
    }
    else if (name == "CASUPosition")
    {
        QString id("ribot-00");
        id.append(message.at(1));
        if (ribot_data.find(id) != fish_data.end())
        {
            ribot_data[id].appendPos(message.at(2).toDouble(),message.at(3).toDouble());
        }
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
    : direction(1)
{
    x.push_front(100.0);
    x.push_front(100.0);
    y.push_front(100.0);
    y.push_front(100.0);
}

void Subscriber::FishData::appendPos(double xk, double yk)
{
    x.push_front(xk);
    y.push_front(yk);

    if (x.size() > buff_max)
    {
        x.pop_back();
        y.pop_back();
    }

    // Create ractangle for rendering the fish
    pose.setRect(x.at(0), y.at(0), )
    // TODO: compute swimming direction
}
