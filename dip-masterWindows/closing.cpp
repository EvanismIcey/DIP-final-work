#include "closing.h"

//闭运算：先膨胀，后腐蚀
QImage calculateclosing(QImage &image)
{
    // 获取图像
    int width = image.width();
    int height = image.height();

    QImage newImage = image;
    QImage newImage1 = newImage;
    //定义卷积核
    int kernel[7][7] = {
                { 1,1,1,1,1,1,1 },
                { 1,1,1,1,1,1,1 },
                { 1,1,1,1,1,1,1 },
                { 1,1,1,1,1,1,1 },
                { 1,1,1,1,1,1,1 },
                { 1,1,1,1,1,1,1 },
                { 1,1,1,1,1,1,1 }  };
    int sizeKernel = 7;
    QColor color;
    QColor Rcolor;

    //膨胀操作
    for (int y = sizeKernel / 2; y < height- sizeKernel / 2; y++)
    {
        for (int x = sizeKernel / 2; x < width- sizeKernel / 2; x++)
        {
            int kr = 0;
            int kg = 0;
            int kb = 0;
            Rcolor = QColor(image.pixel(x, y));
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
                        newImage.setPixel(x, y, qRgb(kr, kg, kb));
                    }
                }

    //用膨胀后的新图像继续进行腐蚀操作
    for (int y = sizeKernel / 2; y <height- sizeKernel / 2; y++)
    {
        for (int x = sizeKernel / 2; x < width- sizeKernel / 2; x++)
        {
            int kr = 255;
            int kg = 255;
            int kb = 255;
            Rcolor = QColor(newImage.pixel(x, y));
                        for (int j = -sizeKernel / 2; j <= sizeKernel / 2; j++)
                        {
                            for (int i = -sizeKernel / 2; i <= sizeKernel / 2; i++)
                            {
                                color = QColor(newImage.pixel(x + i, y + j));
                                while (color.red() < kr && kernel[sizeKernel / 2 + i][sizeKernel / 2 + j])
                                {
                                    kr = color.red();
                                }
                                while (color.green() < kg && kernel[sizeKernel / 2 + i][sizeKernel / 2 + j])
                                {
                                    kg = color.green();
                                }
                                while (color.blue() < kb && kernel[sizeKernel / 2 + i][sizeKernel / 2 + j])
                                {
                                    kb = color.blue();
                                }
                            }
                        }
                        //获得闭运算后的图像
                        newImage1.setPixel(x, y, qRgb(kr, kg, kb));
        }
    }
    return newImage1;
}

