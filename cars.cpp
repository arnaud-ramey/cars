#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <stdio.h>

#define RAD2DEG     57.2957795130823208768  //!< to convert radians to degrees
#define DEG2RAD     0.01745329251994329577  //!< to convert degrees to radians

////////////////////////////////////////////////////////////////////////////////

//! \retun true if \a small is included into \a big
template<class Rect>
inline bool bboxes_included(const Rect & big, const Rect & small) {
  if (big.x > small.x)
    return 0;
  if (big.y > small.y)
    return 0;
  if (big.x + big.width < small.x + small.width)
    return 0;
  if (big.y + big.height < small.y + small.height)
    return 0;
  return 1;
}

////////////////////////////////////////////////////////////////////////////////

template<class Img, class Rect>
inline Rect img_bbox(const Img & i) {
  return Rect(0, 0, i.cols, i.rows);
} // end biggest_rect()

////////////////////////////////////////////////////////////////////////////////

template<class Rect, class Img>
inline bool bbox_included_image(const Rect & r, const Img & img) {
  return bboxes_included(img_bbox<Img, Rect>(img), r);
}

////////////////////////////////////////////////////////////////////////////////

/*!
 Determine the ROIs for pasting an image onto another.
 \param topaste
 \param dst
 \param topaste_x, topaste_y
    The coordinates where to paste "topaste" into "dst"
 \param topaste_roi (out)
   If width = 0, means "topaste" would be out of "dst" (= nothing to do).
   Otherwise, the ROI of "topaste" that needs to be copied.
 \param dst_roi (out)
   If width = 0, means "topaste" would be out of "dst" (= nothing to do).
   Otherwise, the ROI of "dst" where the ROI of "topaste" needs to be pasted.
*/
template<class Image>
inline void paste_img_compute_rois(const Image & topaste, const Image & dst,
                                   int topaste_x, int topaste_y,
                                   cv::Rect & topaste_roi, cv::Rect & dst_roi) {
  dst_roi = cv::Rect(0, 0, dst.cols, dst.rows);
  topaste_roi = cv::Rect(topaste_x, topaste_y, topaste.cols, topaste.rows);

  // sizes check
  if (topaste.cols == 0 || topaste.rows == 0 || dst.rows == 0 || dst.cols == 0) {
    dst_roi.width = dst_roi.height = topaste_roi.width = topaste_roi.height = 0;
    return;
  }

  cv::Rect inter_roi = dst_roi & topaste_roi; // rectangle_intersection
  // disjoint rectangles => do nothing
  if (inter_roi.width <= 0 || inter_roi.height <= 0) {
    dst_roi.width = dst_roi.height = topaste_roi.width = topaste_roi.height = 0;
    return;
  }
  // the corresponding width and height in input images will be the one of the inter
  dst_roi.width = inter_roi.width;
  dst_roi.height = inter_roi.height;
  topaste_roi.width = inter_roi.width;
  topaste_roi.height = inter_roi.height;
  // adjust x, depending on which side of the left edge of "dst" we paste
  if (topaste_x >= 0) { // we paste on the right side of the left edge of "dst"
    dst_roi.x = inter_roi.x;
    topaste_roi.x = 0;
  }
  else { // we paste on the left side of "dst"
    dst_roi.x = 0;
    topaste_roi.x = -topaste_x;
  }
  // same thing with y
  if (topaste_y >= 0) {
    dst_roi.y = inter_roi.y;
    topaste_roi.y = 0;
  }
  else {
    dst_roi.y = 0;
    topaste_roi.y = -topaste_y;
  }
} // end paste_img_compute_rois()

////////////////////////////////////////////////////////////////////////////////

/*! copies to \arg dst
    matrix elements of \arg topaste that are
    marked with non-zero \arg mask elements. */
template<class Image>
inline void paste_img(const Image & topaste, Image & dst,
                      int topaste_x, int topaste_y,
                      const cv::Mat* mask = NULL,
                      const std::string title = "",
                      const cv::Scalar title_color = cv::Scalar(0, 0, 255)) {
  //  printf("paste_img()\n");

  cv::Rect topaste_roi, dst_roi;
  paste_img_compute_rois(topaste, dst, topaste_x, topaste_y, topaste_roi, dst_roi);
  //  printf("topaste_roi:(%ix%i)+(%ix%i), dst_roi:(%ix%i)+(%ix%i)\n",
  //         topaste_roi.x, topaste_roi.y, topaste_roi.width, topaste_roi.height,
  //         dst_roi.x, dst_roi.y, dst_roi.width, dst_roi.height);
  if (topaste_roi.width == 0 || topaste_roi.height == 0
      || dst_roi.width == 0 || dst_roi.height == 0)
    return;

  // make the proper pasting
  Image topaste_sub = topaste(topaste_roi),
      dst_sub = dst(dst_roi);
  bool use_mask = (mask != NULL);
  if (use_mask && !bbox_included_image(topaste_roi, *mask)) {
    //    printf("paste_img(): incorrect dims of mask (%i,%i), topaste_roi:%s\n",
    //           mask->cols, mask->rows, print_rect(topaste_roi).c_str());
    use_mask = false;
  }
  if (use_mask) {
    const cv::Mat mask_sub = (*mask)(topaste_roi);
    topaste_sub.copyTo(dst_sub, mask_sub);
  }
  else
    topaste_sub.copyTo(dst_sub);

  // put the title if needed
  if (title != "")
    cv::putText(dst, title,
                cv::Point(dst_roi.x + 5, dst_roi.y + dst_roi.height - 10),
                cv::FONT_HERSHEY_PLAIN, 1, title_color);
}

////////////////////////////////////////////////////////////////////////////////

/*!
  Roate an image by a given angle.
 \param src
    The image to be rotated
 \param dst
    The resulting rotated image
 \param angle_rad
    The angle of rotation, in radians
 \param rotation_center
    The center of rotation. Must be in "src" bounds.
 \param flags, borderMode, borderValue
    \see cv::warpAffine()
    http://docs.opencv.org/modules/imgproc/doc/geometric_transformations.html?highlight=warpaffine#warpaffine
*/
inline void rotate_image(const cv::Mat & src, cv::Mat & dst,
                         const double & angle_rad, const cv::Point & rotation_center,
                         int flags=cv::INTER_LINEAR,
                         int borderMode=cv::BORDER_CONSTANT,
                         const cv::Scalar& borderValue=cv::Scalar()) {
  cv::Mat rot_mat = cv::getRotationMatrix2D
                    (rotation_center, angle_rad * RAD2DEG, 1.0);
  cv::warpAffine(src, dst, rot_mat, src.size(), flags, borderMode, borderValue);
} // end rotate_image();

////////////////////////////////////////////////////////////////////////////////

inline void color_negmask(const cv::Mat3b & img, const cv::Scalar color,
                          cv::Mat1b & mask) {
  unsigned int rows = img.rows, cols = img.cols;
  mask.create(img.size());
  mask.setTo(0);
  cv::Vec3b color3b(color[0], color[1], color[2]);
  for (unsigned int row = 0; row < rows; ++row) {
    const cv::Vec3b*row_ptr = img.ptr<cv::Vec3b>(row);
    uchar* mask_buffer_ptr = mask.ptr<uchar>(row);
    for (unsigned int col = 0; col < cols; ++col) {
      if (row_ptr[col] != color3b)
        mask_buffer_ptr[col] = 255;
    } // end loop col
  } // end loop row
} // end color_negmask();

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class CarDrawer {
public:
  CarDrawer(const cv::Mat3b & img,
      const cv::Point & front_wheel_center,
      const int & front_wheel_radius_px,
      const cv::Point & back_wheel_center,
      const int & back_wheel_radius_px) {
    img.copyTo(_car_img);
    cv::Scalar lodiff = cv::Scalar::all(0), updiff = cv::Scalar::all(20);
    cv::floodFill(_car_img, cv::Point(0, 0),                   cv::Scalar::all(0), 0, lodiff, updiff);
    cv::floodFill(_car_img, cv::Point(0, _car_img.rows-1),          cv::Scalar::all(0), 0, lodiff, updiff);
    cv::floodFill(_car_img, cv::Point(_car_img.cols-1, _car_img.rows-1), cv::Scalar::all(0), 0, lodiff, updiff);
    cv::floodFill(_car_img, cv::Point(_car_img.cols-1, 0),          cv::Scalar::all(0), 0, lodiff, updiff);
    color_negmask(_car_img, cv::Scalar::all(0), _car_mask);


    _front_wheel_center = front_wheel_center;
    _front_wheel_img.create(2*front_wheel_radius_px, 2*front_wheel_radius_px);
    _front_wheel_img.setTo(cv::Scalar::all(0));
    cv::Mat1b img_mask(img.size(), (uchar) 0);
    cv::circle(img_mask, front_wheel_center, front_wheel_radius_px, CV_RGB(255, 255, 255), -1);
    paste_img(img, _front_wheel_img, -front_wheel_center.x + front_wheel_radius_px,
              -front_wheel_center.y + front_wheel_radius_px, &img_mask);
    color_negmask(_front_wheel_img, cv::Scalar::all(0), _front_wheel_mask);

    _back_wheel_center = back_wheel_center;
    _back_wheel_img.create(2*back_wheel_radius_px, 2*back_wheel_radius_px);
    _back_wheel_img.setTo(cv::Scalar::all(0));
    img_mask.setTo(0);
    cv::circle(img_mask, back_wheel_center, back_wheel_radius_px, CV_RGB(255, 255, 255), -1);
    paste_img(img, _back_wheel_img, -back_wheel_center.x + back_wheel_radius_px,
              -back_wheel_center.y + back_wheel_radius_px, &img_mask);
    color_negmask(_back_wheel_img, cv::Scalar::all(0), _back_wheel_mask);

    //  cv::imshow("car", _car);
    //  cv::imshow("front_wheel_img", _front_wheel_img);
    //  cv::imshow("back_wheel_img", _back_wheel_img);
    //  cv::imshow("front_wheel_mask", _front_wheel_mask);
    //  cv::imshow("back_wheel_mask", _back_wheel_mask);
    //  cv::waitKey(0);
  }

  //////////////////////////////////////////////////////////////////////////////

  inline cv::Point img_center(const cv::Mat & m) {
    return cv::Point(m.cols/2, m.rows/2);
  }

  void rotate_wheels(const double & angle_wheel_rad,
                     cv::Mat3b & out) {
    _car_img.copyTo(out);
    rotate_image(_front_wheel_img, _buffer, angle_wheel_rad, img_center(_front_wheel_img));
    paste_img(_buffer, out, _front_wheel_center.x - _buffer.cols/2,
              _front_wheel_center.y - _buffer.rows/2, &_front_wheel_mask);
    rotate_image(_back_wheel_img, _buffer, angle_wheel_rad, img_center(_back_wheel_img));
    paste_img(_buffer, out, _back_wheel_center.x - _buffer.cols/2,
              _back_wheel_center.y - _buffer.rows/2, &_back_wheel_mask);
    //cv::imshow("_buffer", _buffer);
  }
protected:
  cv::Mat3b _car_img, _front_wheel_img, _back_wheel_img;
  cv::Mat3b _buffer;
  cv::Mat1b _car_mask, _front_wheel_mask, _back_wheel_mask;
  cv::Point _front_wheel_center, _back_wheel_center;
}; // end class CarDrawer

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class Track{
public:
  Track(const cv::Mat3b & track) {
    track.copyTo(_img);
    cv::Mat1b track_mask;
    color_negmask(track, cv::Scalar::all(255), track_mask);
    std::vector<std::vector<cv::Point> > contours;
    cv::findContours(track_mask, contours, // a vector of contours
                     CV_RETR_EXTERNAL, // retrieve the external contours
                     //CV_RETR_LIST, // retrieve the external contours
                     CV_CHAIN_APPROX_NONE
                     //CV_CHAIN_APPROX_TC89_L1
                     ); // all pixels of each contours
    printf("contours.size():%i\n", contours.size());
    if (contours.empty()) {
      printf("Track creation failed!\n");
      return;
    }
    _track_size = track.size();
    _points = contours.front();
    _npts = _points.size();
    //  cv::Mat1b illus(_track_size, (uchar) 0);
    //  for (unsigned int pt_idx = 0; pt_idx < _npts; ++pt_idx) {
    //    illus(_points[pt_idx]) = 255;
    //    cv::imshow("illus", illus); cv::waitKey(5);
    //  } // end for pt_idx
  } // end ctor

  cv::Mat3b _img;
  cv::Size _track_size;
  std::vector<cv::Point> _points;
  unsigned int _npts;
}; // end Track

class CopyRotater {
public:
  CopyRotater() {}

  template<class Image>
  void copy(const Image & topaste, Image & dst,
            const double & angle_rad, const cv::Point & rotation_center,
            int topaste_x, int topaste_y,
            const cv::Mat* mask = NULL) {
    //paste_img()
        //rotate_image();
  }
}; // end CopyRotater

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class Game {
public:

  Game() {}
//protected:
  struct Car {
    CarDrawer _drawer;
    double _speed;
    int _position;
  };

  void draw_cars() {
    //_track._img.copyTo(_final_output);
  }

  std::vector<Car> _cars;
  //Track _track;
  cv::Mat3b _final_output;
}; // end Game

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int main(int argc, char**argv) {
  CarDrawer car1(cv::imread("../models/DSC_0780.png", CV_LOAD_IMAGE_COLOR),
           cv::Point(1211, 502), 129,
           cv::Point(182, 513), 126);
  CarDrawer car2(cv::imread("../models/DSC_0781.png", CV_LOAD_IMAGE_COLOR),
           cv::Point(1165, 501), 119,
           cv::Point(182, 494), 130);
#if 1
  cv::Mat3b out1, out2, out1_resized, out2_resized;
  double angle = 0, ang_speed = .1;
  while (true) {
    printf("angle:%g\n", angle);
    car1.rotate_wheels(angle, out1);
    cv::resize(out1, out1_resized, cv::Size(), .5, .5);
    car2.rotate_wheels(angle, out2);
    cv::resize(out2, out2_resized, cv::Size(), .5, .5);

    cv::imshow("out1", out1_resized);
    cv::imshow("out2", out2_resized);
    char c = cv::waitKey(10);
    if (c == '+') {
      ang_speed = std::min(ang_speed + .05, 1.);
    }
    else if (c == '-') {
      ang_speed = std::max(ang_speed - .05, .01);
    }
    angle -= ang_speed;
  } // end while (true)
#else
  Track track(cv::imread("../models/track.png", CV_LOAD_IMAGE_COLOR));
#endif
  return 0;
}
