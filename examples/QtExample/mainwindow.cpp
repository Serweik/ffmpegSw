#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent):
	QMainWindow(parent), ui(new Ui::MainWindow) {
	ui->setupUi(this);
	cameraLayout = new QGridLayout();
	cameraLayout->setObjectName("cameraLayout");
	ui->widget->setLayout(cameraLayout);
}

MainWindow::~MainWindow() {
	delete ui;
}

void MainWindow::on_addCamera_clicked() {
	if(!ui->pathToSource->text().isEmpty()) {
		Camera* camera = new Camera();
		camera->setPath(ui->pathToSource->text());
		if(camera->start()) {
			int row = cameraLayout->rowCount();
			int column = cameraLayout->columnCount();
			int items = cameraLayout->count();

			if(column < 4) {
				row = 0;
				if(items < 1) {
					column -= 1;
				}
			}else {
				int emptyCeilInLastRow = (row * column) - items;
				if(emptyCeilInLastRow == 0) {
					if(column > row) {
						column = 0;
					}else if(column <= row) {
						row = 0;
					}
				}else {
					column -= emptyCeilInLastRow;
					row -= 1;
				}
			}
			cameraLayout->addWidget(camera, row, column);
		}else {
			delete camera;
		}
	}
}
