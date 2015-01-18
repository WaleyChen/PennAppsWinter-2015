//
//  cvneon.h
//  OpenCV Tutorial
//
//  Created by BloodAxe on 8/12/12.
//
//
#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/nonfree/nonfree.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#import <opencv2/opencv.hpp>
#ifndef OpenCV_Tutorial_cvneon_h
#define OpenCV_Tutorial_cvneon_h

namespace cv
{
  //! Return new matrix identical to the input but 16-bytes aligned
  cv::Mat align16(const cv::Mat& m);
  
  //! Return true if input matrix has 16 bytes aligned rows
  bool isAligned(const cv::Mat& m);
  
  //! 
  void neon_cvtColorBGRA2GRAY(const cv::Mat& input, cv::Mat& gray);
  
  //! 
  void neon_transform_bgra(const cv::Mat& input, cv::Mat& result, const cv::Mat_<float>& m_transposed);
}

#endif
