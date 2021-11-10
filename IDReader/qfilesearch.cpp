#include "qfilesearch.h"
#include <QFile>
#include <QTextStream>
#include <QTextCodec>

#include <QDebug>

QFileSearch::QFileSearch(QString fname, QObject *parent) : QObject(parent)
{
    m_filepath = fname;
}

int QFileSearch::search(int ret_idx, int from_idx, QString val, int cmp_idx, QString op)
{
    int ret = -1;
    bool bOk;

    if( m_filepath.isEmpty() )
    {
        qDebug() << "file path is empty";
        return -1;
    }

    QFile file(m_filepath);
    if ( file.open(QIODevice::ReadOnly) ) {
        QString line;
        QTextStream t( &file );

        // skip header
        t.readLine();

        do
        {
            line = t.readLine();

            QStringList list = line.split(',');

            if( list.size() > from_idx )
            {
                QString str1 = list.at(from_idx).trimmed();
                //QTextCodec::ConverterState state;
                //QTextCodec *codec = QTextCodec::codecForName("UTF-8");
                //const QString text = codec->toUnicode(str1.toLatin1(), str1.toLatin1().size(),  &state);
                //if (state.invalidChars > 0) {
                //    qDebug() << "Not a valid UTF-8 sequence.";
                //}
                //qDebug() << str;
                if(str1.toLocal8Bit() == val.trimmed()) {
                    ret = list.at(ret_idx).toInt(&bOk);
                    // check with optional (province) code
                    if(cmp_idx > -1 && !op.isEmpty()) {
                        QString str2 = list.at(cmp_idx).trimmed().toLocal8Bit();
                        str2.truncate(2);
                        if (op == str2) {
                            break;
                        }
                    } else
                        break;
                }
            }
        } while (!line.isNull());

        file.close();
    } else {
        qDebug() << "error open file";
        return -1;
    }

    return ret;
}
