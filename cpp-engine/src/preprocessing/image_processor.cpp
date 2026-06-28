#include "image_processor.hpp"

#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <format>
#include <iostream>
#include <numbers>
#include <cmath>

namespace clearcapture {

// Image Decoding

Result<cv::Mat> ImageProcessor::preprocess(std::span<const char> buffer) {
    // Decode raw bytes into an OpenCV Mat
    std::vector<unsigned char> vec(buffer.begin(), buffer.end());
    cv::Mat raw = cv::imdecode(vec, cv::IMREAD_COLOR);
    if (raw.empty()) {
        return Result<cv::Mat>("failed to decode image from buffer");
    }

    cv::Mat result = full_pipeline(raw);
    return result;
}

cv::Mat ImageProcessor::preprocess_from_pixels(const unsigned char* pixels,
                                                int width,
                                                int height,
                                                int channels) {
    int cv_type = (channels == 1) ? CV_8UC1 : CV_8UC3;
    cv::Mat raw(height, width, cv_type, const_cast<unsigned char*>(pixels));
    return full_pipeline(raw);
}

// Grayscale Conversion

cv::Mat ImageProcessor::to_grayscale(const cv::Mat& image) {
    if (image.channels() == 1) {
        return image.clone();
    }
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    return gray;
}

// Skew Detection and Correction

double ImageProcessor::detect_skew_angle(const cv::Mat& binary) {
    // Find all non-zero points
    std::vector<cv::Point> points;
    cv::findNonZero(binary, points);

    // If there are very few non-zero pixels, there's nothing to deskew
    if (points.size() < 100) {
        return 0.0;
    }

    // Fit a line using the least squares method
    cv::Vec4f line;
    cv::fitLine(points, line, cv::DIST_L2, 0, 0.01, 0.01);

    // Extract angle from the direction vector
    double angle_rad = std::atan2(line[1], line[0]);
    double angle_deg = angle_rad * 180.0 / std::numbers::pi;

    // Clamp to reasonable skew range (-45 to +45 degrees)
    // Any skew beyond 45 degrees is likely a degenerate detection
    if (std::abs(angle_deg) > 45.0) {
        angle_deg = 0.0;
    }

    // Only correct if skew is significant (> 0.5 degrees)
    if (std::abs(angle_deg) < 0.5) {
        return 0.0;
    }

    return angle_deg;
}

cv::Mat ImageProcessor::deskew(const cv::Mat& grayscale) {
    // Threshold to binary
    cv::Mat binary;
    cv::threshold(grayscale, binary, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    double angle = detect_skew_angle(binary);
    if (std::abs(angle) < 0.5) {
        return grayscale.clone(); // Already straight enough
    }

    // Rotate the image
    cv::Point2f center(grayscale.cols / 2.0f, grayscale.rows / 2.0f);
    cv::Mat rotation_matrix = cv::getRotationMatrix2D(center, angle, 1.0);

    // Calculate new bounding rectangle
    cv::Rect2f bbox = cv::RotatedRect(cv::Point2f(), grayscale.size(), angle).boundingRect2f();

    // Adjust transformation matrix
    rotation_matrix.at<double>(0, 2) += bbox.width / 2.0 - grayscale.cols / 2.0;
    rotation_matrix.at<double>(1, 2) += bbox.height / 2.0 - grayscale.rows / 2.0;

    cv::Mat deskewed;
    cv::warpAffine(grayscale, deskewed, rotation_matrix, bbox.size(),
                   cv::INTER_CUBIC, cv::BORDER_CONSTANT, cv::Scalar(255));

    std::cout << std::format("[Preprocess] Deskewed by {:.2f} degrees\n", angle);
    return deskewed;
}

// Denoising

cv::Mat ImageProcessor::denoise(const cv::Mat& image) {
    // Bilateral filter preserves edges while reducing noise
    cv::Mat denoised;
    cv::bilateralFilter(image, denoised, 9, 75, 75);
    return denoised;
}

// Binarization

cv::Mat ImageProcessor::binarize(const cv::Mat& image) {
    cv::Mat binary;
    // Adaptive thresholding handles varying lighting conditions
    cv::adaptiveThreshold(image, binary, 255,
                          cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                          cv::THRESH_BINARY, 11, 2);
    return binary;
}

// Full Pipeline

cv::Mat ImageProcessor::full_pipeline(const cv::Mat& input) {
    // Step 1: Convert to grayscale
    cv::Mat gray = to_grayscale(input);

    // Step 2: Upscale low-resolution images for better OCR
    cv::Mat scaled = gray;
    if (gray.cols < 600 || gray.rows < 400) {
        double scale = std::max(600.0 / gray.cols, 400.0 / gray.rows);
        cv::resize(gray, scaled, cv::Size(), scale, scale, cv::INTER_CUBIC);
    }

    // Step 3: Skip deskew, denoise, and binarization — Tesseract handles
    // its own binarization internally and does better on clean input.
    // The full pipeline (deskew, denoise, adaptive threshold) is available
    // for scanned/photographed documents via the dedicated methods above.
    // For clean digital documents, grayscale + optional upscale is optimal.

    return scaled;
}

} // namespace clearcapture