#ifndef BKKDEVSCARD_H
#define BKKDEVSCARD_H

#include "bkkdevscard_global.h"
#include <QObject>
#include <QThread>

// PCSC Support.
#include <winscard.h>
#pragma comment (lib, "winscard.lib")

class SCardMonitor : public QObject
{
    Q_OBJECT

public:
    explicit SCardMonitor(QString appPath, QObject *parent = 0);
    ~SCardMonitor();

signals:
    void scardInserted();
    void scardRemoved();

    void gotCitizenID(QString cid);
    void gotPersonalInfo(QString pinfo);
    void gotAddress(QString addr);
    void gotPhoto();
    void gotCardIssueExpire(QString exp);
    void gotCardIssuer(QString iss);

public slots:
    void monitor();
    void stop();
    void read_nophoto(bool bPhoto);
    void scard_connect();
    void scard_disconnect();

private:
    bool m_bRunning;
    int m_timerId;

    SCARDCONTEXT		hContext; // Context
    SCARDHANDLE			hCardReader;    // Handle of SCReader
    SCARD_IO_REQUEST    pioSendPci;
    SCARD_IO_REQUEST    pioRecvPci;

    char cardAttribute[2];

    bool checkSum();
    bool SCardMonitor::transmit(const QByteArray &cmdApdu, QByteArray &respApdu);

    void sendCommand_Select();
    QString sendCommand_CitizenID();
    QString sendCommand_PersonInfo();
    QString sendCommand_Address();
    QString sendCommand_CardIssueExpire();
    QString sendCommand_CardIssuer();

    void read_photo();

    QString m_strAppDir;

protected:
     void timerEvent(QTimerEvent *event);
};

class BKKDEVSCARDSHARED_EXPORT BkkdevScard : public QObject
{
    Q_OBJECT

public:
    explicit BkkdevScard(QString &appPath, QObject *parent = 0);
    ~BkkdevScard();

signals:
    void bkkScardInserted();
    void bkkScardRemoved();
    void stop_monitor();
    void start_monitor();

    void scard_read_data(bool bPhoto);

    void scardGotCitizenID(QString cid);
    void scardGotPersonalInfo(QString pinfo);
    void scardGotAddress(QString addr);
    void scardGotPhoto();
    void scardGotCardIssueExpire(QString exp);
    void scardGotCardIssuer(QString iss);

private:
    QThread *m_pThread;
};

#endif // BKKDEVSCARD_H
