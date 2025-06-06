#include "tiff_image.h"
#include <tiff.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include "kernel.h"

const Kernel<int> kKernelSobelX(1, 3, {{1, 2, 1}}, true);
const Kernel<int> kKernelSobelY(3, 1, {{1}, {2}, {1}}, true);
const Kernel<int> kKernelPrewittX(1, 3, {{1, 1, 1}}, true);
const Kernel<int> kKernelPrewittY(3, 1, {{1}, {1}, {1}}, true);
const Kernel<int> kKernelGradientX(1, 3, {{1, 0, -1}}, true);
const Kernel<int> kKernelGradientY(3, 1, {{1}, {0}, {-1}}, true);

uint16_t* TIFFImage::AddAbsMtx(const int* mtx1, const int* mtx2, size_t height,
                               size_t width) {
  uint16_t* result = new uint16_t[height * width];
  for (size_t i = 0; i < height; i++) {
    for (size_t j = 0; j < width; j++) {
      int sum = abs(mtx1[i * width + j]) + abs(mtx2[i * width + j]);
      result[i * width + j] = static_cast<uint16_t>(std::min(sum, 65535));
    }
  }
  return result;
}

TIFFImage::TIFFImage() {
}

TIFFImage::TIFFImage(const char* name) noexcept(false) {
  Open(name);
}

TIFFImage::TIFFImage(const std::string name) noexcept(false) {
  Open(name);
}

TIFFImage::TIFFImage(size_t width, size_t height, uint16_t samples_per_pixel,
                     uint16_t bits_per_sample, uint16_t photo_metric,
                     uint16_t resolution_unit, uint16_t config)
    : width_(width),
      height_(height),
      samples_per_pixel_(samples_per_pixel),
      bits_per_sample_(bits_per_sample),
      photo_metric_(photo_metric),
      resolution_unit_(resolution_unit),
      config_(config) {
  image_ = new uint16_t[width_ * height_];
}

TIFFImage::TIFFImage(const TIFFImage& other) {
  CopyFields(other);
  CopyDeviceMemPointers(other);
  std::memcpy(image_, other.image_, width_ * height_ * sizeof(uint16_t));
}

TIFFImage::~TIFFImage() {
  Close();
  if (image_ != nullptr) {
    delete[] image_;
    image_ = nullptr;
  }
}

void TIFFImage::Open(const char* name) noexcept(false) {
  tif_ = TIFFOpen(name, "r");
  if (tif_ == nullptr) {
    throw std::runtime_error("Невозможно открыть файл");
  }
  TIFFGetField(tif_, TIFFTAG_IMAGEWIDTH, &width_);
  TIFFGetField(tif_, TIFFTAG_IMAGELENGTH, &height_);
  TIFFGetField(tif_, TIFFTAG_SAMPLESPERPIXEL, &samples_per_pixel_);
  TIFFGetField(tif_, TIFFTAG_BITSPERSAMPLE, &bits_per_sample_);
  TIFFGetField(tif_, TIFFTAG_PLANARCONFIG, &config_);
  if (TIFFGetField(tif_, TIFFTAG_PHOTOMETRIC, &photo_metric_) != 1) {
    photo_metric_enabled_ = false;
    photo_metric_ = PHOTOMETRIC_MINISBLACK;
  }
  if (TIFFGetField(tif_, TIFFTAG_RESOLUTIONUNIT, &resolution_unit_) != 1) {
    resolution_unit_enabled_ = false;
    resolution_unit_ = RESUNIT_NONE;
  }
  if (TIFFGetField(tif_, TIFFTAG_XRESOLUTION, &resolution_x_) == 0) {
    resolution_x_ = -1;
  }
  if (TIFFGetField(tif_, TIFFTAG_YRESOLUTION, &resolution_y_) == 0) {
    resolution_y_ = -1;
  }
  if (bits_per_sample_ != 16) {
    throw std::runtime_error("Поддерживаются только 16-битные изображения");
  }
  if (samples_per_pixel_ != 1) {
    throw std::runtime_error("Поддерживаются только одноканальные изображения");
  }
  if (config_ != PLANARCONFIG_CONTIG) {
    throw std::runtime_error("Поддерживаются только непрерывные плоскости");
  }
  image_ = new uint16_t[width_ * height_];
  for (size_t i = 0; i < height_; i++) {
    TIFFReadScanline(tif_, &image_[i * width_], i);
  }
}

void TIFFImage::Open(const std::string& name) noexcept(false) {
  Open(name.c_str());
}

void TIFFImage::Close() {
  if (tif_ != nullptr) {
    TIFFClose(tif_);
    tif_ = nullptr;
  }
}

void TIFFImage::Save(const char* name) {
  TIFF* tif = TIFFOpen(name, "w");
  if (tif == nullptr) {
    throw std::runtime_error("Невозможно создать файл");
  }
  TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width_);
  TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height_);
  TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, samples_per_pixel_);
  TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, bits_per_sample_);
  TIFFSetField(tif, TIFFTAG_PLANARCONFIG, config_);
  if (photo_metric_enabled_) {
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, photo_metric_);
  }
  if (resolution_unit_enabled_) {
    TIFFSetField(tif, TIFFTAG_RESOLUTIONUNIT, resolution_unit_);
  }
  if (resolution_x_ != -1) {
    TIFFSetField(tif, TIFFTAG_XRESOLUTION, resolution_x_);
  }
  if (resolution_y_ != -1) {
    TIFFSetField(tif, TIFFTAG_YRESOLUTION, resolution_y_);
  }
  for (size_t i = 0; i < height_; i++) {
    TIFFWriteScanline(tif, &image_[i * width_], i);
  }
  TIFFClose(tif);
}

void TIFFImage::Save(const std::string& name) {
  Save(name.c_str());
}

void TIFFImage::Clear() {
  if (image_ != nullptr) {
    delete[] image_;
  }
  image_ = nullptr;
  width_ = 0;
  height_ = 0;
  samples_per_pixel_ = 0;
  bits_per_sample_ = 0;
  photo_metric_ = 0;
  resolution_unit_ = 0;
  config_ = 0;
  photo_metric_enabled_ = true;
  resolution_unit_enabled_ = true;
  resolution_x_ = 0;
  resolution_y_ = 0;
  FreeDeviceMemory();
}

uint16_t TIFFImage::Get(const int x, const int y) const noexcept(false) {
  if ((width_ != 0u) && (height_ != 0u) && (image_ != nullptr)) {
    // if (x < 0 || x >= static_cast<int>(width_) || y < 0 ||
    //     y >= static_cast<int>(height_)) {
    //   return 0;
    // }
    return image_[std::clamp(y, 0, (int)height_ - 1) * width_ +
                  std::clamp(x, 0, (int)width_ - 1)];
    return image_[y * width_ + x];
  } else {
    throw std::runtime_error("Изображение не загружено");
  }
}

size_t TIFFImage::GetWidth() const {
  return width_;
}

size_t TIFFImage::GetHeight() const {
  return height_;
}

void TIFFImage::Set(const size_t x, const size_t y,
                    const uint16_t value) noexcept(false) {
  if ((width_ != 0u) && (height_ != 0u) && (image_ != nullptr)) {
    if (x >= width_ || y >= height_) {
      throw std::runtime_error("Выход за границы изображения");
    }
    image_[y * width_ + x] = value;
  } else {
    throw std::runtime_error("Изображение не загружено");
  }
}

void TIFFImage::SetImage(const size_t width, const size_t height,
                         const uint16_t* image) noexcept(false) {
  if (width == 0 || height == 0) {
    throw std::runtime_error("Размер изображения не может быть нулевым");
  }
  if (image == nullptr) {
    throw std::runtime_error("Изображение не может быть нулевым");
  }
  if (width_ != width || height_ != height) {
    Clear();
    width_ = width;
    height_ = height;
    image_ = new uint16_t[width_ * height_];
  }
  std::memcpy(image_, image, width_ * height_ * sizeof(uint16_t));
}

void TIFFImage::CopyFields(const TIFFImage& other) {
  width_ = other.width_;
  height_ = other.height_;
  samples_per_pixel_ = other.samples_per_pixel_;
  bits_per_sample_ = other.bits_per_sample_;
  photo_metric_ = other.photo_metric_;
  resolution_unit_ = other.resolution_unit_;
  config_ = other.config_;
  photo_metric_enabled_ = other.photo_metric_enabled_;
  resolution_unit_enabled_ = other.resolution_unit_enabled_;
  resolution_x_ = other.resolution_x_;
  resolution_y_ = other.resolution_y_;
  if (image_ != nullptr) {
    delete[] image_;
  }
  image_ = new uint16_t[width_ * height_];
}

void TIFFImage::CopyDeviceMemPointers(const TIFFImage& other) {
  d_src_ = other.d_src_;
  d_dst_ = other.d_dst_;
  d_gaussian_sep_temp_ = other.d_gaussian_sep_temp_;
  d_sep_g_x_ = other.d_sep_g_x_;
  d_sep_g_y_ = other.d_sep_g_y_;
  d_sep_result_x_ = other.d_sep_result_x_;
  d_sep_result_y_ = other.d_sep_result_y_;
  d_gaussian_kernel_ = other.d_gaussian_kernel_;
  d_mem_allocaded_ = other.d_mem_allocaded_;
  gaussian_kernel_size_ = other.gaussian_kernel_size_;
  gaussian_sigma_ = other.gaussian_sigma_;
}

bool TIFFImage::operator==(const TIFFImage& other) const {
  if (width_ != other.width_ || height_ != other.height_ ||
      samples_per_pixel_ != other.samples_per_pixel_ ||
      bits_per_sample_ != other.bits_per_sample_ ||
      photo_metric_ != other.photo_metric_ ||
      resolution_unit_ != other.resolution_unit_ || config_ != other.config_ ||
      photo_metric_enabled_ != other.photo_metric_enabled_ ||
      resolution_unit_enabled_ != other.resolution_unit_enabled_ ||
      resolution_x_ != other.resolution_x_ ||
      resolution_y_ != other.resolution_y_) {
    return false;
  }

  for (size_t i = 0; i < height_; i++) {
    for (size_t j = 0; j < width_; j++) {
      if (image_[i * width_ + j] != other.image_[i * width_ + j]) {
        // std::cerr << "Images differ at (" << i << ", " << j
        //           << "): " << image_[i * width_ + j] << " vs "
        //           << other.image_[i * width_ + j] << std::endl;
        return false;
      }
    }
  }

  return true;
}

TIFFImage& TIFFImage::operator=(const TIFFImage& other) {
  if (this == &other) {
    return *this;
  }
  CopyFields(other);
  CopyDeviceMemPointers(other);
  std::memcpy(image_, other.image_, width_ * height_ * sizeof(uint16_t));
  return *this;
}

TIFFImage TIFFImage::SetKernel(const Kernel<int>& kernel, bool rotate) const {
  TIFFImage result(*this);
  if (rotate) {
    Kernel<int> kernel_y(kernel);
    kernel_y = kernel_y.Rotate(KernelRotationDegrees::DEGREES_90);
    int radius = kernel.GetHeight() / 2;
    for (size_t i = 0; i < height_; i++) {
      for (size_t j = 0; j < width_; j++) {
        int g_x = 0, g_y = 0;
        for (int k = -radius; k <= radius; k++) {
          for (int l = -radius; l <= radius; l++) {
            g_x += kernel.Get(l + radius, k + radius) * Get(j + l, i + k);
            g_y += kernel_y.Get(l + radius, k + radius) * Get(j + l, i + k);
          }
        }
        result.image_[i * width_ + j] =
            std::clamp(abs(g_x) + abs(g_y), 0, 65535);
      }
    }
  } else {
    int radius = kernel.GetHeight() / 2;
    for (size_t i = 0; i < height_; i++) {
      for (size_t j = 0; j < width_; j++) {
        int g = 0;
        for (int k = -radius; k <= radius; k++) {
          for (int l = -radius; l <= radius; l++) {
            g += kernel.Get(l + radius, k + radius) * Get(j + l, i + k);
          }
        }
        result.image_[i * width_ + j] = std::clamp(abs(g), 0, 65535);
      }
    }
  }
  return result;
}

TIFFImage TIFFImage::SetKernelSobelSep() const {
  TIFFImage result(*this);
  delete[] result.image_;
  int* g_x = new int[width_ * height_]();
  int* g_y = new int[width_ * height_]();
  for (size_t i = 0; i < height_; i++) {
    for (size_t j = 0; j < width_; j++) {
      for (int k = -1; k <= 1; k++) {
        g_x[i * width_ + j] += Get(j + k, i) * kKernelSobelY.Get(0, k + 1);
        g_y[i * width_ + j] += Get(j, i + k) * kKernelSobelX.Get(k + 1, 0);
      }
    }
  }
  int* result_x = new int[width_ * height_]();
  int* result_y = new int[width_ * height_]();
  for (size_t i = 0; i < height_; i++) {
    for (size_t j = 0; j < width_; j++) {
      for (int k = -1; k <= 1; k++) {
        result_x[i * width_ + j] +=
            g_x[std::clamp((int)(i + k), 0, (int)height_ - 1) * width_ + j] *
            kKernelGradientX.Get(k + 1, 0);
        result_y[i * width_ + j] +=
            g_y[i * width_ + std::clamp((int)j + k, 0, (int)width_ - 1)] *
            kKernelGradientY.Get(0, k + 1);
      }
    }
  }
  result.image_ = AddAbsMtx(result_x, result_y, height_, width_);
  delete[] g_x;
  delete[] g_y;
  delete[] result_x;
  delete[] result_y;
  return result;
}

TIFFImage TIFFImage::SetKernelPrewittSep() const {
  TIFFImage result(*this);
  delete[] result.image_;
  int* g_x = new int[width_ * height_]();
  int* g_y = new int[width_ * height_]();
  for (size_t i = 0; i < height_; i++) {
    for (size_t j = 0; j < width_; j++) {
      for (int k = -1; k <= 1; k++) {
        g_x[i * width_ + j] += Get(j + k, i) * kKernelPrewittY.Get(0, k + 1);
        g_y[i * width_ + j] += Get(j, i + k) * kKernelPrewittX.Get(k + 1, 0);
      }
    }
  }
  int* result_x = new int[width_ * height_]();
  int* result_y = new int[width_ * height_]();
  for (size_t i = 0; i < height_; i++) {
    for (size_t j = 0; j < width_; j++) {
      for (int k = -1; k <= 1; k++) {
        result_x[i * width_ + j] +=
            g_x[std::clamp((int)(i + k), 0, (int)height_ - 1) * width_ + j] *
            kKernelGradientX.Get(k + 1, 0);
        result_y[i * width_ + j] +=
            g_y[i * width_ + std::clamp((int)j + k, 0, (int)width_ - 1)] *
            kKernelGradientY.Get(0, k + 1);
      }
    }
  }
  result.image_ = AddAbsMtx(result_x, result_y, height_, width_);
  delete[] g_x;
  delete[] g_y;
  delete[] result_x;
  delete[] result_y;
  return result;
}

TIFFImage TIFFImage::GaussianBlur(const size_t size, const float sigma) const {
  Kernel<float> kernel = Kernel<float>::GetGaussianKernel(size, sigma);
  TIFFImage result(*this);
  int radius = kernel.GetHeight() / 2;
  for (size_t i = 0; i < height_; i++) {
    for (size_t j = 0; j < width_; j++) {
      float sum = 0.0;
      for (int k = -radius; k <= radius; k++) {
        for (int l = -radius; l <= radius; l++) {
          sum += kernel.Get(l + radius, k + radius) * Get(j + l, i + k);
        }
      }
      result.image_[i * width_ + j] =
          static_cast<uint16_t>(std::clamp(std::round(sum), 0.0f, 65535.0f));
    }
  }
  return result;
}

TIFFImage TIFFImage::GaussianBlurSep(const size_t size,
                                     const float sigma) const {
  Kernel<float> kernel = Kernel<float>::GetGaussianKernelSep(size, sigma);
  TIFFImage result(*this);
  float* temp = new float[width_ * height_];
  int radius = kernel.GetHeight() / 2;
  for (size_t i = 0; i < height_; i++) {
    for (size_t j = 0; j < width_; j++) {
      float sum = 0.0;
      for (int k = -radius; k <= radius; k++) {
        sum += kernel.Get(k + radius, 0) * Get(j, i + k);
      }
      temp[i * width_ + j] = sum;
    }
  }
  for (size_t i = 0; i < height_; i++) {
    for (size_t j = 0; j < width_; j++) {
      float sum = 0.0;
      for (int k = -radius; k <= radius; k++) {
        sum += kernel.Get(k + radius, 0) *
               temp[i * width_ + std::clamp((int)j + k, 0, (int)width_ - 1)];
      }
      result.image_[i * width_ + j] =
          static_cast<uint16_t>(std::clamp(std::round(sum), 0.0f, 65535.0f));
    }
  }
  delete[] temp;
  return result;
}

#ifdef USE_QT
QImage TIFFImage::ToQImage() const {
  QImage image(width_, height_, QImage::Format_Grayscale16);
  for (size_t y = 0; y < height_; ++y) {
    for (size_t x = 0; x < width_; ++x) {
      image.setPixel(x, y, image_[y * width_ + x]);
    }
  }
  return image;
}
#endif