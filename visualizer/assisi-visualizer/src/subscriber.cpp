#include "subscriber.h"
#include "dev_msgs.pb.h"

using namespace nzmqt;

Subscriber::Subscriber(const QList<QString>& addresses,
                     const QList<QString>& topics,
                     QObject *parent)
    : QObject(parent),
      msg_top(CasuMsg(600,200)),
      msg_bottom(CasuMsg(600,800)),
      msg_cats(CatsMsg(950,500)),
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
    casu_data["casu-001"].ir_thresholds[0] = 11300;
    casu_data["casu-001"].ir_thresholds[1] = 14500;
    casu_data["casu-001"].ir_thresholds[2] = 18500;
    casu_data["casu-001"].ir_thresholds[3] = 17600;
    casu_data["casu-001"].ir_thresholds[4] = 18500;
    casu_data["casu-001"].ir_thresholds[5] = 12000;

    casu_data["casu-002"] = CasuData();
    // Set thresholds
    casu_data["casu-002"].ir_thresholds[0] = 15000;
    casu_data["casu-002"].ir_thresholds[1] = 13500;
    casu_data["casu-002"].ir_thresholds[2] = 20500;
    casu_data["casu-002"].ir_thresholds[3] = 12500;
    casu_data["casu-002"].ir_thresholds[4] = 15500;
    casu_data["casu-002"].ir_thresholds[5] = 13500;

    fish_data["fish-000"] = FishData();
    fish_data["fish-001"] = FishData();
    fish_data["fish-002"] = FishData();
    fish_data["fish-003"] = FishData();
    fish_data["fish-004"] = FishData();
    //fish_data["fish-005"] = FishData();

    ribot_data["ribot-000"] = FishData();
    //ribot_data["ribot-001"] = FishData();
    //ibot_data["ribot-002"] = FishData();
    //ribot_data["ribot-003"] = FishData();
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
                if (static_cast<unsigned>(i) >= casu_data[name].ir_ranges.size()) break;
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
        else if (device == "CommEth")
        {
            QString msg_str = QString::fromUtf8(message.at(3).constData(), message.at(3).length());
            QStringList parts(msg_str.split(','));
            QStringList fish_dir(parts.at(0).split(':').at(1));//CW/CCW
            QStringList ribot_dir(parts.at(1).split(':').at(1));
            if (fish_dir.length() > 0 && ribot_dir.length() > 0)
            {
                msg_cats.incoming(fish_dir.at(0), ribot_dir.at(0));
            }
        }
        emit pingReceived(message);
    }
    else if (name == "cats")
    {
        QString sender(message.at(2));
        bool ok = false;
        if (sender == "casu-001")
        {
            double val = message.at(3).toDouble(&ok)*6;
            if (ok)
            {
                msg_top.incoming(val);
            }
        }
        else if (sender == "casu-002")
        {
            double val = message.at(3).toDouble(&ok)*6;
            if (ok)
            {
                msg_bottom.incoming(val);
            }
        }
    }
    else if (name == "FishPosition")
    {
        QString id("fish-00");
        id.append(message.at(1));
        bool ok_x = false;
        bool ok_y = false;
        if (fish_data.find(id) != fish_data.end())
        {
            double x = message.at(2).toDouble(&ok_x);
            double y = message.at(3).toDouble(&ok_y);
            if (ok_x && ok_y)
            {
                fish_data[id].appendPos(x,y);
            }
        }
    }
    else if (name == "CASUPosition")
    {
        QString id("ribot-00");
        id.append(message.at(1));
        bool ok_x = false;
        bool ok_y = false;
        if (ribot_data.find(id) != ribot_data.end())
        {
            double x = message.at(2).toDouble(&ok_x);
            double y = message.at(3).toDouble(&ok_y);
            if (ok_x && ok_y)
            {
                ribot_data[id].appendPos(x,y,100,30);
            }
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
    : direction(1),
      buff_max(10),
      tank_scale_x(440.0/500.0),
      tank_scale_y(900.0/500.0),
      tank_offset_x(1055),
      tank_offset_y(50)
{
    appendPos(100.0,100.0);
    appendPos(100.0,100.0);
}

void Subscriber::FishData::appendPos(double xk, double yk, double w, double h)
{
    x.push_front(xk*tank_scale_x+tank_offset_x);
    y.push_front(yk*tank_scale_y+tank_offset_y);

    if (x.size() > buff_max)
    {
        x.pop_back();
        y.pop_back();
    }

    // Create ractangle for rendering the fish
    pose.setRect(x.at(0)-w/2.0, y.at(0)-h/2.0, w, h);
    // TODO: compute swimming direction
}

Subscriber::CasuMsg::CasuMsg(int kx0, int ky, int kw, int kh)
    : count(0),
      x0(kx0),
      x(kx0),
      y(ky),
      w(kw),
      h(kh),
      dx(4),
      x_max(1000),
      active(false)
{

}

void Subscriber::CasuMsg::incoming(int m)
{
    if (!active)
    {
        count = m;
        active = true;
    }
    // Do not accept new messages while we are active
}

void Subscriber::CasuMsg::update(void)
{
    if (active)
    {
        x += dx;
    }

    if (x >= x_max)
    {
        x = x0;
        active = false;
    }
    pose.setRect(x-w/2.0,y-h/2.0,w,h);
}

Subscriber::CatsMsg::CatsMsg(int kx0, int ky0, int kw, int kh)
    : fish_direction(1),
      ribot_direction(1),
      x0(kx0),
      y0(ky0),
      x(kx0),
      y_top(ky0),
      y_bot(ky0),
      rot_fish(0),
      rot_ribot(0),
      w(kw),
      h(kh),
      dx(-4),
      dy(3),
      drot(-12),
      x_mid(800),
      x_min(600),
      active(false)
{

}

void Subscriber::CatsMsg::incoming(QString fish_dir, QString ribot_dir)
{
    if (!active)
    {
        fish_direction = dir_to_int(fish_dir);
        ribot_direction = dir_to_int(ribot_dir);
        active = true;
    }
    // Do not accept new messages while we are active
}

void Subscriber::CatsMsg::update(void)
{
    if (active)
    {
        x += dx;
        if (x <= x_mid)
        {
            y_top += dy;
            y_bot -= dy;
        }
        rot_fish += fish_direction * drot;
        rot_ribot += ribot_direction * drot;
    }

    if (x <= x_min)
    {
        active = false;
        x = x0;
        y_top = y0;
        y_bot = y0;
        rot_fish = 0;
        rot_ribot = 0;
    }
    pose_top.setRect(x-w/2.0,y_top-h/2.0,w,h);
    pose_bot.setRect(x-w/2.0,y_bot-h/2.0,w,h);
    ribot_dir_top.setRect(x-w/2.0,y_top-h/2.0,w/2.0,h);
    ribot_dir_bot.setRect(x-w/2.0,y_bot-h/2.0,w/2.0,h);
    fish_dir_top.setRect(x,y_top-h/2.0,w/2.0,h);
    fish_dir_bot.setRect(x,y_bot-h/2.0,w/2.0,h);
}

int Subscriber::CatsMsg::dir_to_int(QString dir)
{
    int result = 1;
    if (dir == "CCW")
    {
        result = 1;
    }
    else if (dir == "CW")
    {
        result = -1;
    }
    return result;
}
