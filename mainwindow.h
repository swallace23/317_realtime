#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include "qcustomplot.h"
#include "mycustomplot.h"
#include "plotwindow.h"
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QtMath>
#include <QMutex>

struct WelcomeEntries{
    QLineEdit *file;
    QComboBox *port;
    QLineEdit *baud;
    WelcomeEntries(QLineEdit* f, QComboBox* p, QLineEdit* b) : file(f), port(p), baud(b) {}
};

struct UserParams{
    QString filename;
    QString port;
    int baud;
    bool save;
    bool is_static;
    quint8 plot_choice;
    UserParams(QString f = "", QString p = "", int b = 230400, bool s = false, bool i = true, quint8 pl = 0)
        : filename(f), port(p), baud(b), save(s), is_static(i), plot_choice(pl) {}

};

struct ShieldData{
    QVector<quint16> sweep0;
    QVector<quint16> sweep1;
    QVector<quint16> sweep0_b;
    QVector<quint16> sweep1_b;
    QVector<qint16> imu;
    QVector<qint16> imu_b;
    QVector<quint32> imu_ts;
    QVector<quint32> imu_ts_b;
    QVector<quint32> sweep_ts;
    QVector<quint32> sweep_ts_b;
    quint8 shield_id;
    int num_sweeps;
    int num_sweeps_b;
    int num_imu_b;
    int num_imu;
    float cad = 0;

};
struct ChamberPlot{
    MyCustomPlot *background_plot;
    QCPColorMap *color_map;
    QCPColorMap *color_map_1;
    QCPAxisRect *map_0_rect;
    QCPAxisRect *map_1_rect;
    QCPAxisRect *imu_rect;
};

// 2D color plot - x axis is time, y axis is sweep step, color is pip value. not useful for realtime but useful for static  with actual data.
// virtual port connect one to another and write pytho nscript that takes data adnd repetadly writes to port a, then run this code with port b
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private slots:
    void startPlots(QWidget *central, QVBoxLayout *layout);
    void onSelect(QButtonGroup *group, QPushButton *file_select, WelcomeEntries entries);
    void toggle_zoom(QAbstractButton *check);
private:
    void plot_sweep(QCPAxisRect* axis_rect, char pip_num, bool is_buf = false);
    void plot_imu(QCPAxisRect* axis_rect, quint8 imu_id, bool is_buf = false);
    void plot_imu_mag(QCPAxisRect* axis_rect);
    double get_imu_mag(int idx, int offset);
    double get_min(QVector<double> d, double val);
    double get_max(QVector<double> d, double val);
    QVector<double> find_sentinel(QByteArray ba, char sentinel);
    QByteArray read_binary(QString filepath, qsizetype buffer_size);
    QString get_file(QString file_name);
    QString filepath;
    QSettings settings;
    //scaling factors for plotting calculated by Jules
    double p_scale =  5.0 / pow(2,14);
    double t_scale = 1.0/pow(10,6);
    double a_scale = 4.0/pow(2,15);
    double m_scale = 1.0/pow(2,15);
    double g_scale = 2000.0/360.0/pow(2,15);
    UserParams params;
    ShieldData s_data;
    ChamberPlot cplot;
    qsizetype buffer_size = 2056;
    quint8 message_size=147;
    quint8 sweep_len = 28;
    quint8 imu_len = 10;
    quint16 num_msgs = buffer_size/message_size+1;

    void parse(bool is_static);
    void save_settings();

    QList<MyCustomPlot*> plots;
    MyCustomPlot* pip_plots;
    MyCustomPlot* background_plot;
    QVector<QVector<double>> get_imu_data(quint8 imu_id);

    void sync_plots();
    void unsync_plots();

    QCPRange default_range;
    QCPRange default_range_b;

    void plot_static(QVBoxLayout* layout);
    void plot_dynamic(QVBoxLayout* layout);
    void plot_static_chamber(QVBoxLayout* layout);
    void plot_dynamic_chamber(MyCustomPlot* background_plot);
    void start_dynamic_chamber(QVBoxLayout* layout);
    QTimer *timer;
    QMutex mutex;

    void read_serial(QSerialPort* serial);
    void parse_serial();
    bool first_read=false;
    QByteArray *serial_bytes = new QByteArray;
    void update_dynamic(bool chamber = false);
    bool is_first = true;

    QList<QCPAxisRect*> rects;
    QList<QCPAxisRect*> rects_b;
};
#endif //MAINWINDOW_H
