#include "visualizer.h"
#include "ui_vassisi.h"
#include "subscriber.h"

#include <QPainter>
#include <QtSvg>

#include <cmath>

const double deg_to_rad = M_PI/180;

Visualizer::Visualizer(const QString &config_path, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VAssisi),
    default_scene_width_(1600),
    default_scene_height_(1000),
    td_(34) // 30 fps

{
    //TODO: Implement config file reading

    QList<QString> addresses;
    addresses.append("tcp://bbg-001:1555");
    addresses.append("tcp://bbg-001:2555");
    addresses.append("tcp://bbg-001:10101");
    addresses.append("tcp://bbg-001:10102");
    addresses.append("tcp://cats-workstation:10203");

    QList<QString> topics;
    topics.append("casu-001");
    topics.append("casu-002");
    topics.append("FishPosition");
    topics.append("CASUPosition");
    topics.append("cats");

    sub_ = new Subscriber(addresses,topics,this);
    /*
    fish_tank_outer_.setRect(80, 50, 480, 900);
    fish_tank_inner_.setRect(200, 170, 240, 660);

    bee_arena_.setRect(1040, 50, 480, 900);
    casu_top_.setRect(1220, 225, 100, 100);
    casu_bottom_.setRect(1220, 675, 100, 100);
    heating_area_top_ = casu_top_.marginsAdded(QMargins(100,100,100,100));
    heating_area_bottom_ = casu_bottom_.marginsAdded(QMargins(100,100,100,100));

    double_arrow_.setRect(800-200, 500-200, 400, 400);
    top_arrow_.setRect(800-200, 200-65, 400, 130);
    bottom_arrow_.setRect(800-200, 800-65, 400, 130);
    */

    // Flip everything, to match the physical layout
    fish_tank_outer_.setRect(1040, 50, 480, 900);
    fish_tank_inner_.setRect(1160, 170, 240, 660);

    bee_arena_.setRect(80, 50, 480, 900);
    casu_top_.setRect(260, 225, 100, 100);
    casu_bottom_.setRect(260, 675, 100, 100);
    heating_area_top_ = casu_top_.marginsAdded(QMargins(100,100,100,100));
    heating_area_bottom_ = casu_bottom_.marginsAdded(QMargins(100,100,100,100));

    double_arrow_.setRect(800-200, 500-200, 400, 400);
    top_arrow_.setRect(800-200, 200-65, 400, 130);
    bottom_arrow_.setRect(800-200, 800-65, 400, 130);

    ui->setupUi(this);

    svg_ = new QSvgRenderer(this);

    QTimer* timer = new QTimer(this);
    // Qt5 style connect does not work with overloaded functions
    //connect(timer, &QTimer::timeout, this, &QWidget::update);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(td_); // 30 FPS
}

Visualizer::~Visualizer()
{
    delete sub_;
    delete ui;
}

void Visualizer::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    // Scale all items
    double scaling_x = this->geometry().width()/default_scene_width_;
    double scaling_y = this->geometry().height()/default_scene_height_;
    painter.scale(scaling_x,
                  scaling_y);

    // Draw fish tank
    svg_->load(QString("://artwork/fisharena2.svg"));
    svg_->render(&painter, fish_tank_outer_);
    painter.drawRect(fish_tank_outer_);
    //painter.drawRect(fish_tank_inner_);

    // Draw fish
    if (sub_->msg_cats.fish_direction > 0)
    {
        svg_->load(QString("://artwork/fish-ccw.svg"));
    }
    else
    {
        svg_->load(QString("://artwork/fish-cw.svg"));
    }
    for (Subscriber::FishMap::iterator it = sub_->fish_data.begin(); it != sub_->fish_data.end(); it++)
    {
        svg_->render(&painter,it->second.pose);
    }

    // Draw ribot
    if (sub_->msg_cats.ribot_direction > 0)
    {
        svg_->load(QString("://artwork/ribot-ccw.svg"));
    }
    else
    {
        svg_->load(QString("://artwork/ribot-cw.svg"));
    }
    for (Subscriber::FishMap::iterator it = sub_->ribot_data.begin(); it != sub_->ribot_data.end(); it++)
    {
        svg_->render(&painter,it->second.pose);
    }

    // Draw bee arena
    svg_->load(QString("://artwork/beearena.svg"));
    svg_->render(&painter, bee_arena_);
    //painter.drawRect(bee_arena_);

    /* Draw CASU signals and bees */

    // Draw top casu heating area
    double r = heating_area_top_.height()/2.0;
    QRadialGradient grad_top(heating_area_top_.center(),r);
    QColor color = tempToColor(sub_->casu_data["casu-001"].temp);
    grad_top.setColorAt(0.0,color);
    grad_top.setColorAt(0.75,color);
    grad_top.setColorAt(1,QColor(251,239,191));
    painter.setBrush(grad_top);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(heating_area_top_);

    // Draw bottom casu heating area
    QRadialGradient grad_bottom(heating_area_bottom_.center(),r);
    color = tempToColor(sub_->casu_data["casu-002"].temp);
    grad_bottom.setColorAt(0.0,color);
    grad_bottom.setColorAt(0.75,color);
    grad_bottom.setColorAt(1,QColor(251,239,191));
    painter.setBrush(grad_bottom);
    painter.drawEllipse(heating_area_bottom_);

    // Draw top casu proximity readings
    // This is horrible, and probably works only for 0.0 and 2.0 readings :(
    painter.setBrush(QColor(150,150,150,100));
    unsigned num_readings = sub_->casu_data["casu-001"].ir_ranges.size();
    double fov = 360.0 / num_readings - 2;
    int h = heating_area_top_.height();
    for (unsigned i = 0; i < num_readings; i++)
    {
        QRect reading_area(heating_area_top_.topLeft(),heating_area_top_.bottomRight());
        // ir_ranges[i] = 0 should show no reading (margin equal to area size)
        // ir_ranges[i] = 2 is max reading (margin equal to 0.3 area size)
        int margin = 0.5*h - 0.5*h*0.5*sub_->casu_data["casu-001"].ir_ranges[i]*0.7;
        reading_area = reading_area.marginsRemoved(QMargins(margin, margin, margin, margin));
        painter.drawPie(reading_area,(60*i-fov/2)*16, fov*16);
        if (sub_->casu_data["casu-001"].ir_ranges[i] > 0.0)
        {
            // A bee has been detected, render it
            svg_->load(QString("://artwork/bee.svg"));
            double dx = reading_area.width()/2*cos(60*i*deg_to_rad);
            double dy = -reading_area.width()/2*sin(60*i*deg_to_rad);
            svg_->render(&painter,QRectF(casu_top_.topLeft(),QSizeF(93.0,65.0)).adjusted(dx,dy,dx,dy));
        }
    }

    // Draw bottom casu proximity readings
    // Again, this is absolutely horrible :(
    num_readings = sub_->casu_data["casu-002"].ir_ranges.size();
    fov = 360.0 / num_readings - 2;
    h = heating_area_bottom_.height();
    for (unsigned i = 0; i < num_readings; i++)
    {
        QRect reading_area(heating_area_bottom_.topLeft(),heating_area_bottom_.bottomRight());
        // ir_ranges[i] = 0 should show no reading (margin equal to area size)
        // ir_ranges[i] = 2 is max reading (margin equal to 0.3 area size)
        int margin = 0.5*h - 0.5*h*0.5*sub_->casu_data["casu-002"].ir_ranges[i]*0.7;
        reading_area = reading_area.marginsRemoved(QMargins(margin, margin, margin, margin));
        painter.drawPie(reading_area,(60*i-fov/2)*16, fov*16);
        if (sub_->casu_data["casu-002"].ir_ranges[i] > 0.0)
        {
            // A bee has been detected, render it
            svg_->load(QString("://artwork/bee.svg"));
            double dx = reading_area.width()/2*cos(60*i*deg_to_rad);
            double dy = -reading_area.width()/2*sin(60*i*deg_to_rad);
            svg_->render(&painter,QRectF(casu_bottom_.topLeft(),QSizeF(93.0,65.0)).adjusted(dx,dy,dx,dy));
        }
    }

    QConicalGradient grad_tref_top(casu_top_.center(),270);
    grad_tref_top.setColorAt(1,tempToColor(24));
    grad_tref_top.setColorAt(0,tempToColor(40));
    painter.setBrush(grad_tref_top);
    painter.drawPie(casu_top_,-45*16,270*16);
    //painter.save();
    svg_->load(QString("://artwork/button.svg"));
    //painter.rotate(temp_ref);
    svg_->render(&painter,casu_top_);
    //painter.restore();

    QConicalGradient grad_tref_bottom(casu_bottom_.center(),270);
    grad_tref_bottom.setColorAt(1,tempToColor(24));
    grad_tref_bottom.setColorAt(0,tempToColor(40));
    painter.setBrush(grad_tref_bottom);
    painter.drawPie(casu_bottom_,-45*16,270*16);
    //painter.rotate(temp_ref);
    svg_->render(&painter,casu_bottom_);

    // Draw comms
    svg_->load(QString("://artwork/doublearrow.svg"));
    svg_->render(&painter, double_arrow_);
    svg_->load(QString("://artwork/arrow.svg"));
    svg_->render(&painter, top_arrow_);
    svg_->render(&painter, bottom_arrow_);

    // Casu to cats
    svg_->load(QString("://artwork/msgcontainer.svg"));
    sub_->msg_top.update();
    QFont font;
    font.setPointSize(24);
    painter.setFont(font);
    if (sub_->msg_top.active)
    {
        svg_->render(&painter, sub_->msg_top.pose);
        painter.setPen(tempToColor(sub_->casu_data["casu-001"].temp));
        painter.drawText(sub_->msg_top.pose,Qt::AlignCenter, QString::number(sub_->msg_top.count));
    }
    sub_->msg_bottom.update();
    if (sub_->msg_bottom.active)
    {
        svg_->render(&painter, sub_->msg_bottom.pose);
        painter.setPen(tempToColor(sub_->casu_data["casu-002"].temp));
        painter.drawText(sub_->msg_bottom.pose,Qt::AlignCenter, QString::number(sub_->msg_bottom.count));
    }

    sub_->msg_cats.update();
    sub_->msg_cats.active = true;
    if (sub_->msg_cats.active)
    {
        svg_->load(QString("://artwork/msgcontainer2.svg"));
        painter.setPen(Qt::NoPen);
        svg_->render(&painter, sub_->msg_cats.pose_top);
        svg_->render(&painter, sub_->msg_cats.pose_bot);
        if (sub_->msg_cats.ribot_direction > 0)
        {
            svg_->load(QString("://artwork/msg-ribot-ccw.svg"));
        }
        else
        {
            svg_->load(QString("://artwork/msg-ribot-cw.svg"));
        }

        painter.save();
        QTransform T;
        T.scale(scaling_x,scaling_y);
        T.translate(sub_->msg_cats.ribot_dir_top.center().x(),
                    sub_->msg_cats.ribot_dir_top.center().y());
        painter.setWorldTransform(T);
        painter.rotate(sub_->msg_cats.rot_ribot);
        QRectF temp_rect = sub_->msg_cats.ribot_dir_top;
        temp_rect.moveCenter(QPoint(0,0));
        svg_->render(&painter, temp_rect);
        painter.restore();


        svg_->render(&painter, sub_->msg_cats.ribot_dir_bot);

        if (sub_->msg_cats.fish_direction > 0)
        {
            svg_->load(QString("://artwork/msg-fish-ccw.svg"));
        }
        else
        {
            svg_->load(QString("://artwork/msg-fish-cw.svg"));
        }
        painter.save();
        //painter.rotate(sub_->msg_cats.rot_fish);
        svg_->render(&painter, sub_->msg_cats.fish_dir_top);
        svg_->render(&painter, sub_->msg_cats.fish_dir_bot);
        painter.restore();
    }
}

QColor Visualizer::tempToColor(double temp)
{
    double temp_min = 24.0;
    double temp_max = 40.0;
    temp = clip(temp, temp_min, temp_max);

    double hue_min = 240;
    double hue_max = 380;
    double k = (hue_max - hue_min) / (temp_max - temp_min);
    int hue = k*(temp-temp_min) + hue_min;

    QColor color;
    color.setHsv(hue, 255, 255);

    return color;
}
