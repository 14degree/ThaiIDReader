#ifndef QFILESEARCH_H
#define QFILESEARCH_H

#include <QObject>

class QFileSearch : public QObject
{
    Q_OBJECT
public:
    explicit QFileSearch(QString fname, QObject *parent = nullptr);

signals:

public slots:
    int search(int ret_idx, int from_idx, QString val, int cmp_idx = -1, QString op = QString());

private:
    QString m_filepath;
};

#endif // QFILESEARCH_H
