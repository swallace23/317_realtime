#include "mainwindow.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <QPalette>
#include <QPushButton>
#include <QDebug>
#include <QStateMachine>
#include <QFile>
#include <QDataStream>
#include <QByteArray>
#include "qcustomplot.h"
#include "settingswindow.h"

#define ACC_ID 1
#define MAG_ID 2
#define GYR_ID 3

#define SATURATION 5

/*
TODO:
- fix flight mode not showing entire pip data
-  reset button broken
- button to toggle to line plot of time vs voltage for single sweep step
- show filename next to shield number
- place to put note for zoom that you like for when you screenshot.
- or, better to do save figure like matlab
- option to choose which IMU components are plotted while already in chamber plot
- helpful IMU data - gyro shows the table moving. accelerometer shows changing vertical-ness. mag less helpful
    - have color plots on the left and 5 IV curves, acc, gyro
    -
- at this stage just do it in terms of volts.
- add buffered data plots
    - static done, do realtime
- make it so you can drag both buffered and non-buffered
- 2D color plot/chamber parsing configuration
- fix time syncing for realtime
*/
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    start_welcome();

}

void MainWindow::start_welcome(){
    //set up welcome screen
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    //set background color
    QPalette pal = central->palette();
    pal.setColor(QPalette::Window, Qt::blue);
    central->setAutoFillBackground(true);
    central->setPalette(pal);
    QVBoxLayout *layout = new QVBoxLayout(central);
    central->setLayout(layout);
    //title
    QLabel *title = new QLabel("317 Lab Data Parser", central);
    title->setAlignment(Qt::AlignCenter|Qt::AlignTop);

    layout->addWidget(title);
    //start button
    QButtonGroup *select_group = new QButtonGroup();
    QCheckBox *static_plot = new QCheckBox("Static", central);
    QCheckBox *dynamic_plot = new QCheckBox("Dynamic", central);
    static_plot->setCheckable(true);
    dynamic_plot->setCheckable(true);
    select_group->addButton(static_plot,0);
    select_group->addButton(dynamic_plot,1);
    select_group->setExclusive(true);


    QPushButton *file_select = new QPushButton("Browse");
    QHBoxLayout *port_row = new QHBoxLayout;
    QLabel *port_label = new QLabel("Port:");
    // QLineEdit *port_entry;
    // if (settings.contains("port")){
    //     port_entry = new QLineEdit(settings.value("port").toString());
    // } else{
    //     port_entry = new QLineEdit();
    // }
    QComboBox *port_entry = new QComboBox;
    QSerialPortInfo port_info;
    QList<QSerialPortInfo> ports = port_info.availablePorts();
    QList<QString> port_names;
    for(int i=0;i<ports.size();i++){
        QString name = ports[i].portName();

// filter for serial ports
#ifdef Q_OS_LINUX
        if(name.startsWith("ttyUSB") || name.startsWith("ttyACM")){
            port_names.append(name);
        }
#elif defined(Q_OS_WIN)
        if(name.contains("COM")){
            port_names.append(name);
        }
#elif defined(Q_OS_MAC)
        if(name.contains("usbmodem")||name.contains("usbserial")){
            port_names.append(name);
        }
#endif
    }

    port_entry->insertItems(0,port_names);

    port_row->addWidget(port_label);
    port_row->addWidget(port_entry);

    QVBoxLayout *static_box = new QVBoxLayout;
    QHBoxLayout *static_row = new QHBoxLayout;
    QLineEdit *file_entry;
    //check for default file path
    if(settings.contains("file")){
        file_entry = new QLineEdit(settings.value("file").toString());
    } else{
        file_entry = new QLineEdit();
    }
    QCheckBox *flight_button = new QCheckBox("Flight");
    QCheckBox *chamber_button = new QCheckBox("Chamber");
    QHBoxLayout *plot_choice_box = new QHBoxLayout;
    plot_choice_box->addWidget(flight_button);
    plot_choice_box->addWidget(chamber_button);
    if(settings.contains("plot_choice")){

        if(settings.value("plot_choice").toInt()==0){
            flight_button->click();
        } else{
            chamber_button->click();
        }
    } else{
        flight_button->click();
    }
    QHBoxLayout *p0_box = new QHBoxLayout;
    QVariant p0_min_init, p0_max_init, p1_min_init, p1_max_init;
    settings.contains("p0_min") ? p0_min_init = settings.value("p0_min") : p0_min_init = 0;
    settings.contains("p1_min") ? p1_min_init = settings.value("p1_min") : p1_min_init = 0;
    settings.contains("p0_max") ? p0_max_init = settings.value("p0_max") : p0_max_init = 5;
    settings.contains("p1_max") ? p1_max_init = settings.value("p1_max") : p1_max_init = 5;
    QVariant p0_t_min_init, p0_t_max_init, p1_t_min_init, p1_t_max_init;
    settings.contains("p0_t_min") ? p0_t_min_init = settings.value("p0_t_min") : p0_t_min_init = 0;
    settings.contains("p1_t_min") ? p1_t_min_init = settings.value("p1_t_min") : p1_t_min_init = 0;
    settings.contains("p0_t_max") ? p0_t_max_init = settings.value("p0_t_max") : p0_t_max_init = 100000;
    settings.contains("p1_t_max") ? p1_t_max_init = settings.value("p1_t_max") : p1_t_max_init = 100000;


    QLineEdit *p0_min = new QLineEdit(p0_min_init.toString());
    QLineEdit *p0_max = new QLineEdit(p0_max_init.toString());
    QLabel *p0_min_label = new QLabel("PIP0 Min (V)");
    QLabel *p0_max_label = new QLabel("PIP0 Max (V)");
    p0_box->addWidget(p0_min_label);
    p0_box->addWidget(p0_min);
    p0_box->addWidget(p0_max_label);
    p0_box->addWidget(p0_max);
    QHBoxLayout *p1_box = new QHBoxLayout;
    QLineEdit *p1_min = new QLineEdit(p1_min_init.toString());
    QLineEdit *p1_max = new QLineEdit(p1_max_init.toString());
    QLabel *p1_min_label = new QLabel("PIP1 Min (V)");
    QLabel *p1_max_label = new QLabel("PIP1 Max (V)");
    p1_box->addWidget(p1_min_label);
    p1_box->addWidget(p1_min);
    p1_box->addWidget(p1_max_label);
    p1_box->addWidget(p1_max);

    QHBoxLayout *p0_t_box = new QHBoxLayout;
    QHBoxLayout *p1_t_box = new QHBoxLayout;
    QLabel *p0_t_min_label = new QLabel("IV 0 Start: ");
    QLineEdit *p0_t_min = new QLineEdit(p0_t_min_init.toString());
    QLabel *p0_t_max_label = new QLabel("IV 0 End: ");
    QLineEdit *p0_t_max = new QLineEdit(p0_t_max_init.toString());
    QLabel *p1_t_min_label = new QLabel("IV 1 Start: ");
    QLineEdit *p1_t_min = new QLineEdit(p1_t_min_init.toString());
    QLabel *p1_t_max_label = new QLabel("IV 1 End: ");
    QLineEdit *p1_t_max = new QLineEdit(p1_t_max_init.toString());
    p0_t_box->addWidget(p0_t_min_label);
    p0_t_box->addWidget(p0_t_min);
    p0_t_box->addWidget(p0_t_max_label);
    p0_t_box->addWidget(p0_t_max);
    p1_t_box->addWidget(p1_t_min_label);
    p1_t_box->addWidget(p1_t_min);
    p1_t_box->addWidget(p1_t_max_label);
    p1_t_box->addWidget(p1_t_max);

    //disable user range choices for now
    p0_min->setVisible(false);
    p1_min->setVisible(false);
    p0_max->setVisible(false);
    p1_max->setVisible(false);
    p0_t_min->setVisible(false);
    p1_t_min->setVisible(false);
    p0_t_max->setVisible(false);

    p1_t_max->setVisible(false);
    p0_min_label->setVisible(false);
    p0_max_label->setVisible(false);
    p1_min_label->setVisible(false);
    p1_max_label->setVisible(false);
    p0_t_min_label->setVisible(false);
    p0_t_max_label->setVisible(false);
    p1_t_min_label->setVisible(false);
    p1_t_max_label->setVisible(false);




    static_box->addWidget(static_plot);
    static_row->addWidget(file_entry);
    static_row->addWidget(file_select);
    static_box->addLayout(static_row);
    static_box->addLayout(plot_choice_box);
    static_box->addLayout(p0_box);
    static_box->addLayout(p1_box);
    static_box->addLayout(p0_t_box);
    static_box->addLayout(p1_t_box);
    layout->addLayout(static_box);
    layout->addStretch();

    connect(file_select,&QPushButton::clicked,this,[this,file_entry](){
        params.filename = QFileDialog::getOpenFileName(nullptr, tr("Open Binary File"), "/home", tr("All Files (*)"));
        file_entry->setText(params.filename);
    });

    QButtonGroup *plot_choice_group = new QButtonGroup;
    plot_choice_group->setExclusive(true);
    plot_choice_group->addButton(flight_button);
    plot_choice_group->addButton(chamber_button);

    QVBoxLayout *dynamic_box = new QVBoxLayout;
    QHBoxLayout *dynamic_plot_choice_box = new QHBoxLayout;
    QCheckBox *dynamic_flight_button = new QCheckBox("Flight");
    QCheckBox *dynamic_chamber_button = new QCheckBox("Chamber");
    dynamic_plot_choice_box->addWidget(dynamic_flight_button);
    dynamic_plot_choice_box->addWidget(dynamic_chamber_button);
    if(settings.contains("plot_choice")){

        if(settings.value("plot_choice").toInt()==0){
            dynamic_flight_button->click();
        } else{
            dynamic_chamber_button->click();
        }
    } else{
        dynamic_flight_button->click();
    }
    QButtonGroup *plot_choice_group_dynamic = new QButtonGroup;
    plot_choice_group_dynamic->setExclusive(true);
    plot_choice_group_dynamic->addButton(dynamic_flight_button);
    plot_choice_group_dynamic->addButton(dynamic_chamber_button);

    dynamic_box->addWidget(dynamic_plot);
    dynamic_box->addLayout(dynamic_plot_choice_box);

    dynamic_box->addLayout(port_row);
    QHBoxLayout *baud_box = new QHBoxLayout;
    QLabel *baud_label = new QLabel("Baud: ");

    QLineEdit *baud_entry;
    if (settings.contains("baud")){
        baud_entry = new QLineEdit(settings.value("baud").toString());
    } else{
        baud_entry = new QLineEdit();
    }
    WelcomeEntries entries(file_entry, port_entry, baud_entry);
    // WelcomeEntries entries(file_entry, port_entry, nullptr);
    connect(select_group, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),this,[=]{onSelect(select_group,file_select,entries);});

    // dynamic_box->addWidget(baud_entry);
    baud_box->addWidget(baud_label);
    baud_box->addWidget(baud_entry);
    dynamic_box->addLayout(baud_box);
    layout->addLayout(dynamic_box);


    layout->addStretch();

    QHBoxLayout *start_box = new QHBoxLayout;
    QPushButton *start_button = new QPushButton("Start", central);
    start_box->addWidget(start_button);
    QLabel *save_label = new QLabel("Save configuration?");
    start_box->addWidget(save_label);
    QCheckBox *save_choices = new QCheckBox();
    start_box->addWidget(save_choices);

    connect(start_button, &QPushButton::clicked,this,[=](){
        params.filename=file_entry->text();
        params.baud=baud_entry->text().toInt();
        params.port=port_entry->currentText();
        params.save=save_choices->isChecked();
        int checked = select_group->checkedId();
        params.p0_min = static_cast<float>(p0_min->text().toFloat());
        params.p1_min = static_cast<float>(p1_min->text().toFloat());
        params.p0_max = static_cast<float>(p0_max->text().toFloat());
        params.p1_max = static_cast<float>(p1_max->text().toFloat());
        params.p0_t_min = static_cast<float>(p0_t_min->text().toFloat());
        params.p1_t_min = static_cast<float>(p1_t_min->text().toFloat());
        params.p0_t_max = static_cast<float>(p0_t_max->text().toFloat());
        params.p1_t_max = static_cast<float>(p1_t_max->text().toFloat());


        if(checked==0){
            params.is_static=true;
        } else if (checked==1){
            params.is_static=false;
        }
        if(static_plot->isChecked()){
            if(flight_button->isChecked()){
                params.plot_choice=0;
            } else{
                params.plot_choice=1;
            }
        } else{
            if(dynamic_flight_button->isChecked()){
                params.plot_choice=0;
            } else{
                params.plot_choice=1;
            }

        }
        // if(flight_button->isChecked() || dynamic_flight_button->isChecked()){
        //     params.plot_choice=0;
        // } else{
        //     params.plot_choice=1;
        // }
        if(params.save){
            save_settings();
        }
        startPlots(central, layout);
    });

    QPushButton *settings_button = new QPushButton("Settings");
    connect(settings_button, &QPushButton::clicked,this, [=](){
        SettingsWindow *settings_window = new SettingsWindow(nullptr, &settings);
        settings_window->setAttribute(Qt::WA_DeleteOnClose);
        settings_window->show();
    });
    start_box->addWidget(settings_button);

    layout->addLayout(start_box);


    if (settings.contains("static")){
        if(settings.value("static").toBool()){
            static_plot->click();
        }
        else{
            dynamic_plot->click();
        }
    } else{
        static_plot->click();
    }

}
MainWindow::~MainWindow() {}

void MainWindow::onSelect(QButtonGroup *group, QPushButton *file_select, WelcomeEntries entries){
    int checked = group->checkedId();
    if(checked==0){
        file_select->setEnabled(true);
        entries.file->setEnabled(true);
        entries.port->setEnabled(false);
        entries.baud->setEnabled(false);
    }
    else if(checked==1){
        file_select->setEnabled(false);
        entries.file->setEnabled(false);
        entries.port->setEnabled(true);
        entries.baud->setEnabled(true);
    }
}



//returns null array if failure
QByteArray MainWindow::read_binary(QString filepath, qsizetype buffer_size){
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Failed to open file";
        return QByteArray();
    }

    // QDataStream in(&file);
    QByteArray ba = file.readAll();
    // qint64 bytes = in.readRawData(ba.data(),ba.size());
    file.close();
    return ba;

}

QVector<double> MainWindow::find_sentinel(QByteArray ba, char sentinel){
    QVector<double> results;
    char *data = ba.data();
    qsizetype len = ba.size();
    for (qsizetype i=0; i<len; ++i){
        if(data[i]=='#' && data[i+1]=='#' && data[i+2]==sentinel){
            results.append(i);
        }
    }
    return results;
}

void MainWindow::save_settings(){
    settings.setValue("file", params.filename);
    if(params.is_static){
        settings.setValue("static", true);
    } else {
        settings.setValue("static", false);
    }
    settings.setValue("port", params.port);
    settings.setValue("baud", params.baud);
    settings.setValue("plot_choice", params.plot_choice);
    settings.setValue("p0_min", params.p0_min);
    settings.setValue("p0_max",params.p0_max);
    settings.setValue("p1_max",params.p1_max);
    settings.setValue("p1_min",params.p1_min);
    settings.setValue("p0_t_min", params.p0_t_min);
    settings.setValue("p0_t_max",params.p0_t_max);
    settings.setValue("p1_t_max",params.p1_t_max);
    settings.setValue("p1_t_min",params.p1_t_min);


}
void MainWindow::returnToWelcome(QWidget *){
    this->resize(800,600);
    QWidget *oldCentral = centralWidget();
    if (oldCentral) {
        oldCentral->deleteLater(); // safely delete
    }

    // Create new central widget for plots screen
    QWidget *central = new QWidget(this);
    setCentralWidget(central);


    QVBoxLayout *layout = new QVBoxLayout(central);
    central->setLayout(layout);
    start_welcome();
}
void MainWindow::startPlots(QWidget *, QVBoxLayout *){
    this->resize(1400,1000);
    QWidget *oldCentral = centralWidget();
    if (oldCentral) {
        oldCentral->deleteLater(); // safely delete
    }

    // Create new central widget for plots screen
    QWidget *central = new QWidget(this);
    setCentralWidget(central);


    QVBoxLayout *layout = new QVBoxLayout(central);
    central->setLayout(layout);

    QPalette pal = central->palette();
    pal.setColor(QPalette::Window, Qt::white);
    central->setAutoFillBackground(true);
    central->setPalette(pal);



    if(params.is_static){
        if(params.plot_choice==0){
            plot_static(layout);
        } else{
            plot_static_chamber(layout);
        }
    } else{
        QString s_datetime = QDateTime::currentDateTime(QTimeZone::systemTimeZone()).toString("yyyy-MM-dd_hh-mm-ss");
        QString exeDir = QCoreApplication::applicationDirPath();
        QString writepath = QDir(exeDir).filePath(s_datetime + "_data.bin");
        writefile.setFileName(writepath);

        if (!writefile.open(QIODevice::WriteOnly | QIODevice::Append)) {
            qWarning() << "Failed to open data file for append:" << writefile.errorString();
            // handle error (disable saving, alert user, etc.)
        } else {
            qDebug() << "Writing serial data to" << writefile.fileName();
        }

        if(params.plot_choice==0){
            plot_dynamic(layout);
        } else{
            start_dynamic_chamber(layout);
        }
    }
}


void MainWindow::toggle_imu_graphs(int idx){
    bool is_visible = imu_graphs[idx]->visible();

    imu_graphs[idx]->setVisible(!is_visible);
    background_plot->replot();

}

void MainWindow::set_color_data(){
    double x, y, z, z1;
    int nx = s_data.sweep_ts.size();
    int ny = 28;
    cplot.color_map->data()->setSize(nx,ny);
    cplot.color_map_1->data()->setSize(nx,ny);
    quint32 t0 = s_data.sweep_ts.first()*t_scale;
    quint32 t1 = s_data.sweep_ts.last()*t_scale;
    cplot.color_map->data()->setRange(QCPRange(t0,t1),QCPRange(28,0));
    cplot.color_map_1->data()->setRange(QCPRange(t0,t1),QCPRange(28,0));

    for (int xIndex=0; xIndex<nx; ++xIndex)
    {
        for (int yIndex=0; yIndex<ny; ++yIndex)
        {
            cplot.color_map->data()->cellToCoord(xIndex, yIndex, &x, &y);
            z = s_data.sweep0[xIndex*ny + (ny-1-yIndex)]*p_scale;
            cplot.color_map->data()->setCell(xIndex, yIndex, z);
            cplot.color_map_1->data()->cellToCoord(xIndex, yIndex, &x, &y);
            z1 = s_data.sweep1[xIndex*ny+(ny-1-yIndex)]*p_scale;
            cplot.color_map_1->data()->setCell(xIndex, yIndex, z1);

        }
    }
    cplot.color_map->rescaleDataRange(true);
    cplot.color_map_1->rescaleDataRange(true);

}

void MainWindow::toggle_flux_voltage(int pip_num, QList<unsigned short>& data){
    double gain;
    pip_num==0 ? gain = p0_gain : gain = p1_gain;
    if(is_voltage){
        for(int i=0;i<data.size();i++){
            data[i] = (data[i]-v_offset)/gain;
        }
    } else{
        for(int i =0;i<data.size();i++){
            data[i] = (data[i]*gain)+v_offset;
        }
    }
}


void MainWindow::plot_static_chamber(QVBoxLayout* layout){
    // ensure central widget exists
    if (!this->centralWidget()) {
        QWidget *c = new QWidget;
        setCentralWidget(c);
    }

    // If layout isn't the central widget's layout, install it.
    // This is safe if you expect layout to be the visible layout.
    if (this->centralWidget()->layout() != layout) {
        this->centralWidget()->setLayout(layout);
        qDebug() << "Installed layout on centralWidget()";
    }

    parse(true);
    // ============================================== set up header ==============================================
    QHBoxLayout *header = new QHBoxLayout;
    QWidget *header_container = new QWidget;
    header_container->setLayout(header);
    header_container->setStyleSheet("background-color:black;");
    QString page_title = "Shield ";
    page_title.append(QString::number(s_data.shield_id));
    QLabel *title = new QLabel(page_title);
    title->setStyleSheet("QLabel{background-color:black;color:white;font:30pt;}");
    title->setAlignment(Qt::AlignCenter);
    header->addWidget(title);

    QHBoxLayout *lock_box = new QHBoxLayout;
    QPushButton *back_button = new QPushButton("Back to Menu");
    connect(back_button,&QPushButton::clicked, this,[=](){
        returnToWelcome(centralWidget());
    });
    QCheckBox *lock_axes = new QCheckBox();
    lock_axes->setStyleSheet("QCheckBox{background-color : cyan;}");
    QLabel *lock_axes_label = new QLabel("Lock axes?");

    QPushButton *reset = new QPushButton();
    connect(reset, &QPushButton::clicked, this, [=]() {
        cplot.acc_rect->axis(QCPAxis::atBottom)->setRange(cplot.acc_range);
        cplot.gyr_rect->axis(QCPAxis::atBottom)->setRange(cplot.gyr_range);
        cplot.map_0_rect->axis(QCPAxis::atBottom)->setRange(cplot.map_range);
        cplot.map_1_rect->axis(QCPAxis::atBottom)->setRange(cplot.map_1_range);
        cplot.iv_rect->axis(QCPAxis::atBottom)->setRange(cplot.iv_range);
        cplot.iv1_rect->axis(QCPAxis::atBottom)->setRange(cplot.iv1_range);
        background_plot->replot();

        // for (int i=0; i<rects.size();i++) {
        //     QCPAxis* xAxis = rects[i]->axis(QCPAxis::atBottom);
        //     if (!xAxis) continue;

        //     xAxis->rescale(true);
        //     xAxis->setRange(default_range);
        // }
        // background_plot->replot();
    });

    reset->setIcon(QIcon(":/home.png"));
    reset->setIconSize(QSize(30,30));
    reset->setFixedSize(34,34);
    QPushButton *zoom = new QPushButton();
    zoom->setCheckable(true);
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
    //declaring this here so it doesn't have to be global
    background_plot = new MyCustomPlot;
    QCPColorScale *color_scale = new QCPColorScale(background_plot);
    QCPColorScale *color_scale_1 = new QCPColorScale(background_plot);
    cplot.map_0_rect = new QCPAxisRect(background_plot);
    cplot.map_1_rect = new QCPAxisRect(background_plot);
    cplot.acc_rect = new QCPAxisRect(background_plot);
    cplot.gyr_rect = new QCPAxisRect(background_plot);
    cplot.iv_rect = new QCPAxisRect(background_plot);
    cplot.iv1_rect = new QCPAxisRect(background_plot);

    cplot.color_map = new QCPColorMap(cplot.map_0_rect->axis(QCPAxis::atBottom),cplot.map_0_rect->axis(QCPAxis::atLeft));
    cplot.color_map_1 = new QCPColorMap(cplot.map_1_rect->axis(QCPAxis::atBottom),cplot.map_1_rect->axis(QCPAxis::atLeft));


    QPushButton *toggle_data = new QPushButton("Toggle Data Type");
    connect(toggle_data, &QPushButton::clicked, this, [=](){
        cplot.color_map->setColorScale(color_scale);
        cplot.color_map_1->setColorScale(color_scale_1);
        toggle_flux_voltage(0,s_data.sweep0);
        toggle_flux_voltage(1,s_data.sweep1);
        set_color_data();
        if(is_voltage){
            color_scale->setLabel("PIP 0 Flux (nA/mV)");
            color_scale_1->setLabel("PIP 1 Flux (nA/mV)");
        } else{
            color_scale->setLabel("PIP 0 Voltage (V)");
            color_scale_1->setLabel("PIP 1 Voltage (V)");
        }
        is_voltage = !is_voltage;
        // color_scale->setType(QCPAxis::atRight);
        // color_scale->rescaleDataRange(true);
        // color_scale_1->rescaleDataRange(true);
        background_plot->replot();


    });
    header->addWidget(back_button);
    header->addWidget(reset);
    header->addWidget(zoom);
    header->addWidget(toggle_data);
    header->addStretch();
    header->addWidget(title);
    header->addStretch();
    lock_box->addWidget(lock_axes_label);
    lock_box->addWidget(lock_axes);
    header->addLayout(lock_box);
    lock_axes->click();

    connect(lock_axes, &QCheckBox::checkStateChanged, this, [=](){
        if(lock_axes->checkState()){
            sync_plots();
        }else{
            unsync_plots();
        }
    });

    // ============================================== set up plot ==============================================
    //for setting up axes - see https://www.qcustomplot.com/index.php/demos/colormapdemo
    background_plot->plotLayout()->clear();
    QCPLayoutGrid *plot_layout = background_plot->plotLayout();
    QCPLayoutGrid *left_col  = new QCPLayoutGrid;
    QCPLayoutGrid *right_col = new QCPLayoutGrid;
    QWidget *gyr_button_cont = new QWidget;
    QWidget *acc_button_cont = new QWidget;
    QVBoxLayout *gyr_button_layout = new QVBoxLayout;
    QVBoxLayout *acc_button_layout = new QVBoxLayout;
    gyr_button_layout->setContentsMargins(0,0,0,0);
    acc_button_layout->setContentsMargins(0,0,0,0);
    gyr_button_layout->setSpacing(2);
    acc_button_layout->setSpacing(2);

    QPushButton *gyr_x = new QPushButton("x");
    QPushButton *gyr_y = new QPushButton("y");
    QPushButton *gyr_z = new QPushButton("z");
    QPushButton *acc_x = new QPushButton("x");
    QPushButton *acc_y = new QPushButton("y");
    QPushButton *acc_z = new QPushButton("z");
    gyr_button_layout->addWidget(gyr_x);
    gyr_button_layout->addWidget(gyr_y);
    gyr_button_layout->addWidget(gyr_z);
    gyr_button_layout->addStretch();
    acc_button_layout->addWidget(acc_x);
    acc_button_layout->addWidget(acc_y);
    acc_button_layout->addWidget(acc_z);
    gyr_button_layout->addStretch();
    gyr_button_cont->setLayout(gyr_button_layout);
    acc_button_cont->setLayout(acc_button_layout);
    QVBoxLayout *button_box = new QVBoxLayout;
    button_box->addWidget(acc_button_cont);
    button_box->addWidget(gyr_button_cont);
    button_box->setSpacing(20);
    // QCPLayoutElement *gyr_element = QWidget::createWindowContainer(gyr_button_cont);
    // QCPLayoutElement *acc_element = QWidget::createWindowContainer(gyr_button_cont);


    plot_layout->addElement(0, 0, left_col);
    plot_layout->addElement(0, 1, right_col);

    background_plot->setInteractions(QCP::iRangeDrag|QCP::iRangeZoom);
    // background_plot->axisRect()->setupFullAxesBox(true);


    left_col->addElement(0, 0, cplot.map_0_rect);
    left_col->addElement(1, 0, cplot.map_1_rect);

    // populate right column (also single-column grid -> use column 0)
    right_col->addElement(0, 0, cplot.acc_rect);
    right_col->addElement(1, 0, cplot.gyr_rect);
    right_col->addElement(2, 0, cplot.iv_rect);
    right_col->addElement(3, 0, cplot.iv1_rect);

    // control internal row distribution (each nested grid controls its own rows)
    left_col->setRowStretchFactor(0, 1);
    left_col->setRowStretchFactor(1, 1);

    right_col->setRowStretchFactor(0, 1);
    right_col->setRowStretchFactor(1, 1);
    right_col->setRowStretchFactor(2, 1);
    right_col->setRowStretchFactor(3, 1);

    right_col->setColumnStretchFactor(0,5);
    right_col->setColumnStretchFactor(1,1);

    // control the two top-level column widths (ratio 1:1 here)
    plot_layout->setColumnStretchFactor(0, 1);
    plot_layout->setColumnStretchFactor(1, 1);

    cplot.map_1_rect->axis(QCPAxis::atBottom)->setLabel("Time (s)");
    cplot.map_0_rect->axis(QCPAxis::atLeft)->setLabel("Sweep Index");
    cplot.map_1_rect->axis(QCPAxis::atLeft)->setLabel("Sweep Index");

    rects = {cplot.map_0_rect, cplot.map_1_rect, cplot.acc_rect, cplot.gyr_rect, cplot.iv_rect};
    rects_b={};

    set_color_data();
    left_col->addElement(0,1,color_scale);
    left_col->addElement(1,1,color_scale_1);

    color_scale->setType(QCPAxis::atRight);
    cplot.color_map->setColorScale(color_scale);
    cplot.color_map_1->setColorScale(color_scale_1);
    color_scale->axis()->setLabel("PIP0 Voltage (V)");
    color_scale_1->axis()->setLabel("PIP1 Voltage (V)");
    // cplot.color_map->setDataRange(QCPRange(pip0_min,pip0_max));
    // cplot.color_map_1->setDataRange(QCPRange(pip1_min,pip1_max));
    cplot.color_map->rescaleDataRange();
    cplot.color_map_1->rescaleDataRange();

    cplot.map_1_rect->axis(QCPAxis::atBottom)->setLabel("Time (s)");
    cplot.map_0_rect->axis(QCPAxis::atLeft)->setLabel("Sweep Index (PIP0)");
    cplot.map_1_rect->axis(QCPAxis::atLeft)->setLabel("Sweep Index (PIP1)");
    background_plot->setAutoAddPlottableToLegend(false);
    plot_imu(cplot.acc_rect, ACC_ID);
    plot_imu(cplot.gyr_rect, GYR_ID);

    connect(acc_x,&QPushButton::clicked, this, [=]{
        toggle_imu_graphs(0);
    });
    connect(acc_y,&QPushButton::clicked, this, [=]{
        toggle_imu_graphs(1);
    });
    connect(acc_z,&QPushButton::clicked, this, [=]{
        toggle_imu_graphs(2);
    });
    connect(gyr_x,&QPushButton::clicked, this, [=]{
        toggle_imu_graphs(3);
    });
    connect(gyr_y,&QPushButton::clicked, this, [=]{
        toggle_imu_graphs(4);
    });
    connect(gyr_z,&QPushButton::clicked, this, [=]{
        toggle_imu_graphs(5);
    });


    plot_sweep(cplot.iv_rect,'0');
    plot_sweep(cplot.iv1_rect,'1');
    double p0_t0, p0_tf, p1_t0, p1_tf;
    params.p0_t_min > s_data.sweep_ts.first()*t_scale ? p0_t0 = params.p0_t_min : p0_t0 = s_data.sweep_ts.first()*t_scale;
    params.p0_t_max < s_data.sweep_ts.last()*t_scale ? p0_tf = params.p0_t_max : p0_tf = s_data.sweep_ts.last()*t_scale;
    params.p1_t_min > s_data.sweep_ts.first()*t_scale ? p1_t0 = params.p1_t_min : p1_t0 = s_data.sweep_ts.first()*t_scale;
    params.p1_t_max < s_data.sweep_ts.last()*t_scale ? p1_tf = params.p1_t_max : p1_tf = s_data.sweep_ts.last()*t_scale;


    // plot_imu_mag(cplot.imu_rect);

    cplot.color_map->rescaleDataRange();
    cplot.color_map_1->rescaleDataRange();


    background_plot->rescaleAxes();
    cplot.iv_rect->axis(QCPAxis::atBottom)->blockSignals(true);
    cplot.iv1_rect->axis(QCPAxis::atBottom)->blockSignals(true);

    cplot.iv_rect->axis(QCPAxis::atBottom)->setRange(QCPRange(p0_t0, p0_tf));
    cplot.iv1_rect->axis(QCPAxis::atBottom)->setRange(QCPRange(p1_t0, p1_tf));
    cplot.iv_rect->axis(QCPAxis::atLeft)->rescale();
    cplot.iv1_rect->axis(QCPAxis::atLeft)->rescale();
    cplot.iv_rect->axis(QCPAxis::atBottom)->blockSignals(false);
    cplot.iv1_rect->axis(QCPAxis::atBottom)->blockSignals(false);

    cplot.acc_range = cplot.acc_rect->axis(QCPAxis::atBottom)->range();
    cplot.gyr_range=cplot.gyr_rect->axis(QCPAxis::atBottom)->range();
    cplot.map_range=cplot.map_0_rect->axis(QCPAxis::atBottom)->range();
    cplot.map_1_range = cplot.map_1_rect->axis(QCPAxis::atBottom)->range();
    cplot.iv_range = cplot.iv_rect->axis(QCPAxis::atBottom)->range();
    cplot.iv1_range = cplot.iv1_rect->axis(QCPAxis::atBottom)->range();


    // set up imu component choices

    // ============================================== set up layout ==============================================
    layout->addWidget(header_container,1);
    QHBoxLayout *row = new QHBoxLayout;
    row->addWidget(background_plot,15);
    QWidget *button_cont = new QWidget;
    button_cont->setLayout(button_box);
    row->addWidget(button_cont,1);
    QWidget *row_cont = new QWidget;
    row_cont->setLayout(row);
    layout->addWidget(row_cont,10);
    header_container->show();
    background_plot->show();

    // give the layout a hint and force a relayout/paint
    header_container->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    background_plot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    background_plot->setMinimumHeight(600);
    background_plot->setMinimumWidth(1200);


    this->centralWidget()->update();
    this->centralWidget()->adjustSize();   // recompute sizes
    this->adjustSize();                    // main window adjust

    // connect(cplot.iv_rect->axis, &QCP)
    connect(cplot.iv_rect->axis(QCPAxis::atBottom),static_cast<void (QCPAxis::*)(const QCPRange &)>(&QCPAxis::rangeChanged),
            cplot.iv1_rect->axis(QCPAxis::atBottom), static_cast<void (QCPAxis::*)(const QCPRange &)>(&QCPAxis::setRange));
    // connect(cplot.iv_rect->axis(QCPAxis::atLeft),static_cast<void (QCPAxis::*)(const QCPRange &)>(&QCPAxis::rangeChanged),
    //         cplot.iv1_rect->axis(QCPAxis::atLeft), static_cast<void (QCPAxis::*)(const QCPRange &)>(&QCPAxis::setRange));
    connect(cplot.iv1_rect->axis(QCPAxis::atBottom),static_cast<void (QCPAxis::*)(const QCPRange &)>(&QCPAxis::rangeChanged),
            cplot.iv_rect->axis(QCPAxis::atBottom), static_cast<void (QCPAxis::*)(const QCPRange &)>(&QCPAxis::setRange));
    // connect(cplot.iv1_rect->axis(QCPAxis::atLeft),static_cast<void (QCPAxis::*)(const QCPRange &)>(&QCPAxis::rangeChanged),
    //         cplot.iv_rect->axis(QCPAxis::atLeft), static_cast<void (QCPAxis::*)(const QCPRange &)>(&QCPAxis::setRange));
    connect(cplot.map_0_rect->axis(QCPAxis::atBottom),static_cast<void (QCPAxis::*)(const QCPRange &)>(&QCPAxis::rangeChanged),
            cplot.map_1_rect->axis(QCPAxis::atBottom), static_cast<void (QCPAxis::*)(const QCPRange &)>(&QCPAxis::setRange));
    connect(cplot.map_0_rect->axis(QCPAxis::atLeft),static_cast<void (QCPAxis::*)(const QCPRange &)>(&QCPAxis::rangeChanged),
            cplot.map_1_rect->axis(QCPAxis::atLeft), static_cast<void (QCPAxis::*)(const QCPRange &)>(&QCPAxis::setRange));
    connect(cplot.map_1_rect->axis(QCPAxis::atBottom),static_cast<void (QCPAxis::*)(const QCPRange &)>(&QCPAxis::rangeChanged),
            cplot.map_0_rect->axis(QCPAxis::atBottom), static_cast<void (QCPAxis::*)(const QCPRange &)>(&QCPAxis::setRange));
    connect(cplot.map_1_rect->axis(QCPAxis::atLeft),static_cast<void (QCPAxis::*)(const QCPRange &)>(&QCPAxis::rangeChanged),
            cplot.map_0_rect->axis(QCPAxis::atLeft), static_cast<void (QCPAxis::*)(const QCPRange &)>(&QCPAxis::setRange));



    // connect(sourceAxis,
    //         static_cast<void (QCPAxis::*)(const QCPRange &)>(&QCPAxis::rangeChanged),
    //         targetAxis,
    //         static_cast<void (QCPAxis::*)(const QCPRange &)>(&QCPAxis::setRange));

    // connect(sourceAxis->parentPlot(), &QCustomPlot::afterReplot, targetAxis->parentPlot(), [=](){
    //     targetAxis->parentPlot()->replot();
    // });

    background_plot->replot();


}
void MainWindow::plot_dynamic(QVBoxLayout* layout){
    pip_plots = new MyCustomPlot();
    pip_plots->plotLayout()->clear();
    QCPTextElement *unbuf_title = new QCPTextElement(pip_plots);
    unbuf_title->setFont(QFont("sans",12,QFont::Bold));
    unbuf_title->setText("Unbuffered");
    QCPTextElement *buf_title = new QCPTextElement(pip_plots);
    buf_title->setText("Buffered");
    buf_title->setFont(QFont("sans",12,QFont::Bold));
    pip_plots->plotLayout()->insertRow(0);
    pip_plots->plotLayout()->addElement(0,0,unbuf_title);
    pip_plots->plotLayout()->addElement(0,1,buf_title);
    QCPAxisRect *s1_rect = new QCPAxisRect(pip_plots);
    QCPAxisRect *s1_rect_b = new QCPAxisRect(pip_plots);
    QCPAxisRect *s2_rect = new QCPAxisRect(pip_plots);
    QCPAxisRect *s2_rect_b = new QCPAxisRect(pip_plots);
    QCPAxisRect *acc_rect = new QCPAxisRect(pip_plots);
    QCPAxisRect *mag_rect = new QCPAxisRect(pip_plots);
    QCPAxisRect *gyr_rect = new QCPAxisRect(pip_plots);
    QCPAxisRect *acc_rect_b = new QCPAxisRect(pip_plots);
    QCPAxisRect *mag_rect_b = new QCPAxisRect(pip_plots);
    QCPAxisRect *gyr_rect_b = new QCPAxisRect(pip_plots);
    rects = {s1_rect, s2_rect, acc_rect, mag_rect, gyr_rect};
    rects_b = {s1_rect_b, s2_rect_b, acc_rect_b, mag_rect_b, gyr_rect_b};
    QCPLayoutGrid *left_layout = new QCPLayoutGrid;
    QCPLayoutGrid *right_layout = new QCPLayoutGrid;
    pip_plots->plotLayout()->addElement(1,0,left_layout);
    pip_plots->plotLayout()->addElement(1,1,right_layout);
    left_layout->addElement(0,0,s1_rect);
    left_layout->addElement(1,0,s2_rect);
    left_layout->addElement(2,0,acc_rect);
    left_layout->addElement(3,0,mag_rect);
    left_layout->addElement(4,0,gyr_rect);


    right_layout->addElement(0,0,s1_rect_b);
    right_layout->addElement(1,0,s2_rect_b);
    right_layout->addElement(2,0,acc_rect_b);
    right_layout->addElement(3,0,mag_rect_b);
    right_layout->addElement(4,0,gyr_rect_b);



    QList<QCPAxis*> allAxes;
    allAxes << s1_rect->axes() <<s1_rect_b->axes() <<s2_rect->axes() << s2_rect_b->axes() << acc_rect->axes() << mag_rect->axes() << gyr_rect->axes() <<
        acc_rect_b->axes() << mag_rect_b->axes() << gyr_rect_b->axes();

    foreach(QCPAxis *axis, allAxes){
        axis->setLayer("axes");
        axis->grid()->setLayer("grid");
        axis->axisRect()->setRangeDrag(Qt::Horizontal);
        axis->axisRect()->setRangeZoom(Qt::Horizontal);

    }
    pip_plots->setSelectionRectMode(QCP::srmNone);
    // THIS MUST BE ABOVE THE PLOT_IMU CALLS OR THE LEGENDS BREAK I DON'T KNOW WHY
    pip_plots->setAutoAddPlottableToLegend(false);

    QCPTextElement *cadence_label = new QCPTextElement(pip_plots);
    cadence_label->setText(QString("Cadence: %1").arg(s_data.cad));
    pip_plots->plotLayout()->addElement(2,0,cadence_label);


    QSerialPort *serial = new QSerialPort;
    QSerialPortInfo port_info(params.port);

    serial->setPort(port_info);
    serial->setBaudRate(params.baud);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);
    serial->setDataBits(QSerialPort::Data8);
    if (!serial->open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open port:" << serial->errorString();
    }
    QTimer *waiter = new QTimer(this);
    waiter->setSingleShot(true);
    connect(waiter,&QTimer::timeout,this,[=](){this->first_read=true; read_serial(serial);});
    waiter->start(500);

    layout->addWidget(pip_plots);


    timer = new QTimer(this);
    connect(timer,&QTimer::timeout,this,[=](){if(first_read){read_serial(serial);}});
    timer->start(222);
}

void MainWindow::start_dynamic_chamber(QVBoxLayout* layout){

    // ============================================== set up header ==============================================
    QHBoxLayout *header = new QHBoxLayout;
    QWidget *header_container = new QWidget;
    header_container->setLayout(header);
    header_container->setStyleSheet("background-color:white;");
    QString page_title = "Shield ";
    page_title.append(QString::number(s_data.shield_id));
    QLabel *title = new QLabel(page_title);
    title->setStyleSheet("QLabel{background-color:white;color:black;font:30pt;}");
    title->setAlignment(Qt::AlignCenter);
    header->addWidget(title);

    // ============================================== set up plot ==============================================
    //for setting up axes - see https://www.qcustomplot.com/index.php/demos/colormapdemo
    background_plot = new MyCustomPlot;
    background_plot->setInteractions(QCP::iRangeDrag|QCP::iRangeZoom);
    background_plot->axisRect()->setupFullAxesBox(true);
    background_plot->xAxis->setLabel("Time (s)");
    background_plot->yAxis->setLabel("Sweep Index");
    cplot.map_0_rect = new QCPAxisRect(background_plot);
    cplot.map_1_rect = new QCPAxisRect(background_plot);
    cplot.acc_rect = new QCPAxisRect(background_plot);
    background_plot->plotLayout()->clear();
    background_plot->plotLayout()->addElement(0,0,cplot.map_0_rect);
    background_plot->plotLayout()->addElement(1,0,cplot.map_1_rect);
    background_plot->plotLayout()->addElement(2,0,cplot.acc_rect);
    cplot.color_map = new QCPColorMap(cplot.map_0_rect->axis(QCPAxis::atBottom),cplot.map_0_rect->axis(QCPAxis::atLeft));
    cplot.color_map_1 = new QCPColorMap(cplot.map_1_rect->axis(QCPAxis::atBottom),cplot.map_1_rect->axis(QCPAxis::atLeft));

    QCPColorScale *color_scale = new QCPColorScale(background_plot);
    QCPColorScale *color_scale_1 = new QCPColorScale(background_plot);
    background_plot->plotLayout()->addElement(0,1,color_scale);
    background_plot->plotLayout()->addElement(1,1,color_scale_1);

    color_scale->setType(QCPAxis::atRight);
    cplot.color_map->setColorScale(color_scale);
    cplot.color_map_1->setColorScale(color_scale_1);
    color_scale->axis()->setLabel("PIP0 Flux (nA)");
    color_scale_1->axis()->setLabel("PIP1 Flux (nA)");


    // ============================================== set up layout ==============================================
    layout->addWidget(header_container,1);
    layout->addWidget(background_plot,10);
    // ============================================== set up serial ==============================================

    QSerialPort *serial = new QSerialPort;
    QSerialPortInfo port_info(params.port);

    serial->setPort(port_info);
    serial->setBaudRate(params.baud);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);
    serial->setDataBits(QSerialPort::Data8);
    if (!serial->open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open port:" << serial->errorString();
    }
    QTimer *waiter = new QTimer(this);
    waiter->setSingleShot(true);
    connect(waiter,&QTimer::timeout,this,[=](){this->first_read=true; read_serial(serial);});
    waiter->start(500);

    timer = new QTimer(this);
    connect(timer,&QTimer::timeout,this,[=](){if(first_read){read_serial(serial);}});
    timer->start(1000);


}
void MainWindow::plot_dynamic_chamber(MyCustomPlot* background_plot){
    int nx = s_data.sweep_ts.size();
    int ny = 28;
    cplot.color_map->data()->setSize(nx,ny);
    cplot.color_map_1->data()->setSize(nx,ny);
    quint32 t0 = s_data.sweep_ts.first()*t_scale;
    quint32 t1 = s_data.sweep_ts.last()*t_scale;
    cplot.color_map->data()->setRange(QCPRange(t0,t1),QCPRange(28,0));
    cplot.color_map_1->data()->setRange(QCPRange(t0,t1),QCPRange(28,0));
    double x, y, z, z1;
    double z_min=0, z_max=0, z_min_1=0, z_max_1=0;
    for (int xIndex=0; xIndex<nx; ++xIndex)
    {
        for (int yIndex=0; yIndex<ny; ++yIndex)
        {
            cplot.color_map->data()->cellToCoord(xIndex, yIndex, &x, &y);
            z = ((s_data.sweep0[xIndex*ny + (ny-1-yIndex)]*p_scale)-1)/(40*qPow(10,-3));
            // calculate min/max nA values while scaling
            if(z<z_min){z_min=z;}
            else if(z>z_max){z_max=z;}
            cplot.color_map->data()->setCell(xIndex, yIndex, z);
            cplot.color_map_1->data()->cellToCoord(xIndex, yIndex, &x, &y);
            z1 = ((s_data.sweep1[xIndex*ny + (ny-1-yIndex)]*p_scale)-1)/(320*qPow(10,-3));
            if(z1<z_min_1){z_min_1=z1;}
            else if(z1>z_max_1){z_max_1=z1;}
            cplot.color_map_1->data()->setCell(xIndex, yIndex, z1);
        }
    }

    // calculate relative position of saturation value
    double z_cutoff = static_cast<double>(SATURATION);
    double z_cutoff_na_low = (z_cutoff-1)/(40*qPow(10,-3));
    double z_cutoff_na_high = (z_cutoff-1)/(320*qPow(10,-3));
    //adjust cutoff for testing with synthetic data
    // z_cutoff_na_low-=10;
    // z_cutoff_na_high-=5;
    double p = (z_cutoff_na_low-z_min)/(z_max-z_min);
    double p1 = (z_cutoff_na_high-z_min_1)/(z_max_1-z_min_1);

    // set saturation
    QCPColorGradient grad = cplot.color_map->gradient();
    QCPColorGradient grad_1 = cplot.color_map_1->gradient();
    QColor sat_color(85,255,0);

    QColor neg_color_end(18,18,18);
    QColor neg_color_start(93,135,182);
    double p_neg = -z_min/(z_max-z_min);
    double p_neg_1 = -z_min_1/(z_max_1-z_min_1);

    QMap<double, QColor> default_positive_stops;
    default_positive_stops[1.0] = QColor(0, 0, 255);    // blue (start of positive region)
    default_positive_stops[0.0] = QColor(0, 255, 239);  // yellow (just before saturation)

    // --- For grad ---
    QMap<double, QColor> stops;
    stops[0.0]     = neg_color_end;
    stops[p_neg]   = neg_color_start;
    if(p<1.0){
        stops[p_neg+0.001]= default_positive_stops[0.0];    // zero
        stops[p]   = default_positive_stops[1.0];
        stops[p+0.001] = sat_color;
        stops[1.0]     = sat_color;        // clamp rest
    } else{
        stops[p_neg+0.001]= default_positive_stops[0.0];    // zero
        stops[1.0]   = default_positive_stops[1.0];

    }

    grad.setColorStops(stops);

    // --- For grad_1 ---
    QMap<double, QColor> stops_1;
    stops_1[0.0]     = neg_color_end;    // very negative
    stops_1[p_neg_1]   = neg_color_start;
    if(p<1.0){
        stops_1[p_neg_1+0.001]= default_positive_stops[0.0];    // zero
        stops_1[p1]   = default_positive_stops[1.0];
        stops_1[p1+0.001] = sat_color;
        stops_1[1.0]     = sat_color;        // clamp rest
    } else{
        stops_1[p_neg_1+0.001]= default_positive_stops[0.0];    // zero
        stops_1[1.0]   = default_positive_stops[1.0];
    }

    grad_1.setColorStops(stops_1);

    // Apply
    cplot.color_map->setGradient(grad);
    cplot.color_map_1->setGradient(grad_1);


    cplot.map_1_rect->axis(QCPAxis::atBottom)->setLabel("Time (s)");
    cplot.map_0_rect->axis(QCPAxis::atLeft)->setLabel("Sweep Index (PIP0)");
    cplot.map_1_rect->axis(QCPAxis::atLeft)->setLabel("Sweep Index (PIP1)");

    plot_imu_mag(cplot.acc_rect);
    cplot.color_map->rescaleDataRange();
    cplot.color_map_1->rescaleDataRange();


    background_plot->rescaleAxes();
    background_plot->replot();


}
void MainWindow::update_dynamic(bool chamber){
    // QMutexLocker locker(&mutex);
    if (!chamber){
        if(first_read){
            plot_sweep(rects[0], '0');
            plot_sweep(rects[1],'1');
            plot_sweep(rects_b[0],'0',true);
            plot_sweep(rects_b[1],'1',true);
            plot_imu(rects[2],ACC_ID);
            plot_imu(rects[3],MAG_ID);
            plot_imu(rects[4],GYR_ID);
            plot_imu(rects_b[2],ACC_ID, true);
            plot_imu(rects_b[3],GYR_ID, true);
            plot_imu(rects_b[4],MAG_ID, true);
            pip_plots->replot();
            s_data.imu.clear();
            s_data.imu_b.clear();
            s_data.sweep0.clear();
            s_data.sweep1.clear();
            s_data.sweep0_b.clear();
            s_data.sweep1_b.clear();
            s_data.imu_ts.clear();
            s_data.imu_ts_b.clear();
            s_data.sweep_ts.clear();
            s_data.sweep_ts_b.clear();
            s_data.num_imu=0;
            s_data.num_imu_b=0;
            s_data.num_sweeps=0;
            s_data.num_sweeps_b=0;

        }

    } else{
        plot_dynamic_chamber(background_plot);
        // background_plot->replot();
        s_data.imu.clear();
        s_data.imu_b.clear();
        s_data.sweep0.clear();
        s_data.sweep1.clear();
        s_data.sweep0_b.clear();
        s_data.sweep1_b.clear();
        s_data.imu_ts.clear();
        s_data.imu_ts_b.clear();
        s_data.sweep_ts.clear();
        s_data.sweep_ts_b.clear();
        s_data.num_imu=0;
        s_data.num_imu_b=0;
        s_data.num_sweeps=0;
        s_data.num_sweeps_b=0;

    }
}
void MainWindow::read_serial(QSerialPort* serial){
    //QMutexLocker locker(&mutex);
    timer->stop();
    serial_bytes->append(serial->readAll());
    parse(false);
    QMutexLocker locker(&fileMutex);
    if (writefile.isOpen()) {
        qint64 written = writefile.write(*serial_bytes);     // writes raw bytes
        if (written == -1) {
            qWarning() << "Failed to write to file:" << writefile.errorString();
        } else if (written != serial_bytes->size()) {
            qWarning() << "Partial write to file:" << written << "of" << serial_bytes->size();
        }
    }

    if(params.plot_choice==0){
        QTimer::singleShot(100, this, [=]() { update_dynamic();
        serial_bytes->clear(); timer->start(1000);});
    } else{
        QTimer::singleShot(100, this, [=]() { update_dynamic(true);
        serial_bytes->clear(); timer->start(1000);});
    }

}

void MainWindow::parse_serial(){
    filepath = params.filename.trimmed();
    // QByteArray bin = read_binary(filepath,buffer_size);
    QVector<double> sweep_starts = find_sentinel(*serial_bytes, 'S');
    s_data.num_sweeps = sweep_starts.size();

    s_data.sweep0.resize(s_data.num_sweeps * sweep_len);
    s_data.sweep1.resize(s_data.num_sweeps * sweep_len);
    s_data.imu.resize(s_data.num_sweeps * imu_len);
    s_data.imu_ts.resize(s_data.num_sweeps);
    s_data.sweep_ts.resize(s_data.num_sweeps);




    for(int i=0;i<s_data.num_sweeps;++i){
        // int sweep_data_start = static_cast<int>(sweep_starts[i]) + 8;
        // pull out full cycle
        QByteArray cycle = serial_bytes->mid(sweep_starts[i],message_size);
        // int start = sweep_starts[i];
        // if (start + message_size > serial_bytes->size())
        //     break;  // wait for more bytes to arrive

        // if (start + message_size > serial_bytes->size())
        //     continue;  // skip incomplete message
        QDataStream stream(cycle);
        stream.setByteOrder(QDataStream::LittleEndian);
        // skip sweep sentinel
        stream.skipRawData(3);
        stream >> s_data.sweep_ts[i];
        if(i==0){
            stream >> s_data.shield_id;
        } else{
            stream.skipRawData(1);
        }
        int swp_offset = i*sweep_len;
        int imu_offset = i*imu_len;
        for (int j=0;j<sweep_len;j++){
            stream >> s_data.sweep0[swp_offset+j];
        }
        for (int j=0;j<sweep_len;j++){
            stream >> s_data.sweep1[swp_offset+j];
        }
        stream.skipRawData(3);
        stream >> s_data.imu_ts[i];
        /*IMU data is 2 bytes for each coordinate: mag x,y,z acc x,y,z gyro x,y,z and two zero bytes*/
        for (int j=0;j<imu_len;j++){
            stream >> s_data.imu[imu_offset+j];
        }

    }
    int last_used = sweep_starts.last() + message_size;
    if (serial_bytes->size() >= last_used)
        serial_bytes->remove(0, last_used);


}



void MainWindow::plot_static(QVBoxLayout* layout){
    // parse sweep data
    parse(true);

    // ============================================== set up header ==============================================
    QHBoxLayout *header = new QHBoxLayout;
    QWidget *header_container = new QWidget;
    header_container->setLayout(header);
    header_container->setStyleSheet("background-color:black;");
    QPushButton *back_button = new QPushButton("Back to Menu");
    connect(back_button,&QPushButton::clicked, this,[=](){
        returnToWelcome(centralWidget());
    });
    QHBoxLayout *lock_box = new QHBoxLayout;
    QCheckBox *lock_axes = new QCheckBox();
    lock_axes->setStyleSheet("QCheckBox{background-color : cyan;}");
    QLabel *lock_axes_label = new QLabel("Lock axes?");
    QString page_title = "Shield ";
    lock_axes_label->setStyleSheet("QLabel{color:white;}");
    page_title.append(QString::number(s_data.shield_id));
    QLabel *title = new QLabel(page_title);
    title->setStyleSheet("QLabel{background-color:black;color:white;font:40pt;}");
    title->setAlignment(Qt::AlignCenter);

    QPushButton *reset = new QPushButton();
    connect(reset, &QPushButton::clicked, this, [=]() {
        for (int i=0; i<rects.size();i++) {
            QCPAxis* xAxis = rects[i]->axis(QCPAxis::atBottom);
            QCPAxis* xAxis_b = rects_b[i]->axis(QCPAxis::atBottom);
            if (!xAxis) continue;

            xAxis->rescale(true);
            xAxis_b->rescale(true);
            xAxis->setRange(default_range);
            xAxis_b->setRange(default_range_b);
        }
        pip_plots->replot();
    });

    reset->setIcon(QIcon(":/home.png"));
    reset->setIconSize(QSize(30,30));
    reset->setFixedSize(34,34);
    QPushButton *zoom = new QPushButton();
    zoom->setCheckable(true);
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
    header->addWidget(back_button);
    header->addWidget(reset);
    header->addWidget(zoom);
    header->addStretch();
    header->addWidget(title);
    header->addStretch();
    lock_box->addWidget(lock_axes_label);
    lock_box->addWidget(lock_axes);
    header->addLayout(lock_box);
    lock_axes->click();

    connect(lock_axes, &QCheckBox::checkStateChanged, this, [=](){
        if(lock_axes->checkState()){
            sync_plots();
        }else{
            unsync_plots();
        }
    });

    if(params.filename.isEmpty()){
        params.filename = QFileDialog::getOpenFileName(nullptr, tr("Open Binary File"), "/home", tr("All Files (*)"));
    }

    // ============================================== unbuffered plots ==============================================
    pip_plots = new MyCustomPlot();
    pip_plots->plotLayout()->clear();
    QCPTextElement *unbuf_title = new QCPTextElement(pip_plots);
    unbuf_title->setFont(QFont("sans",12,QFont::Bold));
    unbuf_title->setText("Unbuffered");
    QCPTextElement *buf_title = new QCPTextElement(pip_plots);
    buf_title->setText("Buffered");
    buf_title->setFont(QFont("sans",12,QFont::Bold));
    pip_plots->plotLayout()->insertRow(0);
    pip_plots->plotLayout()->addElement(0,0,unbuf_title);
    pip_plots->plotLayout()->addElement(0,1,buf_title);
    QCPAxisRect *s1_rect = new QCPAxisRect(pip_plots);
    QCPAxisRect *s1_rect_b = new QCPAxisRect(pip_plots);
    QCPAxisRect *s2_rect = new QCPAxisRect(pip_plots);
    QCPAxisRect *s2_rect_b = new QCPAxisRect(pip_plots);
    QCPAxisRect *acc_rect = new QCPAxisRect(pip_plots);
    QCPAxisRect *mag_rect = new QCPAxisRect(pip_plots);
    QCPAxisRect *gyr_rect = new QCPAxisRect(pip_plots);
    QCPAxisRect *acc_rect_b = new QCPAxisRect(pip_plots);
    QCPAxisRect *mag_rect_b = new QCPAxisRect(pip_plots);
    QCPAxisRect *gyr_rect_b = new QCPAxisRect(pip_plots);
    rects = {s1_rect, s2_rect, acc_rect, mag_rect, gyr_rect};
    rects_b = {s1_rect_b, s2_rect_b, acc_rect_b, mag_rect_b, gyr_rect_b};




    QCPLayoutGrid *left_layout = new QCPLayoutGrid;
    QCPLayoutGrid *right_layout = new QCPLayoutGrid;
    pip_plots->plotLayout()->addElement(1,0,left_layout);
    pip_plots->plotLayout()->addElement(1,1,right_layout);
    left_layout->addElement(0,0,s1_rect);
    left_layout->addElement(1,0,s2_rect);
    left_layout->addElement(2,0,acc_rect);
    left_layout->addElement(3,0,mag_rect);
    left_layout->addElement(4,0,gyr_rect);


    right_layout->addElement(0,0,s1_rect_b);
    right_layout->addElement(1,0,s2_rect_b);
    right_layout->addElement(2,0,acc_rect_b);
    right_layout->addElement(3,0,mag_rect_b);
    right_layout->addElement(4,0,gyr_rect_b);



    QList<QCPAxis*> allAxes;
    allAxes << s1_rect->axes() <<s1_rect_b->axes() <<s2_rect->axes() << s2_rect_b->axes() << acc_rect->axes() << mag_rect->axes() << gyr_rect->axes() <<
        acc_rect_b->axes() << mag_rect_b->axes() << gyr_rect_b->axes();

    foreach(QCPAxis *axis, allAxes){
        axis->setLayer("axes");
        axis->grid()->setLayer("grid");
        axis->axisRect()->setRangeDrag(Qt::Horizontal);
        axis->axisRect()->setRangeZoom(Qt::Horizontal);

    }
    pip_plots->setSelectionRectMode(QCP::srmNone);
    // THIS MUST BE ABOVE THE PLOT_IMU CALLS OR THE LEGENDS BREAK I DON'T KNOW WHY
    pip_plots->setAutoAddPlottableToLegend(false);



    plot_sweep(s1_rect, '0');
    plot_sweep(s2_rect,'1');
    plot_sweep(s1_rect_b,'0',true);
    plot_sweep(s2_rect_b,'1',true);
    plot_imu(acc_rect,ACC_ID);
    plot_imu(gyr_rect,GYR_ID);
    plot_imu(mag_rect,MAG_ID);
    plot_imu(acc_rect_b,ACC_ID, true);
    plot_imu(gyr_rect_b,GYR_ID, true);
    plot_imu(mag_rect_b,MAG_ID, true);
    layout->addWidget(pip_plots);
    pip_plots->replot();

    layout->addWidget(header_container,1);
    layout->addWidget(pip_plots,9);

    sync_plots();
    default_range = s1_rect->axis(QCPAxis::atBottom)->range();
    default_range_b = s1_rect_b->axis(QCPAxis::atBottom)->range();


}
void MainWindow::sync_plots(){
    // Get all axis rects
    for (int i = 0; i < rects.size(); i++) {
        QCPAxis* sourceAxis = rects[i]->axis(QCPAxis::atBottom);  // example: xAxis bottom
        if (!sourceAxis) continue;

        for (int j = 0; j < rects.size(); j++) {
            if (i == j) continue;

            QCPAxis* targetAxis = rects[j]->axis(QCPAxis::atBottom);
            if (!targetAxis) continue;

            connect(sourceAxis,
                    static_cast<void (QCPAxis::*)(const QCPRange &)>(&QCPAxis::rangeChanged),
                    targetAxis,
                    static_cast<void (QCPAxis::*)(const QCPRange &)>(&QCPAxis::setRange));

            connect(sourceAxis->parentPlot(), &QCustomPlot::afterReplot, targetAxis->parentPlot(), [=](){
                targetAxis->parentPlot()->replot();
            });
        }
    }
    if(rects_b.size()!=0){
        for (int i = 0; i < rects_b.size(); i++) {
            QCPAxis* sourceAxis = rects_b[i]->axis(QCPAxis::atBottom);  // example: xAxis bottom
            if (!sourceAxis) continue;

            for (int j = 0; j < rects_b.size(); j++) {
                if (i == j) continue;

                QCPAxis* targetAxis = rects_b[j]->axis(QCPAxis::atBottom);
                if (!targetAxis) continue;

                connect(sourceAxis,
                        static_cast<void (QCPAxis::*)(const QCPRange &)>(&QCPAxis::rangeChanged),
                        targetAxis,
                        static_cast<void (QCPAxis::*)(const QCPRange &)>(&QCPAxis::setRange));

                connect(sourceAxis->parentPlot(), &QCustomPlot::afterReplot, targetAxis->parentPlot(), [=](){
                    targetAxis->parentPlot()->replot();
                });
            }
        }

    }

}

void MainWindow::unsync_plots(){
    for (int i=0;i<rects.size();i++) {
        QCPAxis* axis = rects[i]->axis(QCPAxis::atBottom);
        if (axis) {
            disconnect(axis,
                       static_cast<void (QCPAxis::*)(const QCPRange &)>(&QCPAxis::rangeChanged),
                       nullptr, nullptr);
            disconnect(axis->parentPlot(), &QCustomPlot::afterReplot, nullptr, nullptr);
        }
    }
}


void MainWindow::parse(bool is_static){
    QByteArray bin;
    if(is_static){
        filepath = params.filename.trimmed();
        bin = read_binary(filepath,buffer_size);
    } else{
        bin = *serial_bytes;
    }
    //find data and buffered data start indices
    QVector<double> sweep_starts = find_sentinel(bin, 'S');
    s_data.num_sweeps = sweep_starts.size();
    QVector<double> sweep_starts_b = find_sentinel(bin,'T');
    s_data.num_sweeps_b = sweep_starts_b.size();
    QVector<double> imu_starts = find_sentinel(bin,'I');
    QVector<double> imu_starts_b = find_sentinel(bin,'J');
    s_data.num_imu = imu_starts.size();
    s_data.num_imu_b = imu_starts_b.size();
    //resize data arrays
    s_data.sweep0.resize(s_data.num_sweeps * sweep_len);
    s_data.sweep1.resize(s_data.num_sweeps * sweep_len);
    s_data.imu.resize(s_data.num_imu * imu_len);
    s_data.imu_ts.resize(s_data.num_imu);
    s_data.sweep_ts.resize(s_data.num_sweeps);
    // resize buffer data arrays
    s_data.sweep0_b.resize(s_data.num_sweeps_b * sweep_len);
    s_data.sweep1_b.resize(s_data.num_sweeps_b * sweep_len);
    s_data.imu_b.resize(s_data.num_imu_b * imu_len);
    s_data.imu_ts_b.resize(s_data.num_imu_b);
    s_data.sweep_ts_b.resize(s_data.num_sweeps_b);

    //parse unbuffered data
    int sweep_packet_size = 3+4+1+(28*2*2);
    for(int i=0;i<s_data.num_sweeps;++i){
        // int sweep_data_start = static_cast<int>(sweep_starts[i]) + 8;
        // pull out full cycle
        // if (i >= sweep_starts.size()) break;
        // if (sweep_starts[i] + sweep_packet_size > bin.size()) {
        //     qDebug() << "Partial or invalid sweep packet at index" << i;
        //     continue;  // skip incomplete packet
        // }
        QByteArray cycle = bin.mid(sweep_starts[i],sweep_packet_size);
        QDataStream stream(cycle);
        stream.setByteOrder(QDataStream::LittleEndian);
        // skip sweep sentinel
        stream.skipRawData(3);
        stream >> s_data.sweep_ts[i];
        if(i==0){
            stream >> s_data.shield_id;
        } else{
            stream.skipRawData(1);
        }
        int swp_offset = i*sweep_len;
        // int imu_offset = i*imu_len;
        for (int j=0;j<sweep_len;j++){
            stream >> s_data.sweep0[swp_offset+j];
        }
        for (int j=0;j<sweep_len;j++){
            stream >> s_data.sweep1[swp_offset+j];
        }
    }

    int imu_packet_size = 3+4+10*2;
    for(int i=0;i<s_data.num_imu;i++){
        // if (i >= imu_starts.size()) break;
        // if (imu_starts[i] + imu_packet_size > bin.size()) {
        //     qDebug() << "Partial or invalid imu packet at index" << i;
        //     continue;  // skip incomplete packet
        // }
        QByteArray cycle = bin.mid(imu_starts[i],imu_packet_size);
        QDataStream stream(cycle);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream.skipRawData(3);
        stream >> s_data.imu_ts[i];
        int imu_offset = i*imu_len;
        for (int j=0;j<imu_len;j++){
            stream >> s_data.imu[imu_offset+j];
        }

    }
    //parse buffered data
    for(int i=0;i<s_data.num_sweeps_b;++i){
        // int sweep_data_start = static_cast<int>(sweep_starts[i]) + 8;
        // pull out full cycle
        // if (i >= sweep_starts_b.size()) break;
        // if (sweep_starts_b[i] + sweep_packet_size > bin.size()) {
        //     qDebug() << "Partial or invalid sweep_b packet at index" << i;
        //     continue;  // skip incomplete packet
        // }
        QByteArray cycle = bin.mid(sweep_starts_b[i],sweep_packet_size);
        QDataStream stream(cycle);
        stream.setByteOrder(QDataStream::LittleEndian);
        // skip sweep sentinel
        stream.skipRawData(3);
        stream >> s_data.sweep_ts_b[i];
        //skip shield ID
        stream.skipRawData(1);
        int swp_offset = i*sweep_len;
        // int imu_offset = i*imu_len;
        for (int j=0;j<sweep_len;j++){
            stream >> s_data.sweep0_b[swp_offset+j];
        }
        for (int j=0;j<sweep_len;j++){
            stream >> s_data.sweep1_b[swp_offset+j];
        }
        //skip imu sentinel
        // stream.skipRawData(3);
        // stream >> s_data.imu_ts_b[i];
        // /*IMU data is 2 bytes for each coordinate: mag x,y,z acc x,y,z gyro x,y,z and two zero bytes*/
        // for (int j=0;j<imu_len;j++){
        //     stream >> s_data.imu_b[imu_offset+j];
        // }

    }
    for(int i=0;i<s_data.num_imu_b;i++){
        // if (i >= imu_starts_b.size()) break;
        // if (imu_starts_b[i] + imu_packet_size > bin.size()) {
        //     qDebug() << "Partial or invalid imu_b packet at index" << i;
        //     continue;  // skip incomplete packet
        // }
        QByteArray cycle = bin.mid(imu_starts_b[i],imu_packet_size);
        QDataStream stream(cycle);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream.skipRawData(3);
        stream >> s_data.imu_ts_b[i];
        int imu_offset = i*imu_len;
        for (int j=0;j<imu_len;j++){
            stream >> s_data.imu_b[imu_offset+j];
        }

    }

    // if(!is_static){
    //     int last_used = sweep_starts.last() + message_size;
    //     if (serial_bytes->size() >= last_used){
    //         serial_bytes->remove(0, last_used);
    //     }

    // }



}

void MainWindow::toggle_zoom(QAbstractButton *check){
    bool checked = check->isChecked();
    if(params.plot_choice==0){
        if(checked){
            pip_plots->setSelectionRectMode(QCP::srmZoom);
        }else{
            pip_plots->setSelectionRectMode(QCP::srmNone);

        }
    } else{
        if(checked){
            background_plot->setSelectionRectMode(QCP::srmZoom);
        }else{
            background_plot->setSelectionRectMode(QCP::srmNone);

        }

    }
}

void MainWindow::plot_sweep(QCPAxisRect* axis_rect, char pip_num, bool is_buf){
    QMutexLocker locker(&mutex);
    if(s_data.num_sweeps==0 || s_data.num_imu==0 || s_data.num_sweeps_b==0 || s_data.num_imu_b==0){
        return;
    }
    QVector<double> x_axis, data;
    if(is_buf){
        x_axis.resize(sweep_len*s_data.num_sweeps_b);
        data.resize(sweep_len*s_data.num_sweeps_b);
    }else{
        x_axis.resize(sweep_len*s_data.num_sweeps);
        data.resize(sweep_len*s_data.num_sweeps);

    }
    // QVector<double> x_axis(sweep_len*s_data.num_sweeps), data(sweep_len*s_data.num_sweeps);
    size_t expected_len = sweep_len * (is_buf ? s_data.num_sweeps_b : s_data.num_sweeps);
    int num_sweeps = is_buf ? s_data.num_sweeps_b : s_data.num_sweeps;
    auto& src = (pip_num == '0') ?
                    (is_buf ? s_data.sweep0_b : s_data.sweep0) :
                    (is_buf ? s_data.sweep1_b : s_data.sweep1);

    if (src.size() < expected_len) {
        qWarning() << "Sweep data too short:" << src.size() << "expected" << expected_len;
        return;
    }
    std::copy(src.begin(), src.begin() + expected_len, data.begin());

    for(int i=0;i<expected_len;++i){
        data[i]=data[i]*p_scale;
    }
    for(int i=0; i<num_sweeps;i++){
        double step;
        if (i < num_sweeps - 1) {
            if(is_buf){
                step = (s_data.sweep_ts_b[i+1] - s_data.sweep_ts_b[i]) / sweep_len;
            }
            else{
                step = (s_data.sweep_ts[i+1] - s_data.sweep_ts[i]) / sweep_len;
            }
        }
        else {
            if(is_buf){
                int subtractor = num_sweeps==1 ? 0 : 1;
                step = (s_data.sweep_ts_b[i] - s_data.sweep_ts_b[i-subtractor]) / sweep_len;  // fallback for last
            }
            else {
                int subtractor = num_sweeps==1 ? 0 : 1;
                step = (s_data.sweep_ts[i] - s_data.sweep_ts[i-subtractor]) / sweep_len;  // fallback for last
            }
        }

        int offset = i*sweep_len;
        for(int j=0; j<sweep_len;j++){
            if(is_buf){
                x_axis[offset+j]=(s_data.sweep_ts_b[i]+j*step)*pow(10,-6);
            } else{
                x_axis[offset+j]=(s_data.sweep_ts[i]+j*step)*pow(10,-6);
            }
        }
    }
    // plot->addGraph();
    // plot->graph(0)->setData(x_axis,data);
    QCPGraph* graph = new QCPGraph(axis_rect->axis(QCPAxis::atBottom), axis_rect->axis(QCPAxis::atLeft));
    axis_rect->axis(QCPAxis::atLeft)->setRange(0, 5.5);

    // plot->yAxis->setRange(0,5.5);
    // double visible_seconds = 10;  // seconds of data to show
    // double xmax = x_axis[x_axis.size() - 1];
    // double xmin = xmax - visible_seconds;
    double p0_t0, p0_tf, p1_t0, p1_tf;
    params.p0_t_min > s_data.sweep_ts.first()*t_scale ? p0_t0 = params.p0_t_min : p0_t0 = s_data.sweep_ts.first()*t_scale;
    params.p0_t_max < s_data.sweep_ts.last()*t_scale ? p0_tf = params.p0_t_max : p0_tf = s_data.sweep_ts.last()*t_scale;
    // params.p1_t_min > s_data.sweep_ts.first()*t_scale ? p1_t0 = params.p1_t_min : p1_t0 = s_data.sweep_ts.first()*t_scale;
    // params.p1_t_max < s_data.sweep_ts.last()*t_scale ? p1_tf = params.p1_t_max : p1_tf = s_data.sweep_ts.last()*t_scale;



    axis_rect->axis(QCPAxis::atBottom)->setRange(p0_t0,p0_tf);
    graph->setData(x_axis,data);
    // plot->xAxis->setRange(xmin, xmax);

    // plot->xAxis->setRange(x_axis[0],x_axis[x_axis.size()-1]);
    QString ylabel = QString("Pip %1 (V)").arg(pip_num);
    axis_rect->axis(QCPAxis::atLeft)->setLabel(ylabel);
    axis_rect->axis(QCPAxis::atLeft)->ticker()->setTickCount(3);
    if(!is_buf){
        int total = 0;
        for(int i=0; i<s_data.sweep_ts.size();i++){
            total+=s_data.sweep_ts[i];
        }
    }
    // calculate timestamp cadence
    // QCPLayoutElement *el = pip_plots->plotLayout()->elementAt(2, 0);
    // if (QCPTextElement *label = qobject_cast<QCPTextElement *>(el)) {
    //     label->setText(QString("Cadence: %1").arg(new_cad_value));
    // }

}

void MainWindow::plot_imu_mag(QCPAxisRect* axis_rect){
    int offset_mag = 0;
    int offset_acc = 3;
    int offset_gyr = 6;
    QVector<double> x_axis, acc_data, mag_data, gyr_data, acc_data_x, acc_data_y, acc_data_z, mag_data_x, mag_data_y, mag_data_z, gyr_data_x, gyr_data_y, gyr_data_z;
    x_axis.resize(s_data.num_imu);
    acc_data.resize(s_data.num_imu);
    mag_data.resize(s_data.num_imu);
    gyr_data.resize(s_data.num_imu);
    acc_data_x.resize(s_data.num_imu/3);
    mag_data_x.resize(s_data.num_imu/3);
    gyr_data_x.resize(s_data.num_imu/3);
    acc_data_y.resize(s_data.num_imu/3);
    mag_data_y.resize(s_data.num_imu/3);
    gyr_data_y.resize(s_data.num_imu/3);
    acc_data_z.resize(s_data.num_imu/3);
    mag_data_z.resize(s_data.num_imu/3);
    gyr_data_z.resize(s_data.num_imu/3);



    std::copy(s_data.imu_ts.begin(),s_data.imu_ts.end(),x_axis.begin());
    // implement data offset from id, this is not correct rn
    double min = 0; double max = 0;

    for(int i=0;i<acc_data.size();i++){

        acc_data[i]=get_imu_mag(i,offset_acc)*a_scale;
        mag_data[i]=get_imu_mag(i,offset_mag)*m_scale;
        gyr_data[i]=get_imu_mag(i,offset_gyr)*g_scale;
        x_axis[i]=s_data.imu_ts[i]*t_scale;
    }
    QVector<QVector<double>> acc_comps = get_imu_data(ACC_ID);
    QVector<QVector<double>> mag_comps = get_imu_data(MAG_ID);
    QVector<QVector<double>> gyr_comps = get_imu_data(GYR_ID);
    acc_data_x = acc_comps[0];
    acc_data_y = acc_comps[1];
    acc_data_z = acc_comps[2];
    mag_data_x = mag_comps[0];
    mag_data_y = mag_comps[1];
    mag_data_z = mag_comps[2];
    gyr_data_x = gyr_comps[0];
    gyr_data_y = gyr_comps[1];
    gyr_data_z = gyr_comps[2];


    int norm_len = std::min(acc_data_x.size(),acc_data.size());
    constexpr double epsilon = 1e-3;

    for (int i=0; i<norm_len;i++){
        acc_data_x[i] = (acc_data_x[i]*a_scale)/acc_data[i];
        acc_data_y[i] = (acc_data_y[i]*a_scale)/acc_data[i];
        acc_data_z[i] = (acc_data_z[i]*a_scale)/acc_data[i];
        mag_data_x[i] = (mag_data_x[i]*m_scale)/mag_data[i];
        mag_data_y[i] = (mag_data_y[i]*m_scale)/mag_data[i];
        mag_data_z[i] = (mag_data_z[i]*m_scale)/mag_data[i];
        gyr_data_x[i] = (gyr_data_x[i]*g_scale);
        gyr_data_y[i] = (gyr_data_y[i]*g_scale);
        gyr_data_z[i] = (gyr_data_z[i]*g_scale);

        // double norm = std::max(gyr_data[i],epsilon);
    //     if (gyr_data[i]<epsilon){
    //             gyr_data_x[i]=0;
    //             gyr_data_y[i]=0;
    //             gyr_data_z[i]=0;
    //         }
    //     else{
    //         gyr_data_x[i] = (gyr_data_x[i]*g_scale)/gyr_data[i];
    //         gyr_data_y[i] = (gyr_data_y[i]*g_scale)/gyr_data[i];
    //         gyr_data_z[i] = (gyr_data_z[i]*g_scale)/gyr_data[i];

    //         }


    }


    QVector<QColor> sensorColors = { Qt::red, Qt::blue, Qt::darkGreen };  // ACC, MAG, GYR
    QVector<Qt::PenStyle> axisStyles = { Qt::SolidLine, Qt::DashLine, Qt::DotLine }; // x, y, z


    QCPAxis *xAxis = axis_rect->axis(QCPAxis::atBottom);
    QCPAxis *yAxis = axis_rect->axis(QCPAxis::atLeft);
    QStringList types = {"ACC", "MAG", "GYR"};
    // QStringList types = {"ACC", "MAG"};

    QVector<QVector<double>> data_x = {acc_data_x, mag_data_x, gyr_data_x};
    QVector<QVector<double>> data_y = {acc_data_y, mag_data_y, gyr_data_y};
    QVector<QVector<double>> data_z = {acc_data_z, mag_data_z, gyr_data_z};
    QCPLegend* legend = new QCPLegend;
    axis_rect->insetLayout()->addElement(legend, Qt::AlignTop | Qt::AlignRight);
    legend->setLayer("legend");

    for (int t = 0; t < types.size(); ++t) {
        QCPGraph* gx = new QCPGraph(xAxis, yAxis);
        gx->setData(x_axis, data_x[t]);
        gx->setName(QString("%1_x").arg(types[t]));
        gx->setPen(QPen(sensorColors[t], 1.5, axisStyles[0]));
        gx->addToLegend(legend);

        QCPGraph* gy = new QCPGraph(xAxis, yAxis);
        gy->setData(x_axis, data_y[t]);
        gy->setName(QString("%1_y").arg(types[t]));
        gy->setPen(QPen(sensorColors[t], 1.5, axisStyles[1]));
        gy->addToLegend(legend);

        QCPGraph* gz = new QCPGraph(xAxis, yAxis);
        gz->setData(x_axis, data_z[t]);
        gz->setName(QString("%1_z").arg(types[t]));
        gz->setPen(QPen(sensorColors[t], 1.5, axisStyles[2]));
        gz->addToLegend(legend);

    }


    xAxis->setRange(x_axis[0],x_axis[x_axis.size()-2]);
    min = get_min(acc_data,0);
    min = get_min(mag_data,min);
    min = get_min(gyr_data,min);
    max = get_max(acc_data,0);
    max = get_max(mag_data,max);
    max = get_max(gyr_data,max);

    yAxis->setRange(min,max);
    yAxis->setLabel("IMU");
    xAxis->setLabel("Time (s)");




}

double MainWindow::get_imu_mag(int idx, int offset){
    double x = s_data.imu[idx*imu_len+offset];
    double y = s_data.imu[idx*imu_len+offset+1];
    double z = s_data.imu[idx*imu_len+offset+2];
    return qSqrt(qPow(x,2)+qPow(y,2)+qPow(z,2));

}
double MainWindow::get_min(QVector<double> d, double val){
    for(int i=0; i<d.size();i++){
        if(d[i]<val){
            val=d[i];
        }
    }
    return val;
}
double MainWindow::get_max(QVector<double> d, double val){
    for(int i=0; i<d.size();i++){
        if(d[i]>val){
            val=d[i];
        }
    }
    return val;
}
QVector<QVector<double>> MainWindow::get_imu_data(quint8 imu_id){
    QVector<double> x_axis, data, x_data, y_data, z_data;
    size_t N = s_data.num_imu;

    const auto& imu_ts = s_data.imu_ts;
    const auto& imu_vals = s_data.imu;

    // Check data consistency
    if (imu_ts.size() < N || imu_vals.size() < N * 3) {
        qWarning() << "IMU data too short:"
                   << "imu_ts size =" << imu_ts.size()
                   << ", imu_vals size =" << imu_vals.size()
                   << ", expected =" << N << "timestamps and" << (N * 3) << "values";
        QVector<QVector<double>> failure;
        return failure;
    }

    // Proceed with valid data
    x_axis.resize(N);
    data.resize(N * 3);
    std::copy(imu_ts.begin(), imu_ts.begin() + N, x_axis.begin());

    x_data.resize(N);
    y_data.resize(N);
    z_data.resize(N);

    // Fill x/y/z vectors from imu_vals
    for (size_t i = 0; i < N; ++i) {
        x_data[i] = imu_vals[3*i];
        y_data[i] = imu_vals[3*i + 1];
        z_data[i] = imu_vals[3*i + 2];
    }
    QVector<QVector<double>> result(3);
    result[0] = x_data;
    result[1] = y_data;
    result[2] = z_data;
    return result;

}
void MainWindow::plot_imu(QCPAxisRect* axis_rect, quint8 imu_id, bool is_buf){
    QMutexLocker locker(&mutex);
    if(s_data.num_sweeps==0 | s_data.num_imu==0 | s_data.num_sweeps_b==0 | s_data.num_imu_b==0){
        return;
    }

    // QVector<double> x_axis, data, x_data, y_data, z_data;

    QVector<double> x_axis, data, x_data, y_data, z_data;

    size_t N = is_buf ? s_data.num_imu_b : s_data.num_imu;

    const auto& imu_ts = is_buf ? s_data.imu_ts_b : s_data.imu_ts;
    const auto& imu_vals = is_buf ? s_data.imu_b : s_data.imu;

    // Check data consistency
    if (imu_ts.size() < N || imu_vals.size() < N * 3) {
        qWarning() << "IMU data too short:"
                   << "imu_ts size =" << imu_ts.size()
                   << ", imu_vals size =" << imu_vals.size()
                   << ", expected =" << N << "timestamps and" << (N * 3) << "values";
        return;
    }

    // Proceed with valid data
    x_axis.resize(N);
    data.resize(N * 3);
    std::copy(imu_ts.begin(), imu_ts.begin() + N, x_axis.begin());

    x_data.resize(N);
    y_data.resize(N);
    z_data.resize(N);

    // Fill x/y/z vectors from imu_vals
    for (size_t i = 0; i < N; ++i) {
        x_data[i] = imu_vals[3*i];
        y_data[i] = imu_vals[3*i + 1];
        z_data[i] = imu_vals[3*i + 2];
    }

    // if(is_buf){
    //     x_axis.resize(s_data.num_imu_b);
    //     data.resize(s_data.num_imu_b*3);
    //     std::copy(s_data.imu_ts_b.begin(), s_data.imu_ts_b.end(),x_axis.begin());
    //     x_data.resize(s_data.num_imu_b);
    //     y_data.resize(s_data.num_imu_b);
    //     z_data.resize(s_data.num_imu_b);


    // } else{
    //     x_axis.resize(s_data.num_imu);
    //     data.resize(s_data.num_imu*3);
    //     std::copy(s_data.imu_ts.begin(), s_data.imu_ts.end(),x_axis.begin());
    //     x_data.resize(s_data.num_imu);
    //     y_data.resize(s_data.num_imu);
    //     z_data.resize(s_data.num_imu);
    // }
    for(int i=0;i<x_data.size();i++){
        int offset = i*imu_len;
        if(is_buf){
            for(int j=0;j<3;j++){
                if(imu_id==MAG_ID){
                    data[3*i+j]=s_data.imu_b[offset+j]*m_scale;
                } else if (imu_id==ACC_ID){
                    data[3*i+j]=s_data.imu_b[offset+j+3]*a_scale;
                } else if (imu_id==GYR_ID){
                    data[3*i+j]=s_data.imu_b[offset+j+6]*g_scale;
                }

            }
            // x_axis[i]=i+1;
            x_axis[i]=s_data.imu_ts_b[i]*pow(10,-6);

        } else{
            for(int j=0;j<3;j++){
                if(imu_id==MAG_ID){
                    data[3*i+j]=s_data.imu[offset+j]*m_scale;
                } else if (imu_id==ACC_ID){
                    data[3*i+j]=s_data.imu[offset+j+3]*a_scale;
                } else if (imu_id==GYR_ID){
                    data[3*i+j]=s_data.imu[offset+j+6]*g_scale;
                }

            }
            // x_axis[i]=i+1;
            x_axis[i]=s_data.imu_ts[i]*pow(10,-6);

        }
    }

    double min=data[0];
    double max=data[0];
    for (int i=1;i<x_data.size()*3;i++){
        if(data[i]<min){
            min=data[i];
        }
        if(data[i]>max){
            max=data[i];
        }
    }
    for(int i=0;i<x_data.size();i++){
        x_data[i]=data[3*i];

        y_data[i]=data[3*i+1];
        z_data[i]=data[3*i+2];
    }
    QCPAxis *xAxis = axis_rect->axis(QCPAxis::atBottom);
    QCPAxis *yAxis = axis_rect->axis(QCPAxis::atLeft);
    QCPGraph* graph_x = new QCPGraph(xAxis,yAxis);
    imu_graphs.append(graph_x);
    graph_x->setData(x_axis,x_data);
    graph_x->setName("x");
    graph_x->setPen(QPen(Qt::red));
    QCPGraph* graph_y = new QCPGraph(xAxis,yAxis);
    imu_graphs.append(graph_y);
    graph_y->setData(x_axis,y_data);
    graph_y->setName("y");
    graph_y->setPen(QPen(Qt::blue));
    QCPGraph* graph_z = new QCPGraph(xAxis,yAxis);
    imu_graphs.append(graph_z);
    graph_z->setData(x_axis,z_data);
    graph_z->setName("z");
    graph_z->setPen(QPen(Qt::green));

    xAxis->setRange(x_axis[0],x_axis[x_axis.size()-2]);
    QString ylabel;
    if(imu_id==MAG_ID){
        ylabel = "MAG [G]";
    } else if (imu_id==ACC_ID){
        ylabel = "ACC [g]";
    } else if (imu_id==GYR_ID){
        ylabel = "GYR [Hz]";
    }
    yAxis->setRange(min,max);
    yAxis->setLabel(ylabel);
    // imu_plot->legend->setVisible(true);
    yAxis->ticker()->setTickCount(3);

    QCPLegend* legend = new QCPLegend;
    axis_rect->insetLayout()->addElement(legend, Qt::AlignTop | Qt::AlignRight);
    legend->setLayer("legend");
    graph_x->addToLegend(legend);
    graph_y->addToLegend(legend);
    graph_z->addToLegend(legend);

}

