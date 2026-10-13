// SPDX-License-Identifier: LGPL-2.1-or-later
// Features and geometric check. Values chosen on 25 touches of one finger and
// 10 of another (2026-10-13): impostor pairs never exceeded 3 consistent
// matches, genuine touches that overlap an enrolled one gave 11 or more.
#include <mutex>
#include <opencv2/calib3d.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/imgproc.hpp>
#include "elan0c4d-sift.hpp"

namespace
{
constexpr double kClaheClip = 2.0;       // local contrast, tiles of a quarter of the width
constexpr int kClaheTiles = 4;
constexpr double kUpscale = 2.0;         // ridges are ~4.5 px apart: SIFT needs more room
constexpr double kContrast = 0.01;       // keeps the weak keypoints of a small sensor
constexpr float kRatio = 0.75f;          // Lowe's ratio test
constexpr double kRansacPx = 4.0;        // reprojection tolerance in upscaled pixels
constexpr double kScaleTolerance = 0.15; // the finger is pressed, not zoomed
constexpr int kMaxKeypoints = 1000;      // real touches give 139-280

std::once_flag single_thread;
}  // namespace

namespace elan
{

View
features (const guint8 *image, guint width, guint height)
{
  // fprintd is a single-threaded daemon: no OpenCV worker pool inside it.
  std::call_once (single_thread, [] { cv::setNumThreads (1); });
  cv::Mat img (static_cast<int> (height), static_cast<int> (width), CV_8U, const_cast<guint8 *> (image));
  cv::Mat enhanced, big;
  cv::createCLAHE (kClaheClip, cv::Size (kClaheTiles, kClaheTiles))->apply (img, enhanced);
  cv::resize (enhanced, big, cv::Size (), kUpscale, kUpscale, cv::INTER_CUBIC);

  std::vector<cv::KeyPoint> keypoints;
  cv::Mat descriptors;
  cv::SIFT::create (kMaxKeypoints, 3, kContrast)->detectAndCompute (big, cv::noArray (), keypoints, descriptors);

  View view;
  cv::KeyPoint::convert (keypoints, view.points);
  if (!descriptors.empty ())
    descriptors.convertTo (view.descriptors, CV_8U);  // SIFT values are integers 0..255
  return view;
}

int
consistent_matches (const View &probe, const View &stored)
{
  if (probe.descriptors.rows < 3 || stored.descriptors.rows < 3)
    return 0;
  cv::Mat a, b;
  probe.descriptors.convertTo (a, CV_32F);
  stored.descriptors.convertTo (b, CV_32F);

  std::vector<std::vector<cv::DMatch>> pairs;
  cv::BFMatcher (cv::NORM_L2).knnMatch (a, b, pairs, 2);
  std::vector<cv::Point2f> from, to;
  for (const auto &p : pairs)
    if (p.size () == 2 && p[0].distance < kRatio * p[1].distance)
      {
        from.push_back (probe.points[p[0].queryIdx]);
        to.push_back (stored.points[p[0].trainIdx]);
      }
  if (from.size () < 3)
    return 0;

  std::vector<uchar> inliers;
  cv::Mat m = cv::estimateAffinePartial2D (from, to, inliers, cv::RANSAC, kRansacPx);
  if (m.empty ())
    return 0;
  double scale = std::hypot (m.at<double> (0, 0), m.at<double> (0, 1));
  if (std::abs (scale - 1.0) > kScaleTolerance)
    return 0;
  return cv::countNonZero (inliers);
}

}  // namespace elan
