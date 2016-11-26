#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QImage>
#include "fingerprint.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->graphicsView, &CustomGraphicsView::RIP, this, &MainWindow::RIPvalue);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_SegmentationSlider_valueChanged(int value)
{
    ui->SegmentationSpinBox->setValue(value);
    Segmentation();
}

void MainWindow::on_SegmentationSpinBox_valueChanged(int arg1)
{
    ui->SegmentationSlider->setValue(arg1);
    Segmentation();
}

void MainWindow::on_LoadImageButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open Image File");
    if (fileName.isEmpty())
        return;

    src_img = QImage(fileName);
    src_img = src_img.convertToFormat(QImage::Format_ARGB32);
    fingerprint::ToBW(src_img);
    // create a multiply of 16x16
    int x = (src_img.width() / fingerprint::BLK_SIZE) * fingerprint::BLK_SIZE;
    int y = (src_img.height() / fingerprint::BLK_SIZE) * fingerprint::BLK_SIZE;
    src_img = src_img.copy(QRect(0, 0, x, y));

    ui->graphicsView->loadImage(src_img);
    loaded = true;
    ui->graphicsView->setEnableLine(false);
    Segmentation();
}

void MainWindow::Segmentation()
{
    if (!loaded)
        return;

    ui->graphicsView->loadImage(src_img);
    ui->graphicsView->setEnableLine(false);

    auto cimg = fingerprint::Convert(src_img).channel(0);
    // split to 16x16 blocks, compute variance
    blockValidity.clear();

    int blX = src_img.width() / fingerprint::BLK_SIZE;
    int blY = src_img.height() / fingerprint::BLK_SIZE;

    for (int i = 0; i < blY; i++)
    {
        blockValidity.push_back(std::vector<bool>());
        for (int j = 0; j < blX; j++)
        {
            auto subimage = cimg.get_crop(j * fingerprint::BLK_SIZE,
                i * fingerprint::BLK_SIZE,
                j * fingerprint::BLK_SIZE + fingerprint::BLK_SIZE - 1,
                i * fingerprint::BLK_SIZE + fingerprint::BLK_SIZE - 1);
            //subimage.display();
            bool result = subimage.variance(1) >= ui->SegmentationSlider->value();
            blockValidity.back().push_back(result);
        }
    }

    // 3x3 filter
    for (int i = 0; i < blY; i++)
    {
        for (int j = 0; j < blX; j++)
        {
            int n = fingerprint::Filter3x3(blockValidity, j, i);
            if (blockValidity[i][j] == false && n >= 8)
            {
                // this block is valid
                blockValidity[i][j] = true;
            }
            else if (blockValidity[i][j] == true && n <= 1)
            {
                // marked as valid, but only 1 block in neughbourhood valid
                blockValidity[i][j] = false;
            }
        }
    }

    for (int i = 0; i < blY; i++)
    {
        for (int j = 0; j < blX; j++)
        {
            ui->graphicsView->drawValidityRect(j * fingerprint::BLK_SIZE,
                i * fingerprint::BLK_SIZE,
                fingerprint::BLK_SIZE,
                fingerprint::BLK_SIZE,
                blockValidity[i][j]);
        }
    }
}

void MainWindow::on_ImageEnhanceButton_clicked()
{
    // make BW image with 2 dimensions
    auto cimg = fingerprint::Convert(src_img).channel(0);
    // normalize
    cimg = fingerprint::Normalize(cimg, blockValidity);
    // gradient + orientation field
    orientation = fingerprint::Orientation(cimg, blockValidity);
    // gabor filtering + binarization
    fingerprint::CImg<float> X = cimg.get_normalize(-1.0, 1.0);
    auto X2 = cimg;
    cimgEnhanced = fingerprint::GaborTransform(X, X2, orientation, blockValidity);
    // display
    QImage enhanced = fingerprint::Convert(cimgEnhanced);
    ui->graphicsView->loadImage(enhanced);
    // enable draw-line
    ui->graphicsView->setEnableLine(true);

    // find maximal papilary count
    int max = -1;
    int pos = 0;
    for(int i = 0; i < cimg.width(); i++)
    {
        int count= ui->graphicsView->LineSampler(i, 0, i, cimg.height()-1 );
        if (count > max)
        {
            max = count;
            pos = i;
        }
    }
    ui->maximalX->setText(QString::number(pos));
    ui->maximalXcount->setText(QString::number(max));
    ui->graphicsView->DrawMaxLine(pos, 0, pos, cimg.height()-1);

}

void MainWindow::RIPvalue(int value)
{
    ui->RIPCountLabel->setText(QString::number(value));
}
