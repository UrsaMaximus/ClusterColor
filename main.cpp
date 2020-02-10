#include "clustercolormain.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);
    a.setApplicationName(QString("ClusterColor"));
    a.setOrganizationName(QString("GreatestBear"));
    a.setOrganizationDomain(QString("greatestbear.com"));

    ClusterColorMain w;
    w.show();
    return a.exec();
}
