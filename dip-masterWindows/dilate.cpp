#include "dilate.h"

QImage calculatedilate(QImage &image)
{
    // 获取图像
    int width = image.width();
    int height = image.height();

    QImage newImage = image;

    //定义膨胀过程中的结构元素模板（卷积核）
    int kernel[7][7] = {
        { 0,0,0,1,0,0,0 },
        { 0,1,1,1,1,1,0 },
        { 0,1,1,1,1,1,0 },
        { 1,1,1,1,1,1,1 },
        { 0,1,1,1,1,1,0 },
        { 0,1,1,1,1,1,0 },
        { 0,0,0,1,0,0,0 } };
    int sizeKernel = 7;
    QColor color;
    QColor Rcolor;

    //卷积核的中心点逐个像素扫描原始图像
    for (int y = sizeKernel / 2; y <height- sizeKernel / 2; y++)
    {
        for (int x = sizeKernel / 2; x < width- sizeKernel / 2; x++)
        {
            int kr = 0;
            int kg = 0;
            int kb = 0;
            Rcolor = QColor(image.pixel(x, y));
            //寻找卷积核覆盖区域的像素点最大值，并用最大值替代原像素值
                        for (int j = -sizeKernel / 2; j <= sizeKernel / 2; j++)
                        {
                            for (int i = -sizeKernel / 2; i <= sizeKernel / 2; i++)
                            {
                                color = QColor(image.pixel(x + i, y + j));
                                while (color.red() > kr && kernel[sizeKernel / 2 + i][sizeKernel / 2 + j])
                                {
                                    kr = color.red();
                                }
                                while (color.green() > kg && kernel[sizeKernel / 2 + i][sizeKernel / 2 + j])
                                {
                                    kg = color.green();
                                }
                                while (color.blue() > kb && kernel[sizeKernel / 2 + i][sizeKernel / 2 + j])
                                {
                                    kb = color.blue();
                                }
                            }
                        }
                        //获取更新像素值后的图像，即获取膨胀后的图像
                        newImage.setPixel(x, y, qRgb(kr, kg, kb));
                    }
                }
    //返回膨胀后图像
                return newImage;
            }
