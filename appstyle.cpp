#include "appstyle.h"

void AppStyle::apply(QApplication &app) {
    app.setStyleSheet(styleSheet());
}

QString AppStyle::styleSheet() {
    return R"(
        /* === Общий фон === */
        QWidget {
            background-color: #fdfaf6;
            color: #3b2f2f;
            font-family: "Comic Sans MS";
            font-size: 12pt;
        }

        /* === Кнопка File === */
        QPushButton#fileButton {
            background: #d9b08c;
            border-radius: 12px;
            padding: 12px 20px;
            font-size: 16pt;
            font-weight: bold;
            color: #3b2f2f;
        }
        QPushButton#fileButton:hover {
            background: #c08552;
        }
        QPushButton#fileButton:pressed {
            background: #a9714b;
            color: white;
        }

        /* === Кнопка Help === */
        QPushButton#helpButton {
            background: #e7d4b5;
            border-radius: 12px;
            padding: 10px 18px;
            font-size: 13pt;
            font-weight: bold;
            color: #3b2f2f;
        }
        QPushButton#helpButton:hover {
            background: #d9b08c;
            color: white;
        }
        QPushButton#helpButton:pressed {
            background: #c08552;
            color: white;
        }

        /* === Таблица: заголовки === */
QHeaderView::section {
    background-color: #f3e9dd;
    font-size: 15pt;
    font-weight: bold;
    padding: 8px;
    border-top: 1px solid #d9b08c;
    border-bottom: 2px solid #d9b08c;
    border-right: 1px solid #d9b08c;
    border-left: none;
}
QHeaderView::section:first {
    border-left: 1px solid #d9b08c;
}

        /* === Таблица: строки === */
        QTableView {
            gridline-color: #d9b08c;
            selection-background-color: #d9b08c;
            selection-color: white;
            font-size: 12pt;
        }

        /* === Статусбар === */
        QStatusBar {
            background: #f3e9dd;
            font-size: 13pt;
        }

/* === Всплывающее меню (File) === */
QMenu {
    background-color: #fdfaf6;
    border: 2px solid #d9b08c;
    padding: 6px;
    font-size: 15pt;
}

QMenu::item {
    background-color: transparent;
    padding: 8px 20px;
    color: #3b2f2f;
}

QMenu::item:selected {
    background-color: #d9b08c;
    color: #3b2f2f;
    border-radius: 6px;
}

QMenu::separator {
    height: 1px;
    background: #d9b08c;
    margin: 4px 10px;
}

/* === Всплывающие подсказки QToolTip === */
QToolTip {
    background-color: #f3e9dd;
    color: #3b2f2f;
    border: 2px solid #d9b08c;
    padding: 6px 10px;
    border-radius: 6px;
    font-family: "Comic Sans MS";
    font-weight: bold;
    font-size: 12pt;
}
    )";


}
