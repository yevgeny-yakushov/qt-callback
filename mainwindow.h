#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_mainwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindowClass; };
QT_END_NAMESPACE

class CQueryImpl;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void execute();

private:
    Ui::MainWindowClass *ui;
    CQueryImpl          *m_pQueryImpl;

    int val1 = 123;
    int val2 = 456;
};
