#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include<QFile>
#include <QTimer>
#include <QMainWindow>
#include "qcustomplot.h"

namespace Ui {
    class Dialog;
}
class QTimer;
class QextSerialPort;
class QextSerialEnumerator;

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();
    QString datosPort;
    QString datosPortTEMP;
//    QString directory;
    QFile file;
    void setupRealtimeData(QCustomPlot *customPlot, QCustomPlot *customPlot2);

    float filterAlgebraic_DerivateEstimation(float valor, QList<float>vector[]);
    float filterAlgebraic_Estimation(float valor, QList<float>vector[]);
    float filterMedian(float valor, QList<float>vector[]);

    QList<float> vectorTemp[10];

    QList<float> vector_1[10];
    QList<float> vector_2[10];
    QList<float> vector_3[10];
    QList<float> vector_4[10];
    QList<float> vector_5[10];
    QList<float> vector_6[10];

    QList<float> vector2_1[10];
    QList<float> vector2_2[10];
    QList<float> vector2_3[10];

    int reduccionGyro;
    int reduccionAccel;

    float value1_0;
    float value1_1;
    float value1_2;
    float value2_0;
    float value2_1;
    float value2_2;

    float timeZero;
    float key;



protected:
    void changeEvent(QEvent *e);

private Q_SLOTS:
    void onPortNameChanged(const QString &name);
    void onBaudRateChanged(int idx);
    void onParityChanged(int idx);
    void onDataBitsChanged(int idx);
    void onStopBitsChanged(int idx);
    void onQueryModeChanged(int idx);
    void onTimeoutChanged(int val);
    void onOpenCloseButtonClicked();
    void onSendButtonClicked();
    void onReadyRead();

    void onPortAddedOrRemoved();

    void onLogReady(); //registro en txt

    void realtimeDataSlot(); //gráfica

    void clickedConnect(); //botón conexión arduino

private:
    Ui::Dialog *ui;
    QTimer *timer;
    QextSerialPort *port;
    QextSerialEnumerator *enumerator;

    QString demoName;
    QTimer dataTimer;
    bool graphONag;
    bool connectOn;
    bool saveLogOn;
    QStringList listDatos;

};

#endif // DIALOG_H
