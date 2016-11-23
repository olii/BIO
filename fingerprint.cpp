#include "fingerprint.h"
#include <QDebug>

namespace fingerprint {

void border_correction(int& x, int& y, int w, int h)
{
    if (x >= w)
    {
        x = (w - 1) - (x - (w - 1));
    }
    else if (x < 0)
    {
        x = -x;
    }

    if (y < 0)
    {
        y = -y;
    }
    else if (y >= h)
    {
        y = (h - 1) - (y - (h - 1));
    }
}
template <typename T>
T& getPixel(CImg<T>& v, int x, int y)
{
    border_correction(x, y, v.width(), v.height());
    return v(x, y);
}

void ToBW(QImage& image)
{
    for (int ii = 0; ii < image.height(); ii++)
    {
        QRgb* scan = reinterpret_cast<QRgb*>(image.scanLine(ii));
        for (int jj = 0; jj < image.width(); jj++)
        {
            int gray = qGray(scan[jj]);
            scan[jj] = QColor(gray, gray, gray).rgba();
        }
    }
}

/**
 * converters
 */


QImage Convert(const CImg<float>& img)
{
    QImage qimg(img.width(), img.height(), QImage::Format_ARGB32);
    CImg<unsigned char> img_normalized = img.get_normalize(0,255);
    for (int line = 0; line < img.height(); line++)
    {
        QRgb* scanline = reinterpret_cast<QRgb*>(qimg.scanLine(line));
        for (int p = 0; p < img.width(); p++)
        {
            int c = (unsigned)(unsigned char)img_normalized(p, line);
            scanline[p] = qRgb(c, c, c);
        }
    }
    return qimg;
}

QImage Convert(const CImg<unsigned>& img)
{
    QImage qimg(img.width(), img.height(), QImage::Format_ARGB32);

    if (img.spectrum() == 4)
    {
        CImg<unsigned> tmp = img.get_permute_axes("cxyz");
        std::copy(tmp.data(), tmp.data() + tmp.size(), qimg.bits());
    }
    else if (img.spectrum() == 1)
    {
        for (int line = 0; line < img.height(); line++)
        {
            QRgb* scanline = reinterpret_cast<QRgb*>(qimg.scanLine(line));
            for (int p = 0; p < img.width(); p++)
            {
                int c = (unsigned)(unsigned char)img(p, line);
                scanline[p] = qRgb(c, c, c);
            }
        }
    }
    else
    {
        throw std::runtime_error("Unsuported conversion.");
    }
    return qimg;
}

CImg<unsigned char> Convert(const QImage& img)
{
    //http://stackoverflow.com/questions/21202635/cimg-library-creates-distorted-images-on-rotation
    CImg<unsigned char> src(img.bits(), 4, img.width(), img.height(), 1);
    src.permute_axes("yzcx");
    return src;
}

int Filter3x3(BLOCK_VALIDITY& data, int x, int y)
{
    int validN = 0;
    for (int j = y - 1; j <= y + 1; j++)
    {
        for (int i = x - 1; i <= x + 1; i++)
        {
            if (j == y && i == x)
                continue;   // skip self
            try
            {
                validN += data.at(j).at(i);
            }
            catch (...)
            {
            }
        }
    }
    return validN;
}

CImg<unsigned char> Normalize(CImg<unsigned char>& img, BLOCK_VALIDITY& data)
{
    double mean = 0.0;
    int validCount = 0;

    int blX = img.width();
    int blY = img.height();
    for (int i = 0; i < blY; i += BLK_SIZE)
    {
        for (int j = 0; j < blX; j += BLK_SIZE)
        {
            if (data[i / BLK_SIZE][j / BLK_SIZE])
            {
                mean += img.get_crop(j, i, j + BLK_SIZE - 1, i + BLK_SIZE - 1).mean();
                validCount++;
            }
        }
    }
    mean /= validCount;

    double variance = 0.0;
    for (int i = 0; i < blY; i += BLK_SIZE)
    {
        for (int j = 0; j < blX; j += BLK_SIZE)
        {
            if (data[i / BLK_SIZE][j / BLK_SIZE])
            {
                auto image = img.get_crop(j, i, j + BLK_SIZE - 1, i + BLK_SIZE - 1);
                cimg_for(image, ptr, unsigned char) { variance += ((*ptr - mean) * (*ptr - mean)); }
            }
        }
    }
    variance /= (validCount * BLK_SIZE * BLK_SIZE);

    auto copy = img;

    for (int i = 0; i < img.height(); i++)
    {
        for (int j = 0; j < img.width(); j++)
        {
            float black = img(j, i);

            double tmp = sqrt(NORM_V0 * ((black - mean) * (black - mean)) / variance);

            int result = 0;
            if (black > mean)
            {
                result = NORM_M0 + tmp;
            }
            else
            {
                result = NORM_M0 - tmp;
            }
            // saturate
            if (result < 0)
                result = 0;
            if (result > 255)
                result = 255;

            copy(j, i) = result;
        }
    }
    return copy;
}

CImg<float> Orientation(CImg<unsigned char>& img, BLOCK_VALIDITY& data)
{
    const int W = img.width();
    const int H = img.height();

    CImg<float> GradX = img.get_gradient("x", 2)[0];
    CImg<float> GradY = img.get_gradient("y", 2)[0];

    // Compute  Gxy, Gxx and Gyy as in Equation (3) in [MMJ+09]
    // on page 104.
    CImg<float> Phi(W, H, 1, 1);

    for (int i = 0; i < H; i++)
    {
        for (int j = 0; j < W; j++)
        {
            if (!data[i / BLK_SIZE][j / BLK_SIZE])
            {
                continue;
            }
            // this pixel belongs to a valid block
            const int blkSize = 11;
            const int Half = blkSize / 2;

            float Gxy = 0;
            float Gxx = 0;
            float Gyy = 0;
            for (int ii = i - Half; ii <= i + Half; ii++)
                for (int jj = j - Half; jj <= j + Half; jj++)
                {
                    double dx, dy;
                    dx = getPixel(GradX, jj, ii);
                    dy = getPixel(GradY, jj, ii);
                    Gxy += dx * dy;
                    Gxx += dx * dx;
                    Gyy += dy * dy;
                }

            double theta = 0.5 * (M_PI + atan2(Gxy * 2, Gxx - Gyy));
            Phi(j, i) = theta;
        }
    }
    return Phi;
}


double gabor_2d(int x, int y, float theta, float f)
{
    constexpr float Gx = 4.0;
    constexpr float Gy = Gx;
    //constexpr float offset = 10.0;
    theta -= M_PI_2;

    float X = x * cos(theta) + y * sin(theta);
    float Y = -x * sin(theta) + y * cos(theta);

    // working -> revesed angle
    /*float X = x * sin(theta) + y * cos(theta);
    float Y = -x * cos(theta) + y * sin(theta);*/

    float e = exp(-(X * X + Y * Y) / (2 * Gx * Gx));
    return e * cos(2 * M_PI * X * f);
}

CImg<float> GetGaborKernel(int size, float phi, float pixDistance)
{
    CImg<float> kernel(size, size);

    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            kernel(j, i) = gabor_2d(j - size / 2, i - size / 2, phi, 1.0 / pixDistance);
        }
    }
    return kernel;
}

float FrequencyEstimate(CImg<float> img)
{
    auto L = img.get_normalize(-1, 1) /*.blur(1.2)*/;
    //L.display_graph();
    auto FFT = L.get_FFT('y');
    auto modul = FFT[0].abs();   //(FFT[0].pow(2) + FFT[1].pow(2)).sqrt();
    //modul.display_graph();

    float denom = 0;
    float nom = 0;
    for (int i = 1; i <= 7; i++)
    {
        nom += i * modul(0, i);
        denom += modul(0, i);
    }
    return (nom / denom);
}


CImg<float> GaborTransform(CImg<float>& normalized,CImg<unsigned char>& normalizedChar, CImg<float>& Phi,BLOCK_VALIDITY& validity)
{
    CImg<float> ret(normalized.width(), normalized.height());

    for (int yy = 0; yy < ret.height(); yy++)
        for (int xx = 0; xx < ret.width(); xx++)
        {

            if (!validity[yy / BLK_SIZE][xx / BLK_SIZE])
            {
                ret(xx,yy) = 0;
                continue;
            }

            float Dx = cos(Phi(xx, yy) + M_PI_2);
            float Dy = sin(Phi(xx, yy) + M_PI_2);

            CImg<float> Window(16, 32);
            for (int y = 0; y < 16; y++)
            {
                for (int x = 0; x < 32; x++)
                {
                    int u = xx + (x - 32 / 2) * Dx + (y - 16 / 2) * Dy;
                    int v = yy + (x - 32 / 2) * Dy + (16 / 2 - y) * Dx;
                    Window(15 - y, 31 - x) = getPixel(normalizedChar,u, v);
                }
            }

            CImg<float> w(1, 32);
            for (int x = 0; x < 32; x++)
            {
                float avg = 0.0;
                for (int y = 0; y < 16; y++)
                {
                    avg += Window(y, x);
                }
                w(0, x) = (avg / 16.0);
            }

            float freq = FrequencyEstimate(w);
            float pixDist = 32 / freq;
            if (pixDist < 5) pixDist = 5;
            if (pixDist > 18) pixDist = 18;

            if (std::isnan(pixDist))
            {
                ret(xx,yy) = 0;
                continue;
            }


            auto kernel = GetGaborKernel(18, Phi(xx, yy), pixDist);
            auto subimage = normalized.get_crop(xx - 8, yy - 8, xx + 9, yy + 9);
            auto convoluted = subimage.get_mul(kernel);

            float t = convoluted.sum();
            if (t <= 0.0) t = -1;
            if (t > 0.0) t  = 1.0;

            ret(xx,yy) = t;
        }
    return ret;
}


}   // namespace fingerprint
