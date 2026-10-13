// SPDX-License-Identifier: LGPL-2.1-or-later
// Keypoint comparator shared by elan0c4d-sift*.cpp: SIFT on the enhanced
// image and a RANSAC similarity check, measured on real 0c4d touches.
#pragma once

#include <glib.h>
#include <opencv2/core.hpp>
#include <stdexcept>
#include <vector>

namespace elan
{

// One touch: where its keypoints are and their 128-byte SIFT descriptors.
struct View
{
  std::vector<cv::Point2f> points;
  cv::Mat descriptors;  // CV_8U, one row per point
};

// A stored template that is truncated, foreign or inconsistent.
struct TemplateError : std::runtime_error
{
  using std::runtime_error::runtime_error;
};

View features (const guint8 *image, guint width, guint height);

// Matches that agree on one rotation + shift (+-15 % scale); 0 if none does.
int consistent_matches (const View &probe, const View &stored);

GBytes *pack (const std::vector<View> &views);
std::vector<View> unpack (GBytes *tmpl);

}  // namespace elan
