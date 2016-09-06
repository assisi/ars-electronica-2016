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
    default_scene_height_(1000)

{
    //TODO: Implement config file reading

    sub_ = new Subscriber("tcp://localhost:5555","casu-001",this);

    fish_tank_outer_.setRect(80, 50, 480, 900);
    fish_tank_inner_.setRect(200, 170, 240, 660);

    bee_arena_.setRect(1040, 50, 480, 900);
    casu_top_.setRect(1220, 225, 100, 100);
    casu_bottom_.setRect(1220, 675, 100, 100);
    heating_area_top_ = casu_top_.marginsAdded(QMargins(100,100,100,100));
    heating_area_bottom_ = casu_bottom_.marginsAdded(QMargins(100,100,100,100));
    ui->setupUi(this);

    svg_ = new QSvgRenderer(this);

    QTimer* timer = new QTimer(this);
    // Qt5 style connect does not work with overloaded functions
    //connect(timer, &QTimer::timeout, this, &QWidget::update);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(34); // 30 FPS
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
    painter.scale(this->geometry().width()/default_scene_width_,
                  this->geometry().height()/default_scene_height_);

    // Draw fish tank
    painter.drawRect(fish_tank_outer_);
    painter.drawRect(fish_tank_inner_);

    // Draw fish
    //painter.draw...

    // Draw bee arena
    painter.drawRect(bee_arena_);

    /* Draw CASU signals and bees */

    //painter.save();

    // Draw top casu heating area
    double r = heating_area_top_.height()/2.0;
    QRadialGradient grad_top(heating_area_top_.center(),r);
    QColor color = tempToColor(sub_->casu_data["casu-001"].temp);
    grad_top.setColorAt(0.0,color);
    grad_top.setColorAt(0.75,color);
    grad_top.setColorAt(1,QColor(255,255,255));
    painter.setBrush(grad_top);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(heating_area_top_);

    // Draw bottom casu heating area
    QRadialGradient grad_bottom(heating_area_bottom_.center(),r);
    color = tempToColor(sub_->casu_data["casu-002"].temp);
    grad_bottom.setColorAt(0.0,color);
    grad_bottom.setColorAt(0.75,color);
    grad_bottom.setColorAt(1,QColor(255,255,255));
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

    //painter.restore();
    svg_->load(QString("://artwork/button.svg"));
    svg_->render(&painter,casu_top_);
    svg_->render(&painter,casu_bottom_);

    // Draw comms

    /*
    QPainter painter(this);
    QPixmap ribot("://artwork/ribot.jpg");
    QRectF target(10.0, 20.0, 80.0, 60.0);
    QRectF source(0.0, 0.0, 267.0, 179.0);
    painter.drawPixmap(target, ribot, source);
    */

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
