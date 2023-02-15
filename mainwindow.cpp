#include "mainwindow.h"
#include "queryImpl.h"

#include <QPushButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindowClass())
{
    ui->setupUi(this);

    m_pQueryImpl = new CQueryImpl();

    connect(ui->m_btnExecute, &QPushButton::clicked, this, &MainWindow::execute);
}

MainWindow::~MainWindow()
{
    delete m_pQueryImpl; m_pQueryImpl = nullptr;
    delete ui;
}

void MainWindow::execute()
{
    //m_pQueryImpl->startTest(val1, val2);
    m_pQueryImpl->startTest(123, 456);
}