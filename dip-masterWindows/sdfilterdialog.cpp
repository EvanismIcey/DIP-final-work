#include "sdfilterdialog.h"

//空域滤波
//定义8种不同滤波算子

//robert算子，实际为2*2模板，对角线作差
float robertsX[9] = {
     0, 0, 0,
     0,-1, 0,
     0, 0, 1};
float robertsY[9] = {
     0, 0, 0,
     0, 0,-1,
     0, 1, 0};

//sobel算子一阶导，着重考虑中心点，因此中心点权重增加
float sobelX[9] = {
    -1,-2,-1,
     0, 0, 0,
     1, 2, 1};
float sobelY[9] = {
    -1,0,1,
    -2,0,2,
    -1,0,1};

//prewitt算子以f(x+1)-f(x-1)近似一阶差分
float prewittX[9] = {
    -1,-1,-1,
     0, 0, 0,
     1, 1, 1};
float prewittY[9] = {
    -1,0,1,
    -1,0,1,
    -1,0,1};

//laplacian算子为二阶导算子
//二维情况：f'(x,y)=-4f(x,y)+f(x-1,y)+f(x+1,y)+f(x,y-1)+f(x,y+1);提取系数得到模板
float laplacian4[9] = {
    0, 1,0,
    1,-4,1,
    0, 1,0};

//考虑两个斜对角的模板
float laplacian8[9] = {
    1, 1,1,
    1,-8,1,
    1, 1,1};

//定义log算子
float logElement(int x, int y, float sigma)
{
    float g = 0;
    for(float ySubPixel = y - 0.5f; ySubPixel <= y + 0.5f; ySubPixel += 0.1f) {
        for(float xSubPixel = x - 0.5f; xSubPixel <= x + 0.5f; xSubPixel += 0.1f) {
            float s = ((xSubPixel*xSubPixel)+(ySubPixel*ySubPixel)) / (2*sigma*sigma);
            g += 2 * (s-1/(sigma*sigma)) * expf(-s);
        }
    }
    g /= 121;

    return g;
}

float *generateLOGKernel(float sigma, int &kernelSize)
{
    kernelSize = (int)(4*sigma+1 + 0.5f) /2 * 2 + 1;
    float *LOGKernel = new float[kernelSize*kernelSize];
    double sum = 0;
    for(int j=0; j<kernelSize; ++j){
        for(int i=0; i<kernelSize; ++i){
            int x = (-kernelSize/2)+i;
            int y = (-kernelSize/2)+j;
            LOGKernel[j*kernelSize+i] = logElement(x, y, sigma);
            sum += LOGKernel[j*kernelSize+i];
        }
    }
    // subtract mean to get zero sum
    double mean = sum / (kernelSize * kernelSize);
    for(int i=0; i<kernelSize*kernelSize; ++i){
        LOGKernel[i] -= mean;
    }

    return LOGKernel;
}

//滤波函数
static void filterProc(const uchar *src, int w, int h,
            const float *kernel, int hkw, int hkh, uchar *dst)
{
    int i, j, m, n;
    float val_r, val_g, val_b, r, g, b;
    int nw = w-2*hkw;
    int nh = h-2*hkh;
    int kw = 2*hkw+1;
    int kh = 2*hkh+1;

    memset(dst, 0, nw*nh*3);

    //模板滑过各像素点，与覆盖区域卷积，更新像素值
    for (j=hkh; j<h-hkh; j++)
    {
        for (i=hkw; i<w-hkw; i++)
        {
            val_r = val_g  = val_b = 0;
            for (n=-hkh; n<=hkh; n++)
            {
                for (m=-hkw; m<=hkw; m++)
                {
                    r = src[(j-n)*w*3+(i-m)*3];
                    g = src[(j-n)*w*3+(i-m)*3+1];
                    b = src[(j-n)*w*3+(i-m)*3+2];
                    val_r +=  r * kernel[(n+hkh)*kw+m+hkw];
                    val_g +=  g * kernel[(n+hkh)*kw+m+hkw];
                    val_b +=  b * kernel[(n+hkh)*kw+m+hkw];
                }
            }
            //限定更新后RGB像素值范围在0-255
            val_r = val_r>255 ? 255 : val_r;
            val_g = val_g>255 ? 255 : val_g;
            val_b = val_b>255 ? 255 : val_b;

            val_r = val_r<0 ? 0 : val_r;
            val_g = val_g<0 ? 0 : val_g;
            val_b = val_b<0 ? 0 : val_b;

            //更新图像像素值
            dst[(j-hkh)*nw*3+(i-hkw)*3] = (uchar)(val_r);
            dst[(j-hkh)*nw*3+(i-hkw)*3+1] = (uchar)(val_g);
            dst[(j-hkh)*nw*3+(i-hkw)*3+2] = (uchar)(val_b);
        }
    }
}

//定义滤波函数的参数并获取滤波后图像
SDFilterDialog::SDFilterDialog(QImage inputImage)
{
    srcImage = inputImage;

    //获取原图尺寸
    int w = srcImage.width();
    int h = srcImage.height();
    int pixel_num = w*h;

    rgb = new uchar[3*pixel_num];
    borderType = 0;
    halfKernelSize = 1;
    sigma = 2;
    maxSigma = 7;

    iniUI();

    // obtain image channels
    splitImageChannel(srcImage, rgb);

    int count = borderTypeComboBox->count();
    borderTypeComboBox->setCurrentIndex(count-1);

    // padding
    int hkw = 1;
    int hkh = 1;
    int nw = w+hkw*2;
    int nh = h+hkh*2;
    rgbPadded = new uchar[3*nw*nh];
    paddedImage = QImage(nw, nh, QImage::Format_RGB888);
    uchar constBorder[3] = {0};

    //调用copyMakeBorder填充边缘（固定值填充）
    copyMakeBorder(rgb, w, h, 3,
                   halfKernelSize, halfKernelSize, halfKernelSize, halfKernelSize, (BorderType)borderType,
                   constBorder, rgbPadded);

    robertsImage = imageFilter((int)FilterType::Roberts);
    sobelImage = imageFilter((int)FilterType::Sobel);
    prewittImage = imageFilter((int)FilterType::Prewitt);
    laplacian4Image = imageFilter((int)FilterType::Laplacian4);
    laplacian8Image = imageFilter((int)FilterType::Laplacian8);
    LOGImage = imageLOGFilter(sigma);

    //调用setPixmap函数显示图像
    srcImageLabel->setPixmap(QPixmap::fromImage(srcImage).scaled(srcImageLabel->width(), srcImageLabel->height()));
    robertsImageLabel->setPixmap(QPixmap::fromImage(robertsImage).scaled(robertsImageLabel->width(), robertsImageLabel->height()));
    sobelImageLabel->setPixmap(QPixmap::fromImage(sobelImage).scaled(sobelImageLabel->width(), sobelImageLabel->height()));
    prewittImageLabel->setPixmap(QPixmap::fromImage(prewittImage).scaled(prewittImageLabel->width(), prewittImageLabel->height()));
    laplacian4ImageLabel->setPixmap(QPixmap::fromImage(laplacian4Image).scaled(laplacian4ImageLabel->width(), laplacian4ImageLabel->height()));
    laplacian8ImageLabel->setPixmap(QPixmap::fromImage(laplacian8Image).scaled(laplacian8ImageLabel->width(), laplacian8ImageLabel->height()));
    LOGImageLabel->setPixmap(QPixmap::fromImage(LOGImage).scaled(LOGImageLabel->width(), LOGImageLabel->height()));

    delete [] rgbPadded;
    rgbPadded = nullptr;

}

//log算子特殊，单独拿出来设计滤波函数
QImage SDFilterDialog::imageLOGFilter(float sigma)
{
    int w = srcImage.width();
    int h = srcImage.height();

    float *filterKernel;
    int kernelSize = 0;
    filterKernel = generateLOGKernel(sigma, kernelSize);

    halfKernelSize = kernelSize/2;
    int nw = w+halfKernelSize*2;
    int nh = h+halfKernelSize*2;
    rgbFilteredX = new uchar[3*w*h];
    rgbPadded = new uchar[3*nw*nh];
    paddedImage = QImage(nw, nh, QImage::Format_RGB888);

    // padding
    uchar constBorder[3] = {0};
    copyMakeBorder(rgb, w, h, 3,
                   halfKernelSize, halfKernelSize, halfKernelSize, halfKernelSize, (BorderType)borderType,
                   constBorder, rgbPadded);
    // filtering
    filterProc(rgbPadded, nw, nh, filterKernel,
               halfKernelSize, halfKernelSize, rgbFilteredX);

    QImage dst;
    concatenateImageChannel(rgbFilteredX, w, h, dst);

    delete [] rgbFilteredX;
    rgbFilteredX = nullptr;

    delete [] filterKernel;
    filterKernel = nullptr;

    return dst;
}

//滤波函数，判断滤波算子类型后进行滤波，返回滤波后图像
QImage SDFilterDialog::imageFilter(int inputFilterType)
{
    int w = srcImage.width();
    int h = srcImage.height();
    int hkw = 1;
    int hkh = 1;

    float *filterKernelX;// = new float[(2*hkw+1)*(2*hkh+1)];
    float *filterKernelY;// = new float[(2*hkw+1)*(2*hkh+1)];

    switch (inputFilterType) {
    case FilterType::Roberts:
        filterKernelX = robertsX;
        filterKernelY = robertsY;
        break;
    case FilterType::Sobel:
        filterKernelX = sobelX;
        filterKernelY = sobelY;
        break;
    case FilterType::Prewitt:
        filterKernelX = prewittX;
        filterKernelY = prewittY;
        break;
    case FilterType::Laplacian4:
        filterKernelX = laplacian4;
        break;
    case FilterType::Laplacian8:
        filterKernelX = laplacian8;
        break;
    }

    int nw = w+hkw*2;
    int nh = h+hkh*2;
    rgbFilteredX = new uchar[3*w*h];
    rgbFilteredY = new uchar[3*w*h];

    // 调用滤波函数进行图像滤波
    filterProc(rgbPadded, nw, nh, filterKernelX, hkw, hkh, rgbFilteredX);
    if ((inputFilterType != (int)FilterType::Laplacian4) &&
        (inputFilterType != (int)FilterType::Laplacian8) ) {
        filterProc(rgbPadded, nw, nh, filterKernelY, hkw, hkh, rgbFilteredY);
        for (int i=0; i<w*h*3; i++) {
           rgbFilteredX[i] = (rgbFilteredX[i]+rgbFilteredY[i])/2;
        }
    }

    QImage dst;
    concatenateImageChannel(rgbFilteredX, w, h, dst);

    delete [] rgbFilteredX;
    rgbFilteredX = nullptr;
    delete [] rgbFilteredY;
    rgbFilteredY = nullptr;

    return dst;
}

// 设计空域滤波界面
void SDFilterDialog::iniUI()
{
    // four image labels
    srcImageLabel = new QLabel();
    srcImageLabel->setAlignment(Qt::AlignCenter);

    robertsImageLabel = new QLabel();
    robertsImageLabel->setAlignment(Qt::AlignCenter);

    sobelImageLabel = new QLabel();
    sobelImageLabel->setAlignment(Qt::AlignCenter);

    prewittImageLabel = new QLabel();
    prewittImageLabel->setAlignment(Qt::AlignCenter);

    laplacian4ImageLabel = new QLabel();
    laplacian4ImageLabel->setAlignment(Qt::AlignCenter);

    laplacian8ImageLabel = new QLabel();
    laplacian8ImageLabel->setAlignment(Qt::AlignCenter);

    LOGImageLabel = new QLabel();
    LOGImageLabel->setAlignment(Qt::AlignCenter);
    LOGImageLabel->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

    srcLabel = new QLabel(tr("Original"));
    srcLabel->setAlignment(Qt::AlignCenter);

    robertsLabel = new QLabel(tr("Roberts"));
    robertsLabel->setAlignment(Qt::AlignCenter);

    sobelLabel = new QLabel(tr("Sobel"));
    sobelLabel->setAlignment(Qt::AlignCenter);

    prewittLabel = new QLabel(tr("Prewitt"));
    prewittLabel->setAlignment(Qt::AlignCenter);

    laplacian4Label = new QLabel(tr("Laplacian4"));
    laplacian4Label->setAlignment(Qt::AlignCenter);

    laplacian8Label = new QLabel(tr("Laplacian8"));
    laplacian8Label->setAlignment(Qt::AlignCenter);

    LOGLabel = new QLabel(tr("LOG"));
    LOGLabel->setAlignment(Qt::AlignCenter);

    // border type
    borderTypeLabel = new QLabel(tr("Border"));
    borderTypeLabel->setAlignment(Qt::AlignRight);
    borderTypeComboBox = new QComboBox;
    borderTypeComboBox->addItem(tr("replicate"));
    borderTypeComboBox->addItem(tr("reflect"));
    borderTypeComboBox->addItem(tr("reflect101"));
    borderTypeComboBox->addItem(tr("wrap"));
    borderTypeComboBox->addItem(tr("zero padding"));

    // border size
    sigmaLabel = new QLabel(tr("Size"));
    sigmaSlider = new FloatSlider(Qt::Horizontal);
    sigmaEdit = new QLineEdit();

    //sigmaLabel->setMinimumWidth(30);
    sigmaLabel->setAlignment(Qt::AlignRight);
    //sigmaEdit->setMinimumWidth(16);
    sigmaSlider->setFloatRange(2, maxSigma);
    sigmaSlider->setFloatValue(sigma);
    sigmaSlider->setFloatStep(0.2f);
    sigmaEdit->setText(QString("%1").arg(sigma));

    // signal slot
    connect(sigmaSlider, SIGNAL(floatValueChanged(float)), this, SLOT(updateLOGImage(float)));

    // three buttons
    btnOK = new QPushButton(tr("OK"));
    btnCancel = new QPushButton(tr("Cancel"));
    btnClose = new QPushButton(tr("Exit"));

    connect(btnOK, SIGNAL(clicked()), this, SLOT(accept()));
    connect(btnCancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(btnClose, SIGNAL(clicked()), this, SLOT(close()));

    QHBoxLayout *layout1 = new QHBoxLayout;
    layout1->addWidget(srcImageLabel);
    layout1->addWidget(robertsImageLabel);
    layout1->addWidget(sobelImageLabel);

    QHBoxLayout *layout2 = new QHBoxLayout;
    layout2->addWidget(srcLabel);
    layout2->addWidget(robertsLabel);
    layout2->addWidget(sobelLabel);

    QHBoxLayout *layout3 = new QHBoxLayout;
    layout3->addWidget(prewittImageLabel);
    layout3->addWidget(laplacian4ImageLabel);
    layout3->addWidget(laplacian8ImageLabel);

    QHBoxLayout *layout4 = new QHBoxLayout;
    layout4->addWidget(prewittLabel);
    layout4->addWidget(laplacian4Label);
    layout4->addWidget(laplacian8Label);

    QHBoxLayout *layout5 = new QHBoxLayout;
    layout5->addWidget(LOGImageLabel);

    QHBoxLayout *layout6 = new QHBoxLayout;
    layout6->addWidget(LOGLabel);

    QHBoxLayout *layout7 = new QHBoxLayout;

    layout7->addWidget(sigmaLabel, 2);
    layout7->addWidget(sigmaSlider, 6);
    layout7->addWidget(sigmaEdit, 2);
    //layout7->addStretch();

    QHBoxLayout *layout8 = new QHBoxLayout;
    layout8->addStretch();
    layout8->addWidget(btnOK);
    layout8->addWidget(btnCancel);
    layout8->addStretch();
    layout8->addWidget(btnClose);

    // main layout
    QVBoxLayout *mainlayout = new QVBoxLayout;
    mainlayout->addLayout(layout1);
    mainlayout->addLayout(layout2);
    mainlayout->addLayout(layout3);
    mainlayout->addLayout(layout4);
    mainlayout->addLayout(layout5);
    mainlayout->addLayout(layout6);
    mainlayout->addLayout(layout7);
    mainlayout->addLayout(layout8);
    setLayout(mainlayout);
}

void SDFilterDialog::setImage(QImage image, QLabel *label)
{
    QPixmap pix;
    pix.fromImage(image);
    label->setPixmap(pix);
}

//log算子图像滤波下，滤波后图像随着sigma的改变而更新
void SDFilterDialog::updateLOGImage(float value)
{
    if (QObject::sender() == sigmaSlider)
    {
        sigma = value;
        sigmaEdit->setText(QString("%1").arg(value));
        LOGImage = imageLOGFilter(sigma);
        LOGImageLabel->setPixmap(QPixmap::fromImage(LOGImage).scaled(LOGImageLabel->width(), LOGImageLabel->height()));
    }
}
