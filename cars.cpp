#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <stdio.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define RAD2DEG     57.2957795130823208768  //!< to convert radians to degrees
#define DEG2RAD     0.01745329251994329577  //!< to convert degrees to radians

class Texture {
  Texture() { _tex = NULL; }
  ~Texture() { free(); }

  void free() {
    //Free texture if it exists
    if( _tex == NULL )
      return;
      SDL_DestroyTexture( _tex );
      _tex = NULL;
      mWidth = 0;
      mHeight = 0;
  } // end free()

  bool load(const std::string &str, SDL_Renderer* renderer) {
    // Load image as SDL_Surface
    SDL_Surface* surface = IMG_Load( str.c_str() );

    // SDL_Surface is just the raw pixels
    // Convert it to a hardware-optimzed texture so we can render it
    _tex = SDL_CreateTextureFromSurface( renderer, surface );
    if (_tex == NULL) {
      printf("Could not load texture '%s':'%s'\n", str.c_str(), SDL_GetError());
      return false;
    }
    //Get image dimensions
    mWidth = surface->w;
    mHeight = surface->h;

    // Don't need the orignal texture, release the memory
    SDL_FreeSurface( surface );
    return true;
  }// end load()

  bool render( SDL_Renderer* renderer, int x, int y, SDL_Rect* clip ) {
    //Set rendering space and render to screen
    SDL_Rect renderQuad = { x, y, mWidth, mHeight };

    //Set clip rendering dimensions
    if( clip != NULL ) {
      renderQuad.w = clip->w;
      renderQuad.h = clip->h;
    }

    //Render to screen
    SDL_RenderCopy( renderer, _tex, clip, &renderQuad );
  } // end render()

  inline int getWidth() { return mWidth;}
  inline int getHeight() { return mHeight;}

private:
  //The actual hardware texture
  SDL_Texture* _tex;
  //Image dimensions
  int mWidth, mHeight;
};

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
                      const cv::Mat* mask = NULL) {
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

//! read a PNG file with transparency
//! http://docs.opencv.org/2.4/modules/core/doc/operations_on_arrays.html?highlight=mixchannels#mixchannels
inline bool file2bgra(const std::string & filename,
                      cv::Mat3b & bgr,
                      cv::Mat1b & alpha) {
  cv::Mat img = cv::imread(filename, cv::IMREAD_UNCHANGED);
  if (img.channels() != 4) {
    printf("File '%s' does not have four layers!\n", filename.c_str());
    return false;
  }
  bgr.create(img.size());
  alpha.create(img.size());
  int from_to1[] = { 0,0, 1,1, 2,2 };
  cv::mixChannels(&img, 1, &bgr, 1, from_to1, 3);
  int from_to2[] = { 3,0 };
  cv::mixChannels(&img, 1, &alpha, 1, from_to2, 1);
  return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class CarDrawer {
public:
  CarDrawer(const std::string & car_filename,
            const cv::Point & front_wheel_center,
            const std::string & front_wheel_filename,
            const cv::Point & back_wheel_center,
            const std::string & back_wheel_filename) {
    file2bgra(car_filename, _car_img, _car_mask);

    _front_wheel_center = front_wheel_center;
    file2bgra(front_wheel_filename, _front_wheel_img, _front_wheel_mask);

    _back_wheel_center = back_wheel_center;
    file2bgra(back_wheel_filename, _back_wheel_img, _back_wheel_mask);
    //  cv::imshow("car", _car);
    //  cv::imshow("front_wheel_img", _front_wheel_img);
    //  cv::imshow("back_wheel_img", _back_wheel_img);
    //  cv::imshow("front_wheel_mask", _front_wheel_mask);
    //  cv::imshow("back_wheel_mask", _back_wheel_mask);
    //  cv::waitKey(0);
  }

  //////////////////////////////////////////////////////////////////////////////

  static inline cv::Point img_center(const cv::Mat & m) {
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
  Track(const std::string & track_filename) {
    cv::Mat1b track_mask;
    file2bgra(track_filename, _img, track_mask);
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
    _track_size = _img.size();
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
    //rotate_image(const cv::Mat & src, cv::Mat & dst,
    //                         const double & angle_rad, const cv::Point & rotation_center,
    //                         int flags=cv::INTER_LINEAR,
    //                         int borderMode=cv::BORDER_CONSTANT,
    //                         const cv::Scalar& borderValue=cv::Scalar())
    rotate_image(topaste, buffer, angle_rad, rotation_center);
    //    paste_img(const Image & topaste, Image & dst,
    //              int topaste_x, int topaste_y,
    //              const cv::Mat* mask = NULL,
    //              const std::string title = "",
    //              const cv::Scalar title_color = cv::Scalar(0, 0, 255))
    paste_img(buffer, dst, topaste_x, topaste_y, mask);
  }
  cv::Mat buffer;
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
  CarDrawer car1("../models/unai.png",
                 cv::Point(1211, 502), "../models/unai_front_wheel.png",
                 cv::Point(182, 513), "../models/unai_back_wheel.png");
  CarDrawer car2("../models/DSC_0781.png",
                 cv::Point(1165, 501), "../models/arnaud_front_wheel.png",
                 cv::Point(182, 494), "../models/arnaud_back_wheel.png");
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
    char c = cv::waitKey(50);
    if (c == '+') {
      ang_speed = std::min(ang_speed + .05, 1.);
    }
    else if (c == '-') {
      ang_speed = std::max(ang_speed - .05, .01);
    }
    angle -= ang_speed;
  } // end while (true)
#else
  Track track("../models/track.png");
#endif
  return 0;
}
