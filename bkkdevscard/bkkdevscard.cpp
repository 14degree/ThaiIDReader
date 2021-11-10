#include "bkkdevscard.h"
#include <QEventLoop>
#include <QCoreApplication>
#include <QDebug>
#include <QRegExp>
#include <QTextCodec>
#include <QDir>

#include <iostream>
#include <sstream>
#include <fstream>

BkkdevScard::BkkdevScard(QString &appPath,QObject *parent)
    : QObject(parent)
{
    // create thread to process image
    m_pThread = new QThread(this);
    SCardMonitor* worker = new SCardMonitor(appPath);
    worker->moveToThread(m_pThread);

    //connect(m_pThread, SIGNAL (started()), worker, SLOT (monitor()));

    connect(this, SIGNAL(start_monitor()), worker, SLOT(monitor()));
    connect(this, SIGNAL(stop_monitor()), worker, SLOT(stop()));
    connect(worker, SIGNAL(scardInserted()), this, SIGNAL(bkkScardInserted()));
    connect(worker, SIGNAL(scardRemoved()), this, SIGNAL(bkkScardRemoved()));

    connect(this, SIGNAL(scard_read_data(bool)), worker, SLOT(read_nophoto(bool)));

    connect(worker, SIGNAL(gotCitizenID(QString)), this, SIGNAL(scardGotCitizenID(QString)));
    connect(worker, SIGNAL(gotPersonalInfo(QString)), this, SIGNAL(scardGotPersonalInfo(QString)));
    connect(worker, SIGNAL(gotAddress(QString)), this, SIGNAL(scardGotAddress(QString)));
    connect(worker, SIGNAL(gotCardIssueExpire(QString)), this, SIGNAL(scardGotCardIssueExpire(QString)));
    connect(worker, SIGNAL(gotCardIssuer(QString)), this, SIGNAL(scardGotCardIssuer(QString)));
    connect(worker, SIGNAL(gotPhoto()), this, SIGNAL(scardGotPhoto()));

    m_pThread->start();
}

BkkdevScard::~BkkdevScard()
{
    emit stop_monitor();
    m_pThread->quit();
    m_pThread->wait();
}

SCardMonitor::SCardMonitor(QString appPath, QObject *parent)
    : QObject(parent)
{
    m_bRunning = false;
    m_strAppDir = appPath;
}

SCardMonitor::~SCardMonitor()
{
    qDebug() << "monitor was killed";

    int rv;

    if(hContext)
    {
        if(hCardReader)
            rv = ::SCardFreeMemory(hContext, (LPCVOID)hCardReader);

        rv = ::SCardReleaseContext(hContext);
    }
}

void SCardMonitor::stop()
{
    m_bRunning = false;
    killTimer(m_timerId);
}

void SCardMonitor::timerEvent(QTimerEvent *event)
{
    LPTSTR       szReaderList = NULL;
    DWORD        dwReaderListSize = 0, dwNewState, dwOldState;
    HRESULT      hr;

    bool fEvent = false;

    hr = SCardEstablishContext(SCARD_SCOPE_USER, NULL, NULL, &hContext);

    unsigned long AUTO_ALLOC = SCARD_AUTOALLOCATE;
    long ret = ::SCardListReaders(hContext, NULL, (LPTSTR)&szReaderList, &AUTO_ALLOC);

    if (ret != SCARD_S_SUCCESS)
    {
        //QString msg("no reader available : ");
        //qDebug() << msg;

        SCardReleaseContext(hContext);

    } else {

        qDebug() << "found smartcard reader";

        // create the readerState array
        SCARD_READERSTATE readerState;

        readerState.szReader		= (LPCTSTR)szReaderList; // name of the card reader to monitor
        readerState.dwCurrentState	= SCARD_STATE_UNAWARE;
        readerState.dwEventState	= SCARD_STATE_UNKNOWN;

        hr = SCardGetStatusChange(hContext, INFINITE, &readerState, 1);

        fEvent = false;

        if ((readerState.dwEventState & SCARD_STATE_EMPTY) == SCARD_STATE_EMPTY)
        {
            // Card Removed
            emit scardRemoved();
        }
        else if ((readerState.dwEventState & SCARD_STATE_PRESENT) == SCARD_STATE_PRESENT)
        {
            // Card Inserted
            emit scardInserted();
        }

        do
        {
            // Only interested in state changes which don't involve
            // attaching/detaching the card.
            DWORD dwStateMask = ~(SCARD_STATE_INUSE   |
            SCARD_STATE_EXCLUSIVE |
            SCARD_STATE_UNAWARE   |
            SCARD_STATE_IGNORE    |
            SCARD_STATE_CHANGED);

            readerState.dwCurrentState = readerState.dwEventState;

            try
            {
                hr = SCardGetStatusChange(hContext, 500, &readerState, 1);
            }
            catch(...)
            {
                // don't care what the error is -- just leave
                hr = SCARD_E_CANCELLED;
            }

            // SCardCancel allows the loop to exit
            // any thing other than NOERROR makes this loop exit
            if (!(hr == NOERROR) && !(hr == SCARD_E_TIMEOUT))
            {
                // this is the default exit route for this function
                return;
            }

            dwNewState = readerState.dwEventState & dwStateMask;
            dwOldState = readerState.dwCurrentState & dwStateMask;
            if (dwNewState != dwOldState)
            {
                fEvent = true;
            }

            QCoreApplication::processEvents();

        } while (!fEvent && m_bRunning);
    }
}

void SCardMonitor::monitor()
{
    if(!m_bRunning)
    {
        m_bRunning = true;
        m_timerId = startTimer(1000);
        qDebug() <<"timer id : " << m_timerId;
    }
}

void SCardMonitor::scard_connect()
{
    unsigned long AUTO_ALLOC = SCARD_AUTOALLOCATE;
    LPTSTR szReaderList = NULL;
    long rv;
    DWORD protocol;

    rv = ::SCardListReaders(hContext, NULL, (LPTSTR)&szReaderList, &AUTO_ALLOC);

    if(rv != SCARD_S_SUCCESS)
        return ;

    //qDebug() << "connect : got list";

    rv = ::SCardConnect(hContext, szReaderList,
            SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,
            &hCardReader, &protocol);

    if(rv != SCARD_S_SUCCESS)
    {
        qDebug() << "connect : failed";
        return ;
    }

    //qDebug() << "connect : connected";

    switch(protocol)
    {
        case SCARD_PROTOCOL_T0:
            pioSendPci = *SCARD_PCI_T0;
            break;

        case SCARD_PROTOCOL_T1:
            pioSendPci = *SCARD_PCI_T1;
            break;

        case SCARD_PROTOCOL_UNDEFINED:
        default:
            qDebug() << "Active protocol unnegotiated or unknown";
            break;

    }

    pioRecvPci.dwProtocol = protocol;
    pioRecvPci.cbPciLength = 255;

    // check atr
    // Determine the status.
    DWORD           cch = 200;
    BYTE            bAttr[32];
    DWORD           cByte = 32;
    DWORD           dwState, dwProtocol;

    rv = SCardStatus(hCardReader,
                          szReaderList,
                          &cch,
                          &dwState,
                          &dwProtocol,
                          (LPBYTE)&bAttr,
                          &cByte);

    if(rv != SCARD_S_SUCCESS)
    {
        qDebug() << "get status failed";
        return ;
        //throw SCardException(rv);
    }

    if (bAttr == nullptr || cByte < 2)
    {
        qDebug() << "wrong attribute ! ";
        return ;
        //throw SCardException(SCARD_E_INVALID_ATR);
    }

    cardAttribute[0] = bAttr[0];
    cardAttribute[1] = bAttr[1];

    if (bAttr[0] == 0x3B && bAttr[1] == 0x68) //Smart card tested with old type (Figure A.)
    {
        qDebug() << "smart card type : Old 3B68\n";
    }
    else if (bAttr[0] == 0x3B && bAttr[1] == 0x78) //Smart card tested with new type (figure B.)
    {
        qDebug() << "smart card type : 3B78\n";
    }
    else if (bAttr[0] == 0x3B && bAttr[1] == 0x79) // 2016-10
    {
        qDebug() << "smart card type : 3B79\n";
    }
    else if (bAttr[0] == 0x3B && bAttr[1] == 0x67)
    {
        qDebug() << "smart card type : 3B67\n";
    }
    else
    {
        if(!checkSum())
            qDebug() << "Unsupported Card Reader";
            //throw SCardException(SCARD_W_UNSUPPORTED_CARD);
        else
            qDebug() << "Supported CardReader";
    }
}

void SCardMonitor::scard_disconnect()
{
    int rv = ::SCardDisconnect(hCardReader, SCARD_LEAVE_CARD);

    if(rv != SCARD_S_SUCCESS)
        qDebug() << "disconnect card failed";
}

bool SCardMonitor::checkSum()
{
        // 1. send command SELECT
        sendCommand_Select();

        Sleep(100); // wait for reader get ready

        // 2. send command to get id
        QString strCID = sendCommand_CitizenID();

        if(strCID.length() != 13) {
            qDebug() << "checksum read CitizenID : false";
            return false;
        }

        int sum = 0;
        for(int i=0; i<12; i++)
        {
            sum += QString(strCID.at(i)).toInt() * (13 - i);
        }

        if(((11 - (sum % 11)) % 10) == QString(strCID.at(12)).toInt())
        {
            qDebug() << "Ok, this is a supported card";
            return true;
        }

        qDebug() << "checksum : false";
        return false;
}

bool SCardMonitor::transmit(const QByteArray &cmdApdu, QByteArray &respApdu)
{
    BYTE resp[256]; //Maximum buffer size
    unsigned long respLength = 256;

    int rv = ::SCardTransmit(hCardReader, &pioSendPci, (LPCBYTE)cmdApdu.data(), cmdApdu.size(),
                             &pioRecvPci, resp, &respLength);

    if(rv != SCARD_S_SUCCESS)
        return false;
        //throw SCardException(rv);

    respApdu.clear();

    for(int i=0; i < respLength; i++){

        respApdu.append(resp[i]);
    }

    return true;
}

void SCardMonitor::sendCommand_Select()
{
    QByteArray cmdAdpu("\x00\xA4\x04\x00\x08\xA0\x00\x00\x00\x54\x48\x00\x01", 13);

    QByteArray resp;
    transmit(cmdAdpu, resp);
}

QString SCardMonitor::sendCommand_CitizenID()
{
    QByteArray cmd_1("\x80\xb0\x00\x04\x02\x00\x0d", 7);
    QByteArray cmd_2("\x00\xc0\x00\x00\x0d", 5);

    QByteArray cmd367_1("\x80\xb0\x00\x04\x02\x00\x0d", 7);
    QByteArray cmd367_2("\x00\xc0\x00\x01\x0d", 5);

    QByteArray *pCmd_1 = NULL;
    QByteArray *pCmd_2 = NULL;

    if (cardAttribute[0] == 0x3B && cardAttribute[1] == 0x67) {
        pCmd_1 = &cmd367_1;
        pCmd_2 = &cmd367_2;
    } else {
        pCmd_1 = &cmd_1;
        pCmd_2 = &cmd_2;
    }

    QByteArray resp_1;
    transmit(*pCmd_1, resp_1);

    QByteArray resp_2;
    transmit(*pCmd_2, resp_2);

    QString strCID = QString(resp_2).remove(QRegExp("[^\\d]"));
    //qDebug() << "got citizen ID : " << strCID;
    return strCID;
}

QString SCardMonitor::sendCommand_PersonInfo()
{
    QByteArray cmd_1("\x80\xb0\x00\x11\x02\x00\xd1", 7);
    QByteArray cmd_2("\x00\xc0\x00\x00\xd1", 5);

    QByteArray cmd367_1("\x80\xb0\x00\x11\x02\x00\xd1", 7);
    QByteArray cmd367_2("\x00\xc0\x00\x01\xd1", 5);

    QByteArray *pCmd_1 = NULL;
    QByteArray *pCmd_2 = NULL;

    if (cardAttribute[0] == 0x3B && cardAttribute[1] == 0x67) {
        pCmd_1 = &cmd367_1;
        pCmd_2 = &cmd367_2;
    } else {
        pCmd_1 = &cmd_1;
        pCmd_2 = &cmd_2;
    }

    QByteArray resp_1;
    transmit(*pCmd_1, resp_1);

    QByteArray resp_2;
    transmit(*pCmd_2, resp_2);

    QTextCodec *codec = QTextCodec::codecForName("TIS-620");
    QString str = codec->toUnicode(resp_2).trimmed();

    return str;
}

QString SCardMonitor::sendCommand_Address() {

    QByteArray cmd_1("\x80\xb0\x15\x79\x02\x00\x64", 7);
    QByteArray cmd_2("\x00\xc0\x00\x00\x64", 5);

    QByteArray cmd367_1("\x80\xb0\x15\x79\x02\x00\x64", 7);
    QByteArray cmd367_2("\x00\xc0\x00\x01\x64", 5);

    QByteArray *pCmd_1 = NULL;
    QByteArray *pCmd_2 = NULL;

    if (cardAttribute[0] == 0x3B && cardAttribute[1] == 0x67) {
        pCmd_1 = &cmd367_1;
        pCmd_2 = &cmd367_2;
    } else {
        pCmd_1 = &cmd_1;
        pCmd_2 = &cmd_2;
    }

    QByteArray resp_1;
    transmit(*pCmd_1, resp_1);

    QByteArray resp_2;
    transmit(*pCmd_2, resp_2);

    QTextCodec *codec = QTextCodec::codecForName("TIS-620");
    QString str = codec->toUnicode(resp_2).trimmed();

    return str;
}

QString SCardMonitor::sendCommand_CardIssueExpire()
{
    //0x80, 0xb0, 0x01, 0x67, 0x02, 0x00, 0x12

    QByteArray cmd_1("\x80\xb0\x01\x67\x02\x00\x12", 7);
    QByteArray cmd_2("\x00\xc0\x00\x00\x12", 5);

    QByteArray cmd367_1("\x80\xb0\x01\x67\x02\x00\x12", 7);
    QByteArray cmd367_2("\x00\xc0\x00\x01\x12", 5);

    QByteArray *pCmd_1 = NULL;
    QByteArray *pCmd_2 = NULL;

    if (cardAttribute[0] == 0x3B && cardAttribute[1] == 0x67) {
        pCmd_1 = &cmd367_1;
        pCmd_2 = &cmd367_2;
    } else {
        pCmd_1 = &cmd_1;
        pCmd_2 = &cmd_2;
    }

    QByteArray resp_1;
    transmit(*pCmd_1, resp_1);

    QByteArray resp_2;
    transmit(*pCmd_2, resp_2);

    QTextCodec *codec = QTextCodec::codecForName("TIS-620");
    QString str = codec->toUnicode(resp_2).trimmed();

    return str;
}

QString SCardMonitor::sendCommand_CardIssuer()
{
    //0x80, 0xb0, 0x00, 0xf6, 0x02, 0x00, 0x64
    QByteArray cmd_1("\x80\xb0\x00\xf6\x02\x00\x64", 7);
    QByteArray cmd_2("\x00\xc0\x00\x00\x64", 5);

    QByteArray cmd367_1("\x80\xb0\x00\xf6\x02\x00\x64", 7);
    QByteArray cmd367_2("\x00\xc0\x00\x01\x64", 5);

    QByteArray *pCmd_1 = NULL;
    QByteArray *pCmd_2 = NULL;

    if (cardAttribute[0] == 0x3B && cardAttribute[1] == 0x67) {
        pCmd_1 = &cmd367_1;
        pCmd_2 = &cmd367_2;
    } else {
        pCmd_1 = &cmd_1;
        pCmd_2 = &cmd_2;
    }

    QByteArray resp_1;
    transmit(*pCmd_1, resp_1);

    QByteArray resp_2;
    transmit(*pCmd_2, resp_2);

    QTextCodec *codec = QTextCodec::codecForName("TIS-620");
    QString str = codec->toUnicode(resp_2).trimmed();

    return str;
}

void SCardMonitor::read_nophoto(bool bPhoto)
{
    QStringList data;

    // pause detection
    stop();

    //int rv = ::SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hContext);

    // 1. connet
    scard_connect();

    // 2. select
    sendCommand_Select();

    Sleep(100);

    // 3. get citizen ID
    QString strCID = sendCommand_CitizenID();
    emit gotCitizenID(strCID);

    qDebug() << "citizen ID : " << strCID;

    // 4. personal info
    QString strInfo = sendCommand_PersonInfo();
    emit gotPersonalInfo(strInfo);

    qDebug() << "Personal Info : " << strInfo;

    // 5. address
    QString strAddress = sendCommand_Address();
    emit gotAddress(strAddress);

    qDebug() << "Address : " << strAddress;

    // 6. Card Expire
    QString strExpire = sendCommand_CardIssueExpire();
    emit gotCardIssueExpire(strExpire);

    qDebug() << "Issue/Expire : " << strExpire;

    // 7. Card Issuer
    QString strIssuer = sendCommand_CardIssuer();
    emit gotCardIssuer(strIssuer);

    qDebug() << "Issuer : " << strIssuer;

    // 8. photo
    if (bPhoto) {
        read_photo();
        emit gotPhoto();
    }

    // start monitor again
    monitor();
}

void SCardMonitor::read_photo()
{
    /*
    bool read_success = false;
    std::ofstream file;
    QString strfname = m_strAppDir + "/photo.jpg";
    qDebug() << "jpg : " << strfname;
    file.open(strfname.toStdString(), std::ios::out | std::ios::binary);
    if (!(file.is_open() && file.good())) {
        qDebug() << "Could not open file for write";
        read_success = false;
        return;
    }*/

    QByteArray cmd_1("\x80\xb0\x00\x00\x02\x00\xff", 7); // byte 2,3,6 will be replaced
    QByteArray cmd_2("\x00\xc0\x00\x00\x00", 5); // byte 4 will replaced

    QByteArray *pCmd_1 = &cmd_1;
    QByteArray *pCmd_2 = &cmd_2;

    QByteArray img;

    for (int i = 0; i <= 20; i++)
    {
        int xwd;
        BYTE xof = i + 1 ;
        if (i == 20)
            xwd = 38;
        else
            xwd = 254;

        BYTE sp2 = xof;
        BYTE sp3 = '\x7b' - (i*2);
        BYTE sp6 = xwd;
        BYTE spx = xwd;

        //qDebug() << hex << sp2 << " " << sp3 << " " << sp6 << " " << spx;

        BYTE *pData = (BYTE *)pCmd_1->data();
        pData[2] = sp2;
        pData[3] = sp3;
        if(i == 20)
            pData[6] = sp6;
        pData = (BYTE *)pCmd_2->data();
        pData[4] = spx;

        //qDebug() << "cmd_1 : " << pCmd_1->length();
        //qDebug() << hex << (BYTE)pCmd_1->at(0) << " " << (BYTE)pCmd_1->at(1) << " " << (BYTE)pCmd_1->at(2)
        //         << " " << (BYTE)pCmd_1->at(3) << " " << (BYTE)pCmd_1->at(4) << " " << (BYTE)pCmd_1->at(5)
        //         << " " << (BYTE)pCmd_1->at(6);

        //Sleep(25);

        QByteArray resp_1;
        transmit(*pCmd_1, resp_1);

        //Sleep(25);

        QByteArray resp_2;
        transmit(*pCmd_2, resp_2);

        //qDebug() << " i = " << i;

        img.append(resp_2.data(), resp_2.count() - 2);
        //qDebug() << "resp_2 : " << resp_2.size();
        //file.write(resp_2.data(), resp_2.size() - 2);
        //file.flush();
    }
//    file.close();
    qDebug() << "read photo is done";

    QFile imgFile(m_strAppDir + "/photo.jpg");

    if(imgFile.open(QIODevice::WriteOnly)){
        imgFile.write(img);
    }

    imgFile.close();

}
