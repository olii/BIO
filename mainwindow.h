#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QImage>
#include <fingerprint.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = 0);
    ~MainWindow();

private slots:
    void on_SegmentationSlider_valueChanged(int value);
    void on_SegmentationSpinBox_valueChanged(int arg1);
    void on_LoadImageButton_clicked();
    void on_ImageEnhanceButton_clicked();
    void RIPvalue(int value);

private:
    void Segmentation();

    Ui::MainWindow* ui;
    QImage src_img;

    bool loaded = false;
    fingerprint::BLOCK_VALIDITY blockValidity;

    cimg_library::CImg<float> orientation;
    cimg_library::CImg<float> cimgEnhanced;
};

#endif   // MAINWINDOW_H
