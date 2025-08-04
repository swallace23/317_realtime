#ifndef PLOTWINDOW_H
#define PLOTWINDOW_H
#include <QObject>
#include <QWidget>
#include "mycustomplot.h"

class PlotWindow : public QWidget
{
    Q_OBJECT
public:
    PlotWindow(QWidget *parent = nullptr, MyCustomPlot *plot = nullptr, QMainWindow *w = nullptr, bool is_sweep = true);
    ~PlotWindow();
private:
    MyCustomPlot *old_plot;
    MyCustomPlot *new_plot;
    QMainWindow *main;
    void toggle_zoom(QAbstractButton *check);
    MyCustomPlot* clone_sweep();
    MyCustomPlot* clone_imu();
};

#endif // PLOTWINDOW_H


