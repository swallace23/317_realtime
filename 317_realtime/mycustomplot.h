#include "qcustomplot.h"
#include <QObject>
#ifndef MYCUSTOMPLOT_H
#define MYCUSTOMPLOT_H

class MyCustomPlot : public QCustomPlot {
    Q_OBJECT
public:
    MyCustomPlot(QWidget *parent = nullptr) : QCustomPlot(parent) {
        setInteractions(QCP::iRangeZoom | QCP::iRangeDrag);
    }
protected:
    void wheelEvent(QWheelEvent *event) override {
        static QElapsedTimer timer;
        static constexpr int throttle_ms = 30;  // minimum interval between zooms

        if (!timer.isValid() || timer.elapsed() > throttle_ms) {
            QCustomPlot::wheelEvent(event);  // call base zoom handling
            timer.restart();
        } else {
            event->ignore();  // throttle excessive wheel events
        }
    }
};

#endif // MYCUSTOMPLOT_H
