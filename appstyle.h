#ifndef APPSTYLE_H
#define APPSTYLE_H

#include <QApplication>

class AppStyle {
public:
    static void apply(QApplication &app);
    static QString styleSheet();
};

#endif // APPSTYLE_H
