#include <QGuiApplication>
#include <QQuickView>
#include <QQmlContext>
#include <QDir>

#include "../../bangkokdev/bkkdevscard/bkkdevscard.h"
#include "qfilesearch.h"

int main(int argc, char *argv[])
{
#if defined(Q_OS_WIN)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QGuiApplication app(argc, argv);


    QQuickView view;
    QQmlContext *pContext = view.rootContext();

    QString strPath = QGuiApplication::applicationDirPath();

    BkkdevScard scard(strPath);

    pContext->setContextProperty("applicationDirPath", strPath);
    pContext->setContextProperty("scard", &scard);

    //
    QFileSearch provinces("../code/province.csv");
    QFileSearch amphurs("../code/amphur.csv");
    QFileSearch districts("../code/district.csv");
    //
    pContext->setContextProperty("applicationDirPath", strPath);
    pContext->setContextProperty("scard", &scard);
    pContext->setContextProperty("provincesCSV", &provinces);
    pContext->setContextProperty("amphursCSV", &amphurs);
    pContext->setContextProperty("districtsCSV", &districts);

    QString strCurrPath = QDir::currentPath();
    qDebug() << "Current Path : " + strCurrPath;
    view.setSource(QUrl::fromLocalFile(strCurrPath + "/main.qml"));

    QQuickItem *item = view.rootObject();

    QObject::connect((QObject *)item, SIGNAL(scardStartMonitor()), &scard, SIGNAL(start_monitor()));
    QObject::connect((QObject *)item, SIGNAL(scardStopMonitor()), &scard, SIGNAL(stop_monitor()));
    QObject::connect((QObject *)item, SIGNAL(get_data(bool)), &scard, SIGNAL(scard_read_data(bool)));

    view.connect((QObject *)(view.engine()), SIGNAL(quit()), &app, SLOT(quit()));
    view.setResizeMode(QQuickView::SizeRootObjectToView);

    QMetaObject::invokeMethod((QObject *)item, "startMonitor", Qt::QueuedConnection);

    // show full screen
    //view.showFullScreen();

    view.show();

    return app.exec();
}
