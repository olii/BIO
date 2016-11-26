#ifndef ALGORITHM_H
#define ALGORITHM_H

#include "external/CImg.h"
#include <vector>
#include <QImage>
#include <QDebug>

namespace fingerprint {
using namespace cimg_library;

constexpr int BLK_SIZE = 16;
constexpr int NORM_M0 = 100;
constexpr int NORM_V0 = 100;

typedef std::vector<std::vector<bool>> BLOCK_VALIDITY;

void ToBW(QImage& image);
QImage Convert(const CImg<unsigned char>& img);
QImage Convert(const CImg<float>& img);
CImg<unsigned char> Convert(const QImage& img);

int Filter3x3(BLOCK_VALIDITY& data, int x, int y);

CImg<unsigned char> Normalize(CImg<unsigned char>& img, BLOCK_VALIDITY& data);
CImg<float> Orientation(CImg<unsigned char>& img, BLOCK_VALIDITY& data);


CImg<float> GetGaborKernel(int size, float phi, float pixDistance);
float FrequencyEstimate(CImg<float> img);

CImg<float> GaborTransform(CImg<float>& normalized,CImg<unsigned char>& normalizedChar, CImg<float>& Phi,BLOCK_VALIDITY& validity);

}

#endif   //
