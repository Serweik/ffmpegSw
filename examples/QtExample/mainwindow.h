#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "camera.h"

#include <QMainWindow>
#include <QGridLayout>

namespace Ui {
	class MainWindow;
}

class MainWindow: public QMainWindow {
	Q_OBJECT

	public:
		explicit MainWindow(QWidget* parent = nullptr);
		~MainWindow();

	private slots:
		void on_addCamera_clicked();

	private:
		Ui::MainWindow* ui;
		QGridLayout* cameraLayout = nullptr;
		QSharedPointer<AVffmpegWrapper> aVffmpegWrapper;
};

#endif // MAINWINDOW_H
