#include "plotwindow.h"

PlotWindow::PlotWindow(QWidget *parent, MyCustomPlot *plot, QMainWindow *w, bool is_sweep) : QWidget(parent), old_plot(plot), main(w) {
    QPalette pal = this->palette();
    pal.setColor(QPalette::Window, Qt::white);
    this->setAutoFillBackground(true);
    this->setPalette(pal);


    QVBoxLayout *layout = new QVBoxLayout(this);
    QHBoxLayout *header = new QHBoxLayout;
    QHBoxLayout *lock_box = new QHBoxLayout;
    QPushButton *reset = new QPushButton();
    reset->setIcon(QIcon(":/circle_arrow.png"));
    reset->setIconSize(QSize(24,24));
    reset->setFixedSize(QSize(30,30));
    if(is_sweep){
        new_plot = clone_sweep();
    } else{
        new_plot = clone_imu();
    }
    connect(reset, &QPushButton::clicked,this,[=](){
            new_plot->rescaleAxes();
            new_plot->replot();
    });
    QPushButton *zoom = new QPushButton();
    zoom->setCheckable(true);
    // QCheckBox *zoom = new QCheckBox();
    zoom->setIcon(QIcon(":/dashed_box.png"));
    zoom->setIconSize(QSize(24,24));
    zoom->setFixedSize(QSize(30,30));
    zoom->setStyleSheet(R"(
        QPushButton{background-color:white;border:1px solid black;}
        QPushButton::checked{background-color: #d0d0d0;border:1px inset black;}
    )");
    connect(zoom, &QPushButton::clicked, this, [=](){
        toggle_zoom(zoom);
    });
    // QLabel *zoom_label = new QLabel("Select Zoom?");
    // zoom_label->setStyleSheet("QLabel{color:black;}");
    QPushButton *back_button = new QPushButton();
    back_button->setIcon(QIcon(":/back.png"));
    back_button->setIconSize(QSize(24,24));
    back_button->setFixedSize(QSize(30,30));
    connect(back_button, &QPushButton::clicked, this, [=](){
        new_plot->rescaleAxes();
        new_plot->replot();
        main->show();
        this->hide();
        this->deleteLater();
    });
    header->addWidget(back_button);
    lock_box->addWidget(reset);
    // lock_box->addWidget(zoom_label);
    lock_box->addWidget(zoom);
    // header->addStretch();
    // header->addStretch();
    header->addLayout(lock_box);
    header->setAlignment(Qt::AlignLeft );
    layout->addLayout(header);
    layout->addWidget(new_plot);

}

PlotWindow::~PlotWindow(){}

void PlotWindow::toggle_zoom(QAbstractButton *check){
    bool checked = check->isChecked();
    if(checked){
            new_plot->setSelectionRectMode(QCP::srmZoom);
    }else{
            new_plot->setSelectionRectMode(QCP::srmNone);

    }
}

MyCustomPlot* PlotWindow::clone_sweep(){
    MyCustomPlot *new_plot = new MyCustomPlot();
    new_plot->addGraph();
    new_plot->graph(0)->data()->set(*old_plot->graph(0)->data());
    QString ylabel = old_plot->yAxis->label();
    new_plot->xAxis->setLabel("Time (s)");
    new_plot->yAxis->setLabel(ylabel);
    new_plot->yAxis->setRange(old_plot->yAxis->range());
    new_plot->xAxis->setRange(old_plot->xAxis->range());
    new_plot->setInteraction(QCP::iRangeDrag, true);
    new_plot->setInteraction(QCP::iRangeZoom, true);
    new_plot->axisRect()->setRangeDrag(Qt::Horizontal);
    new_plot->axisRect()->setRangeZoom(Qt::Horizontal);
    new_plot->replot();


    return new_plot;
}

MyCustomPlot* PlotWindow::clone_imu(){
    MyCustomPlot *new_plot = new MyCustomPlot();
    new_plot->addGraph();
    new_plot->graph(0)->data()->set(*old_plot->graph(0)->data());
    new_plot->graph(0)->setName("x");
    new_plot->graph(0)->setPen(QPen(Qt::red));
    new_plot->addGraph();
    new_plot->graph(1)->data()->set(*old_plot->graph(1)->data());
    new_plot->graph(1)->setName("y");
    new_plot->graph(1)->setPen(QPen(Qt::blue));

    new_plot->addGraph();
    new_plot->graph(2)->data()->set(*old_plot->graph(2)->data());
    new_plot->graph(2)->setName("z");
    new_plot->graph(2)->setPen(QPen(Qt::green));

    new_plot->yAxis->setRange(old_plot->yAxis->range());
    new_plot->xAxis->setRange(old_plot->xAxis->range());
    QString ylabel = old_plot->yAxis->label();
    QString xlabel = old_plot->xAxis->label();
    new_plot->xAxis->setLabel("Time (s)");
    new_plot->yAxis->setLabel(ylabel);
    new_plot->setInteraction(QCP::iRangeDrag, true);
    new_plot->setInteraction(QCP::iRangeZoom, true);
    new_plot->axisRect()->setRangeDrag(Qt::Horizontal);
    new_plot->axisRect()->setRangeZoom(Qt::Horizontal);

    new_plot->legend->setVisible(true);

    new_plot->replot();


    return new_plot;

}

