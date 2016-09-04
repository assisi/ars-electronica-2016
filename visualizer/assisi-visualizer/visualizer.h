#ifndef VISUALIZER_H
#define VISUALIZER_H

#include <QWidget>

namespace Ui {
class VAssisi;
}

class Subscriber;

class Visualizer : public QWidget
{
    Q_OBJECT

public:
    explicit Visualizer(const QString& config_path, QWidget *parent = 0);
    ~Visualizer();

protected:
    virtual void paintEvent(QPaintEvent *event);

private:
    Ui::VAssisi *ui;

    Subscriber* sub_;
};

#endif // VISUALIZER_H
