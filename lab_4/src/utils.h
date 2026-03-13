#pragma once

#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <ios>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

struct Image {
    uint32_t width;
    uint32_t height;
    std::vector<uint8_t> bytes;
};

inline Image read_file(const std::string& path);
inline void write_file(const std::string& path, const Image& image);
inline void invert_image(Image& image);
inline void blur_image(Image& image);
inline void sobel_image(Image& image);
inline void process_image(Image& image);

inline Image read_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + path);
    }

    std::string magic;
    file >> magic;
    if (magic != "P6") {
        throw std::runtime_error("Invalid PPM format (expected P6): " + path);
    }

    Image image;
    file >> image.width >> image.height;

    int max_val = 0;
    file >> max_val;

    while (true) {
        int c = file.peek();
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            file.get();
        } else {
            break;
        }
    }

    size_t expected_size = static_cast<size_t>(image.width) * image.height * 3;
    image.bytes.resize(expected_size);

    file.read(reinterpret_cast<char*>(image.bytes.data()), expected_size);

    size_t actual_read = static_cast<size_t>(file.gcount());

    if (actual_read < expected_size) {
        size_t missing = expected_size - actual_read;
        std::fill(image.bytes.begin() + actual_read, image.bytes.end(), 0);
        std::cerr << "[WARN] " << path << ": missing " << missing
                  << " byte(s), padded with zeros\n";
    }

    return image;
}

inline void write_file(const std::string& path, const Image& image) {
    std::ofstream file(path, std::ios::binary);

    if (!file.is_open()) {
        // TODO exception
        throw std::runtime_error("Failed to open file: " + path);
        // std::cout << "Cannot open file\n";
    }

    file << "P6\n" << image.width << ' ' << image.height << "\n255\n";

    file.write(reinterpret_cast<const char*>(image.bytes.data()), image.bytes.size());

    if (!file) {
        // TODO exception
        throw std::runtime_error("Failed to write image data");
        // std::cout << "Failed to write data to file.\n";
    }
}

inline void invert_image(Image& image) {
    for (size_t i = 0; i < image.bytes.size(); ++i) {
        image.bytes[i] = 255u - image.bytes[i];
    }
}

inline void blur_image(Image& image) {
    std::vector<uint8_t> original = image.bytes;

    int w = image.width;
    int h = image.height;
    int radius = 1;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int sum_r = 0, sum_g = 0, sum_b = 0;
            int count = 0;

            for (int dy = -radius; dy <= radius; ++dy) {
                for (int dx = -radius; dx <= radius; ++dx) {
                    int nx = x + dx;
                    int ny = y + dy;

                    if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                        size_t idx = (ny * w + nx) * 3;
                        sum_r += original[idx];
                        sum_g += original[idx + 1];
                        sum_b += original[idx + 2];
                        count++;
                    }
                }
            }

            size_t idx = (y * w + x) * 3;
            image.bytes[idx] = sum_r / count;
            image.bytes[idx + 1] = sum_g / count;
            image.bytes[idx + 2] = sum_b / count;
        }
    }
}

inline void sobel_image(Image& image) {
    std::vector<uint8_t> original = image.bytes;
    int w = image.width;
    int h = image.height;

    int gx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    int gy[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};

    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            int sum_gx_r = 0, sum_gx_g = 0, sum_gx_b = 0;
            int sum_gy_r = 0, sum_gy_g = 0, sum_gy_b = 0;

            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    size_t idx = ((y + dy) * w + (x + dx)) * 3;
                    uint8_t r = original[idx];
                    uint8_t g = original[idx + 1];
                    uint8_t b = original[idx + 2];

                    sum_gx_r += r * gx[dy + 1][dx + 1];
                    sum_gx_g += g * gx[dy + 1][dx + 1];
                    sum_gx_b += b * gx[dy + 1][dx + 1];

                    sum_gy_r += r * gy[dy + 1][dx + 1];
                    sum_gy_g += g * gy[dy + 1][dx + 1];
                    sum_gy_b += b * gy[dy + 1][dx + 1];
                }
            }

            size_t idx = (y * w + x) * 3;
            image.bytes[idx] = std::min(
                255, static_cast<int>(std::sqrt(sum_gx_r * sum_gx_r + sum_gy_r * sum_gy_r)));
            image.bytes[idx + 1] = std::min(
                255, static_cast<int>(std::sqrt(sum_gx_g * sum_gx_g + sum_gy_g * sum_gy_g)));
            image.bytes[idx + 2] = std::min(
                255, static_cast<int>(std::sqrt(sum_gx_b * sum_gx_b + sum_gy_b * sum_gy_b)));
        }
    }
}

inline void process_image(Image& image) {
    blur_image(image);
    sobel_image(image);
    invert_image(image);
}