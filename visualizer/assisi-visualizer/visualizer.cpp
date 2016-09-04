#include "visualizer.h"
#include "ui_vassisi.h"
#include "subscriber.h"

#include <QPainter>

Visualizer::Visualizer(const QString &config_path, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VAssisi)
{
    //TODO: Implement config file reading
    sub_ = new Subscriber("tcp://127.0.0.1:5555","ping",this);
    qDebug() << "Created subscriber!";
    ui->setupUi(this);
}

Visualizer::~Visualizer()
{
    delete sub_;
    delete ui;
}

void Visualizer::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    QPixmap ribot("://artwork/ribot.jpg");
    QRectF target(10.0, 20.0, 80.0, 60.0);
    QRectF source(0.0, 0.0, 267.0, 179.0);
    painter.drawPixmap(target, ribot, source);

    /*
    painter.setPen(Qt::blue);
    painter.setFont(QFont("Arial", 30));
    painter.drawText(rect(), Qt::AlignCenter, "Qt");
    */

}
