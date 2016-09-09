#ifndef VISUALIZER_H
#define VISUALIZER_H

#include <QWidget>

namespace Ui {
class VAssisi;
}

class Subscriber;
class QSvgRenderer;

class Visualizer : public QWidget
{
    Q_OBJECT

public:
    explicit Visualizer(const QString& config_path, QWidget *parent = 0);
    ~Visualizer();
    QColor tempToColor(double temp);
    double tempToAngle(double temp);

protected:
    virtual void paintEvent(QPaintEvent *event);
    void drawRotatedSvg(QPainter& painter,
                        QRectF area,
                        double angle,
                        const QString& resource_name);

private:
    Ui::VAssisi *ui;

    Subscriber* sub_;

    QSvgRenderer* svg_;

    // Fish tank dimensions
    QRect fish_tank_outer_;
    QRect fish_tank_inner_;

    // Bee arena dimensions
    QRect bee_arena_;
    QRect casu_top_;
    QRect casu_bottom_;
    QRect heating_area_top_;
    QRect heating_area_bottom_;

    // Communication arrow dimensions
    QRect double_arrow_;
    QRect top_arrow_;
    QRect bottom_arrow_;

    // Scene dimensions
    qreal default_scene_width_;
    qreal default_scene_height_;

    // Sample time for scene refreshing
    double td_;

};

template <typename T>
T clip(T x, T lower, T upper)
{
    return std::max(lower, std::min(x, upper));
}

#endif // VISUALIZER_H
