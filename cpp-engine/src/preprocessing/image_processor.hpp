#pragma once

#include <span>
#include <string>
#include <vector>
#include "ingestion/file_loader.hpp"
#include <opencv2/core.hpp>

namespace clearcapture {

/**
 * @brief Image preprocessing pipeline using OpenCV.
 *
 * Applies grayscale conversion, deskewing, and denoising
 * to prepare document images for OCR.
 */
class ImageProcessor {
public:
    ImageProcessor() = default;

    /**
     * @brief Preprocess a raw image buffer into a grayscale, deskewed, denoised image.
     *
     * @param buffer Raw image file bytes (PNG, JPEG, TIFF, BMP).
     * @return cv::Mat The preprocessed grayscale image, or error string.
     */
    [[nodiscard]] Result<cv::Mat> preprocess(std::span<const char> buffer);

    /**
     * @brief Preprocess a PDF page rendered as pixel data.
     *
     * @param pixels Raw pixel buffer (grayscale or RGB).
     * @param width Image width in pixels.
     * @param height Image height in pixels.
     * @param channels Number of channels (1 = grayscale, 3 = RGB).
     * @return cv::Mat The preprocessed grayscale image.
     */
    [[nodiscard]] cv::Mat preprocess_from_pixels(const unsigned char* pixels,
                                                   int width,
                                                   int height,
                                                   int channels);

    /**
     * @brief Convert image to grayscale.
     */
    [[nodiscard]] static cv::Mat to_grayscale(const cv::Mat& image);

    /**
     * @brief Deskew the image (correct rotation).
     * Uses Hough transform to detect the dominant angle and rotates.
     */
    [[nodiscard]] static cv::Mat deskew(const cv::Mat& grayscale);

    /**
     * @brief Apply noise reduction (bilateral filter preserves edges).
     */
    [[nodiscard]] static cv::Mat denoise(const cv::Mat& image);

    /**
     * @brief Apply adaptive thresholding to get clean binary image.
     */
    [[nodiscard]] static cv::Mat binarize(const cv::Mat& image);

    /**
     * @brief Full preprocessing pipeline.
     */
    [[nodiscard]] static cv::Mat full_pipeline(const cv::Mat& input);

private:
    /**
     * @brief Detect the skew angle using Hough line transform.
     */
    [[nodiscard]] static double detect_skew_angle(const cv::Mat& binary);
};

} // namespace clearcapture