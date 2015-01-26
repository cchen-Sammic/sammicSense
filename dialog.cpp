#include "qextserialport.h"
#include "qextserialenumerator.h"
#include "dialog.h"
#include "ui_dialog.h"
#include <QtCore>
#include <qthread.h>

#include <QFile>
#include <QDebug>



Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    QDateTime dateTime = QDateTime::currentDateTime();
    QString dateTimeString = dateTime.toString("yyyy_MMMdd hh_mm");
    ui->logFile->setText(dateTimeString);

    reduccionGyro = 131; //±250dps
    reduccionAccel = 16384; //±2g
    timeZero=0.0;
    connectOn= false;
    saveLogOn = false;



    //! [0]
    foreach (QextPortInfo info, QextSerialEnumerator::getPorts())
        ui->portBox->addItem(info.portName);
    //make sure user can input their own port name!
    ui->portBox->setEditable(true);

    ui->baudRateBox->addItem("1200", BAUD1200);
    ui->baudRateBox->addItem("2400", BAUD2400);
    ui->baudRateBox->addItem("4800", BAUD4800);
    ui->baudRateBox->addItem("9600", BAUD9600);
    ui->baudRateBox->addItem("38400", BAUD38400);
    ui->baudRateBox->addItem("19200", BAUD19200);
    ui->baudRateBox->addItem("115200", BAUD115200);
    ui->baudRateBox->setCurrentIndex(6);

    ui->parityBox->addItem("NONE", PAR_NONE);
    ui->parityBox->addItem("ODD", PAR_ODD);
    ui->parityBox->addItem("EVEN", PAR_EVEN);

    ui->dataBitsBox->addItem("5", DATA_5);
    ui->dataBitsBox->addItem("6", DATA_6);
    ui->dataBitsBox->addItem("7", DATA_7);
    ui->dataBitsBox->addItem("8", DATA_8);
    ui->dataBitsBox->setCurrentIndex(3);

    ui->stopBitsBox->addItem("1", STOP_1);
    ui->stopBitsBox->addItem("2", STOP_2);

    ui->queryModeBox->addItem("Polling", QextSerialPort::Polling);
    ui->queryModeBox->addItem("EventDriven", QextSerialPort::EventDriven);


    //! [0]

    ui->led->turnOff();

    timer = new QTimer(this);
    timer->setInterval(5);
    //! [1]
    PortSettings settings = {BAUD115200, DATA_8, PAR_NONE, STOP_1, FLOW_OFF, 10};
    port = new QextSerialPort(ui->portBox->currentText(), settings, QextSerialPort::Polling);
    //! [1]

    enumerator = new QextSerialEnumerator(this);
    enumerator->setUpNotifications();

    connect(ui->baudRateBox, SIGNAL(currentIndexChanged(int)), SLOT(onBaudRateChanged(int)));
    connect(ui->parityBox, SIGNAL(currentIndexChanged(int)), SLOT(onParityChanged(int)));
    connect(ui->dataBitsBox, SIGNAL(currentIndexChanged(int)), SLOT(onDataBitsChanged(int)));
    connect(ui->stopBitsBox, SIGNAL(currentIndexChanged(int)), SLOT(onStopBitsChanged(int)));
    connect(ui->queryModeBox, SIGNAL(currentIndexChanged(int)), SLOT(onQueryModeChanged(int)));
    connect(ui->timeoutBox, SIGNAL(valueChanged(int)), SLOT(onTimeoutChanged(int)));
    connect(ui->portBox, SIGNAL(editTextChanged(QString)), SLOT(onPortNameChanged(QString)));
    connect(ui->openCloseButton, SIGNAL(clicked()), SLOT(onOpenCloseButtonClicked()));
    connect(ui->sendButton, SIGNAL(clicked()), SLOT(onSendButtonClicked()));

    connect(timer, SIGNAL(timeout()), SLOT(onReadyRead()));
    connect(port, SIGNAL(readyRead()), SLOT(onReadyRead()));

    connect(timer, SIGNAL(timeout()), SLOT(onLogReady()));

    connect(enumerator, SIGNAL(deviceDiscovered(QextPortInfo)), SLOT(onPortAddedOrRemoved()));
    connect(enumerator, SIGNAL(deviceRemoved(QextPortInfo)), SLOT(onPortAddedOrRemoved()));

    connect(ui->playButton, SIGNAL(clicked()), this, SLOT(clickedConnect()));
    //    connf(ui->saveBox,);
    setupRealtimeData(ui->customPlot,ui->customPlot2);


    setWindowTitle(tr("Sammic Sense"));


}

Dialog::~Dialog()
{
    if (port->isOpen())
    {
        port->write("9");
        QThread::msleep(20);
    }
    delete ui;
    delete port;
}

void Dialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void Dialog::onPortNameChanged(const QString & /*name*/)
{
    if (port->isOpen()) {
        port->close();
        ui->led->turnOff();
    }
}
//! [2]
void Dialog::onBaudRateChanged(int idx)
{
    port->setBaudRate((BaudRateType)ui->baudRateBox->itemData(idx).toInt());
}

void Dialog::onParityChanged(int idx)
{
    port->setParity((ParityType)ui->parityBox->itemData(idx).toInt());
}

void Dialog::onDataBitsChanged(int idx)
{
    port->setDataBits((DataBitsType)ui->dataBitsBox->itemData(idx).toInt());
}

void Dialog::onStopBitsChanged(int idx)
{
    port->setStopBits((StopBitsType)ui->stopBitsBox->itemData(idx).toInt());
}

void Dialog::onQueryModeChanged(int idx)
{
    port->setQueryMode((QextSerialPort::QueryMode)ui->queryModeBox->itemData(idx).toInt());
}

void Dialog::onTimeoutChanged(int val)
{
    port->setTimeout(val);
}
//! [2]
//! [3]
void Dialog::onOpenCloseButtonClicked()
{
    if (!port->isOpen()) {
        port->setPortName(ui->portBox->currentText());
        port->open(QIODevice::ReadWrite);
//        if(ui->saveBox->isChecked())
//        {
//            saveLogOn= true;
//        }
    }
    else {
        port->close();
    }

    //If using polling mode, we need a QTimer
    if (port->isOpen() && port->queryMode() == QextSerialPort::Polling)
        timer->start();
    else
        timer->stop();

    //update led's status
    ui->led->turnOn(port->isOpen());
}

//! [3]
//! [4]
void Dialog::onSendButtonClicked()
{
    if (port->isOpen() && !ui->sendEdit->text().isEmpty())
        port->write(ui->sendEdit->text().toLatin1());
    ui->sendEdit->clear();
}

void Dialog::onReadyRead()
{
    if (port->bytesAvailable()) {
        ui->recvEdit->moveCursor(QTextCursor::End);
        if(connectOn==false){
            ui->recvEdit->insertPlainText(datosPort = QString::fromLatin1(port->readLine()));
        }else datosPort = QString::fromLatin1(port->readLine());
    }

}

void Dialog::onPortAddedOrRemoved()
{
    QString current = ui->portBox->currentText();

    ui->portBox->blockSignals(true);
    ui->portBox->clear();
    foreach (QextPortInfo info, QextSerialEnumerator::getPorts())
        ui->portBox->addItem(info.portName);

    ui->portBox->setCurrentIndex(ui->portBox->findText(current));

    ui->portBox->blockSignals(false);
}

//! [4]




//Chao
void Dialog::clickedConnect()
{
    if (port->isOpen())
    {
        if(connectOn==false){
            port->write("0");
            ui->playButton->setText("PLAY");
            timeZero = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0-4;
            connectOn= true;
            if(ui->saveBox->isChecked())
            {
                saveLogOn= true;
            }
        }
        else
        {
            port->write("1");
            ui->playButton->setText("PAUSE");
            connectOn= false;

        }
    }
}

//Chao
float Dialog::filterMedian(float valor, QList<float> vector[])
{
    vector->prepend(valor);
    int vectorSize = vector->size();
    int posMedian;
    float returnValue;

    QVector<float> vectorTemp = vector->toVector();


    //ordenación de mayor a menor
    qStableSort(vectorTemp.begin(),vectorTemp.end());
    if(vectorSize%2==0){
        posMedian = vectorSize/2 ;
        returnValue= (vectorTemp.at(posMedian-1)+vectorTemp.at(posMedian))/2;
    }
    else
    {
        posMedian = static_cast<int>((vectorSize+1)/2);
        returnValue= vectorTemp.at(posMedian-1);
    }
    int boxValue = 20;
    if(vectorSize>=boxValue){
        vector->removeLast();
    }

    //condición umbral
    if(abs(valor-returnValue)> 1)
    {
        returnValue= valor;
    }


    //    vector->replace(0,returnValue);
    return returnValue;
}


float Dialog::filterAlgebraic_Estimation(float valor, QList<float> vector[])
{
    vector->prepend(valor);
    int vectorSize = vector->size();
    float suma = 0;
    float tempSuma;
    float tempSumaPeso;
    int windowValue = 20;
    float stimation =0;


    for(int i=0;i<vectorSize;i++)
    {
        tempSumaPeso =float(2*vectorSize - 3*i);
        tempSuma = vector->at(i)*tempSumaPeso;

        /*Suma*/
        suma = tempSuma + tempSumaPeso+ suma;
    }

    if(vectorSize>=windowValue){
        stimation = 2*suma/(vectorSize*vectorSize);
        vector->removeLast();
    }
    return stimation;
}


float Dialog::filterAlgebraic_DerivateEstimation(float valor, QList<float> vector[])
{
    vector->prepend(valor);
    int vectorSize = vector->size();
    float suma = 0;
    float tempSuma;
    float tempSumaPeso;
    int windowValue = 20;
    float derivada = 0;

    /*Derivada M Fliess */
    for(int i=0;i<vectorSize;i++)
    {
        tempSumaPeso =float(vectorSize - 2*i);
        tempSuma = vector->at(i)*tempSumaPeso;

        /*Suma*/
        suma = tempSuma + tempSumaPeso+ suma;
    }



    if(vectorSize>=windowValue){
        derivada = -6*suma/(vectorSize*vectorSize*vectorSize*0.05);
        vector->removeLast();
    }
    return derivada;
}


//chao
void Dialog::onLogReady()
{

    if(!datosPort.compare(datosPortTEMP)==0) // comparación de qstring i con i-1
    {
        listDatos = datosPort.split(",", QString::SkipEmptyParts);
//        ui->recvEditLog->moveCursor(QTextCursor::End);
        static QString datosLog;

        if (listDatos[0]=="$a/g"){
            graphONag=true;
            //            QStringList listDatosCopy = listDatos;
            //            listDatosCopy[0]=QString("ax: %1").arg(listDatos[1]);
            //            listDatosCopy[1]=QString("ay: %1").arg(listDatos[2]);
            //            listDatosCopy[2]=QString("az: %1").arg(listDatos[3]);
            //            listDatosCopy[3]=QString("\tgx: %1").arg(listDatos[4]);
            //            listDatosCopy[4]=QString("gy: %1").arg(listDatos[5]);
            //            listDatosCopy[5]=QString("gz: %1").arg(listDatos[6]);
            //            listDatosCopy.removeLast();

            QStringList listDatosCopy = listDatos;
            listDatosCopy[0]=QString("%1, ").arg(listDatos[1]);
            listDatosCopy[1]=QString("%1, ").arg(listDatos[2]);
            listDatosCopy[2]=QString("%1, ").arg(listDatos[3]);
            listDatosCopy[3]=QString("%1, ").arg(listDatos[4]);
            listDatosCopy[4]=QString("%1, ").arg(listDatos[5]);
            listDatosCopy[5]=QString("%1, ").arg(listDatos[6]);
            listDatosCopy[6]=QString("%1").arg(key);
            //            listDatosCopy.removeLast();
            datosLog = listDatosCopy.join(" ");
//            qDebug()<<datosPort;
//            qDebug()<<datosLog;
//                        ui->recvEditLog->insertPlainText(listDatosCopy.join("   "));
        }
        else graphONag= false;

        //guardar en archivo

        if(saveLogOn)
        {
            QString directory = "../log/"+ui->logFile->text()+".txt";
            QFile file(directory);
            if(file.open(QIODevice::WriteOnly | QIODevice::Text| QIODevice::Append)){
                QTextStream out(&file);
                 qDebug()<<datosLog;
                out<<datosLog<<"\n";//datosPort
                file.close();
                datosPortTEMP = datosPort;
            }
        }

    }

}

//chao
void Dialog::setupRealtimeData(QCustomPlot *customPlot,QCustomPlot *customPlot2)
{
#if QT_VERSION < QT_VERSION_CHECK(4, 7, 0)
    QMessageBox::critical(this, "", "You're using Qt < 4.7, the realtime data demo needs functions that are available with Qt 4.7 to work properly");
#endif
    demoName = "Real Time Data";

    customPlot->addGraph(); // blue line
    customPlot->graph(0)->setPen(QPen(Qt::blue));
    //  customPlot->graph(0)->setBrush(QBrush(QColor(240, 255, 200)));
    customPlot->graph(0)->setAntialiasedFill(false);
    customPlot->addGraph(); // red line
    customPlot->graph(1)->setPen(QPen(Qt::red));
    //  customPlot->graph(0)->setChannelFillGraph(customPlot->graph(1));
    customPlot->addGraph(); //green line
    customPlot->graph(2)->setPen(QPen(Qt::green));
    //  customPlot->graph(1)->setChannelFillGraph(customPlot->graph(2));


    customPlot->addGraph(); // blue dot
    customPlot->graph(3)->setPen(QPen(Qt::blue));
    customPlot->graph(3)->setLineStyle(QCPGraph::lsNone);
    customPlot->graph(3)->setScatterStyle(QCPScatterStyle::ssDisc);
    customPlot->addGraph(); // red dot
    customPlot->graph(4)->setPen(QPen(Qt::red));
    customPlot->graph(4)->setLineStyle(QCPGraph::lsNone);
    customPlot->graph(4)->setScatterStyle(QCPScatterStyle::ssDisc);
    customPlot->addGraph(); // green dot
    customPlot->graph(5)->setPen(QPen(Qt::green));
    customPlot->graph(5)->setLineStyle(QCPGraph::lsNone);
    customPlot->graph(5)->setScatterStyle(QCPScatterStyle::ssDisc);

    customPlot->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    customPlot->xAxis->setDateTimeFormat("mm:ss");
    customPlot->xAxis->setAutoTickStep(false);
    customPlot->xAxis->setTickStep(2);
    customPlot->xAxis2->setLabel("g-force      scale range ±2g");
    customPlot->yAxis->setRange(0.0,4.0, Qt::AlignCenter);
    customPlot->axisRect()->setupFullAxesBox();

    // make left and bottom axes transfer their ranges to right and top axes:
    connect(customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->xAxis2, SLOT(setRange(QCPRange)));
    connect(customPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->yAxis2, SLOT(setRange(QCPRange)));


    customPlot2->addGraph(); // blue line
    customPlot2->graph(0)->setPen(QPen(Qt::blue));
    //  customPlot2->graph(0)->setBrush(QBrush(QColor(240, 255, 200)));
    customPlot2->graph(0)->setAntialiasedFill(false);
    customPlot2->addGraph(); // red line
    customPlot2->graph(1)->setPen(QPen(Qt::red));
    //  customPlot2->graph(0)->setChannelFillGraph(customPlot->graph(1));
    customPlot2->addGraph(); //green line
    customPlot2->graph(2)->setPen(QPen(Qt::green));
    //  customPlot2->graph(1)->setChannelFillGraph(customPlot->graph(2));

    customPlot2->addGraph(); // blue dot
    customPlot2->graph(3)->setPen(QPen(Qt::blue));
    customPlot2->graph(3)->setLineStyle(QCPGraph::lsNone);
    customPlot2->graph(3)->setScatterStyle(QCPScatterStyle::ssDisc);
    customPlot2->addGraph(); // red dot
    customPlot2->graph(4)->setPen(QPen(Qt::red));
    customPlot2->graph(4)->setLineStyle(QCPGraph::lsNone);
    customPlot2->graph(4)->setScatterStyle(QCPScatterStyle::ssDisc);
    customPlot2->addGraph(); // green dot
    customPlot2->graph(5)->setPen(QPen(Qt::green));
    customPlot2->graph(5)->setLineStyle(QCPGraph::lsNone);
    customPlot2->graph(5)->setScatterStyle(QCPScatterStyle::ssDisc);

    customPlot2->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    customPlot2->xAxis->setDateTimeFormat("mm:ss");
    customPlot2->xAxis->setAutoTickStep(false);
    customPlot2->xAxis->setTickStep(2);
    customPlot2->xAxis2->setLabel("Angular rate º/s       scale range ±250dps");
    customPlot2->yAxis->setRange(0.0,720, Qt::AlignCenter);
    customPlot2->axisRect()->setupFullAxesBox();

    // make left and bottom axes transfer their ranges to right and top axes:
    connect(customPlot2->xAxis, SIGNAL(rangeChanged(QCPRange)), customPlot2->xAxis2, SLOT(setRange(QCPRange)));
    connect(customPlot2->yAxis, SIGNAL(rangeChanged(QCPRange)), customPlot2->yAxis2, SLOT(setRange(QCPRange)));



    // setup a timer that repeatedly calls MainWindow::realtimeDataSlot:
    connect(&dataTimer, SIGNAL(timeout()), this, SLOT(realtimeDataSlot()));
    dataTimer.start(0); // Interval 0 means to refresh as fast as possible

}



//chao
void Dialog::realtimeDataSlot()
{

    if (graphONag)  //(graphONag)
    {
        key = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0 - timeZero;
        //// Gráfica 1
        value1_0 = (listDatos[1].toFloat())/reduccionAccel;
        value1_1 = (listDatos[2].toFloat())/reduccionAccel;
        value1_2 = (listDatos[3].toFloat())/reduccionAccel;

        value1_0 = filterMedian(value1_0,vector_1);
        value1_1 = filterMedian(value1_1,vector_2);
        value1_2 = filterMedian(value1_2,vector_3);


        value1_0 = filterAlgebraic_Estimation(value1_0,vector2_1);
        value1_1 = filterAlgebraic_Estimation(value1_1,vector2_2);
        value1_2 = filterAlgebraic_Estimation(value1_2,vector2_3);

        value1_0 = value1_0 - 1.045;
        value1_1 = value1_1 - 1.045;
        value1_2 = value1_2 - 1.08;

        // add data to lines:
        ui->customPlot->graph(0)->addData(key, value1_0);
        ui->customPlot->graph(1)->addData(key, value1_1);
        ui->customPlot->graph(2)->addData(key, value1_2);
        // set data of dots:
        ui->customPlot->graph(3)->clearData();
        ui->customPlot->graph(3)->addData(key, value1_0);
        ui->customPlot->graph(4)->clearData();
        ui->customPlot->graph(4)->addData(key, value1_1);
        ui->customPlot->graph(5)->clearData();
        ui->customPlot->graph(5)->addData(key, value1_2);
        // remove data of lines that's outside visible range:
        ui->customPlot->graph(0)->removeDataBefore(key-8);
        ui->customPlot->graph(1)->removeDataBefore(key-8);
        ui->customPlot->graph(2)->removeDataBefore(key-8);
        // rescale value (vertical) axis to fit the current data:
        //    ui->customPlot->graph(0)->rescaleValueAxis();
        //    ui->customPlot->graph(1)->rescaleValueAxis();
        //    ui->customPlot->graph(2)->rescaleValueAxis();

        //// Gráfica 2
        value2_0 = (listDatos[4].toFloat())/reduccionGyro;
        value2_1 = (listDatos[5].toFloat())/reduccionGyro;
        value2_2 = (listDatos[6].toFloat())/reduccionGyro;

        value2_0 = filterAlgebraic_Estimation(value2_0,vector_4);
        value2_1 = filterAlgebraic_Estimation(value2_1,vector_5);
        value2_2 = filterAlgebraic_Estimation(value2_2,vector_6);

        value2_0 = filterMedian(value2_0,vector_4);
        value2_1 = filterMedian(value2_1,vector_5);
        value2_2 = filterMedian(value2_2,vector_6);


        // add data to lines:
        ui->customPlot2->graph(0)->addData(key, value2_0);
        ui->customPlot2->graph(1)->addData(key, value2_1);
        ui->customPlot2->graph(2)->addData(key, value2_2);
        // set data of dots:
        ui->customPlot2->graph(3)->clearData();
        ui->customPlot2->graph(3)->addData(key, value2_0);
        ui->customPlot2->graph(4)->clearData();
        ui->customPlot2->graph(4)->addData(key, value2_1);
        ui->customPlot2->graph(5)->clearData();
        ui->customPlot2->graph(5)->addData(key, value2_2);
        // remove data of lines that's outside visible range:
        ui->customPlot2->graph(0)->removeDataBefore(key-8);
        ui->customPlot2->graph(1)->removeDataBefore(key-8);
        ui->customPlot2->graph(2)->removeDataBefore(key-8);
        // rescale value (vertical) axis to fit the current data:
        //    ui->customPlot2->graph(0)->rescaleValueAxis();
        //    ui->customPlot2->graph(1)->rescaleValueAxis();
        //    ui->customPlot2->graph(2)->rescaleValueAxis();


        // make key axis range scroll with the data (at a constant range size of 8):
        ui->customPlot->xAxis->setRange(key+0.15, 8, Qt::AlignRight);
        ui->customPlot->replot();

        // make key axis range scroll with the data (at a constant range size of 8):
        ui->customPlot2->xAxis->setRange(key+0.15, 8, Qt::AlignRight);
        ui->customPlot2->replot();

        ui->graph1name_1->setText(QString("Accel X"));
        ui->graph1name_2->setText(QString("Accel Y"));
        ui->graph1name_3->setText(QString("Accel Z"));
        ui->graph2name_1->setText(QString("Gyro X"));
        ui->graph2name_2->setText(QString("Gyro Y"));
        ui->graph2name_3->setText(QString("Gyro Z"));


        // calculate frames per second:
        static float lastFpsKey;
        static int frameCount;
        ++frameCount;
        float average = key-lastFpsKey;
        if (average > 2) // average fps over 1 seconds
        {
            ui->statusPlot->setReadOnly(true);
            ui->statusPlot->setText(
                        QString("  %1 FPS,  Total Data points: %2")
                        .arg(frameCount/(key-lastFpsKey), 0, 'f', 0)
                        .arg(ui->customPlot->graph(0)->data()->count()+ui->customPlot->graph(1)->data()->count()+ui->customPlot->graph(2)->data()->count())
                        );
            lastFpsKey = key;
            frameCount = 0;
        }
    }
}

