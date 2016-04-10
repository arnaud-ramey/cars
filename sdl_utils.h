/*!
  \file
  \author      Arnaud Ramey <arnaud.a.ramey@gmail.com>
                -- Robotics Lab, University Carlos III of Madrid
  \date        2016/4/9

________________________________________________________________________________

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
________________________________________________________________________________

\todo Description of the file
 */
#ifndef SDL_UTILS_H
#define SDL_UTILS_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include "timer.h"
#include <sstream>
#include <vector>

#define DEBUG_PRINT(...)   {}
//#define DEBUG_PRINT(...)   printf(__VA_ARGS__)
#define RAD2DEG     57.2957795130823208768  //!< to convert radians to degrees
#define DEG2RAD     0.01745329251994329577  //!< to convert degrees to radians

#define ROTATE_COSSIN_X(x, y, cos_angle, sin_angle) \
  ((cos_angle) * (x) - (sin_angle) * (y))
#define ROTATE_COSSIN_Y(x, y, cos_angle, sin_angle) \
  ((sin_angle) * (x) + (cos_angle) * (y))
#define ROTATE_ANGLE_X(x, y, angle) \
  (ROTATE_COSSIN_X(x, y, cos(angle), sin(angle)) )
#define ROTATE_ANGLE_Y(x, y, angle) \
  (ROTATE_COSSIN_Y(x, y, cos(angle), sin(angle)) )

/*! a generic templated class for 2D points.
  It contains a x and a y field so as to be compatible
  with OpenCV Point2d*
*/
template<class _Type>
class Point2 {
public:
  typedef _Type value_type; //!< an alias for the data type (compat with OpenCV)

  //! a constructor without arguments
  Point2() : x(0), y(0) {}

  //! a constructor
  Point2(const _Type & x_, const _Type & y_) :
    x(x_), y(y_) {}

  //! the == operator, for some std algorithms
  bool operator== (const Point2<_Type>& b) const {
    return (x == b.x) && (y == b.y);
  }

  double norm() const { return hypot(x, y); }
  void renorm(const double & newnorm) {
    double currn = norm();
    if (fabs(currn) < 1E-6)
      return;
    x *= newnorm / currn;
    y *= newnorm / currn;
  }

  //! the + operator, that adds field by field
  Point2<_Type> operator + (const Point2<_Type>& B) const {
    return Point2<_Type>(x + B.x, y + B.y);
  }

  //! the - operator, that substracts field by field
  Point2<_Type> operator - (const Point2<_Type>& B) const {
    return Point2<_Type>(x - B.x, y - B.y);
  }

  //! the * operator, that multiplies field by field
  void operator *= (const double & alpha) {
    x = alpha * x;
    y = alpha * y;
  }

  //! the * operator, that multiplies field by field
  void operator += (const Point2<_Type>& B) {
    x += B.x;
    y += B.y;
  }

  //! the dot operator
  inline _Type dot(const Point2<_Type>& B) const {
    return x * B.x + y * B.y;
  }

  //! define the output to a stream
  friend std::ostream & operator << (std::ostream & stream,
                                     const Point2<_Type> & P) {
    stream << P.to_string();
    return stream;
  }

  std::string to_string() const {
    std::ostringstream out;
    out << '[' << x << ", " << y << ']';
    return out.str();
  }

  SDL_Point to_sdl() const {
    SDL_Point ans;
    ans.x = x;
    ans.y = y;
  }
  //implicit conversion
  operator SDL_Point() const { return to_sdl(); }

  //  //! return a string representation of the point
  //  inline std::string to_string() const {
  //    std::ostringstream ans;
  //    ans << *this;
  //    return ans.str();
  //  }

  _Type x; //!< the first data field
  _Type y; //!< the second data field
}; // end Point2

//! the * operator, that multiplies field by field
template<class _Type>
static Point2<_Type> operator * (double alpha, const Point2<_Type> & p) {
  return Point2<_Type>(alpha * p.x, alpha * p.y);
}

typedef Point2<int> Point2i;
typedef Point2<double> Point2d;

////////////////////////////////////////////////////////////////////////////////

inline Point2d rotate(const Point2d &pt, double angle) {
  double cosa = cos(angle), sina = sin(angle);
  Point2d ans;
  ans.x = ROTATE_COSSIN_X(pt.x, pt.y, cosa, sina);
  ans.y = ROTATE_COSSIN_Y(pt.x, pt.y, cosa, sina);
  return ans;
}

/*!
 * \brief   detect if a point is inside a polygon - return true or false
 *  http://en.wikipedia.org/wiki/Point_in_polygon#Winding_number_algorithm
     *
 * \param   p the point
 * \param   poly the polygon
 * \return  true if the point is in the polygon
 */
static inline bool point_inside_polygon(const Point2d & p,
                                        const std::vector<Point2d> & poly) {
  /*
     * algo from http://www.visibone.com/inpoly/
     */
  Point2d p_old, p_new, p1, p2;
  bool inside = false;
  int npoints = poly.size();
  if (npoints < 3) {
    return false;
  }
  p_old = poly[npoints-1];
  for (int i=0 ; i < npoints ; i++) {
    p_new = poly[i];
    if (p_new.x > p_old.x) {
      p1 = p_old;
      p2 = p_new;
    }
    else {
      p1 = p_new;
      p2 = p_old;
    }
    if ((p_new.x < p.x) == (p.x <= p_old.x)          /* edge "open" at one end */
        && 1.f * (p.y-p1.y) * (p2.x-p1.x) < 1.f * (p2.y-p1.y) * (p.x-p1.x)) {
      inside = !inside;
    }
    p_old.x = p_new.x;
    p_old.y = p_new.y;
  } // end loop i
  return(inside);
}

/// Checks if the two polygons are intersecting.
bool IsPolygonsIntersecting(std::vector<Point2d> A, std::vector<Point2d> B,
                            bool reverse_already_checked = false)
{
  for (int i1 = 0; i1 < A.size(); i1++) {
    int i2 = (i1 + 1) % A.size();
    Point2d p1 = A[i1], p2 = A[i2];

    Point2d normal(p2.y - p1.y, p1.x - p2.x);

    double UNDEF = 1E9;
    double minA = UNDEF, maxA = UNDEF;
    for (unsigned int i = 0; i < A.size(); ++i) {
      Point2d p = A[i];
      double projected = normal.x * p.x + normal.y * p.y;
      if (minA == UNDEF || projected < minA)
        minA = projected;
      if (maxA == UNDEF || projected > maxA)
        maxA = projected;
    }

    double minB = UNDEF, maxB = UNDEF;
    for (unsigned int i = 0; i < B.size(); ++i) {
      Point2d p = B[i];
      double projected = normal.x * p.x + normal.y * p.y;
      if (minB == UNDEF || projected < minB)
        minB = projected;
      if (maxB == UNDEF || projected > maxB)
        maxB = projected;
    }

    if (maxA < minB || maxB < minA)
      return false;
  }
  if (!reverse_already_checked)
    return true;
  return IsPolygonsIntersecting(B, A, false);
}

////////////////////////////////////////////////////////////////////////////////

// https://www.libsdl.org/release/SDL-1.2.15/docs/html/guidevideo.html
Uint32 getpixel(SDL_Surface *surface, int x, int y) {
  int bpp = surface->format->BytesPerPixel;
  /* Here p is the address to the pixel we want to retrieve */
  Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
  switch(bpp) {
    case 1:
      return *p;
      break;

    case 2:
      return *(Uint16 *)p;
      break;

    case 3:
      if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
        return p[0] << 16 | p[1] << 8 | p[2];
      else
        return p[0] | p[1] << 8 | p[2] << 16;
      break;

    case 4:
      return *(Uint32 *)p;
      break;

    default:
      return 0;       /* shouldn't happen, but avoids warnings */
  }
}

// https://www.libsdl.org/release/SDL-1.2.15/docs/html/guidevideo.html
void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel) {
  int bpp = surface->format->BytesPerPixel;
  /* Here p is the address to the pixel we want to set */
  Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

  switch(bpp) {
    case 1:
      *p = pixel;
      break;

    case 2:
      *(Uint16 *)p = pixel;
      break;

    case 3:
      if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
        p[0] = (pixel >> 16) & 0xff;
        p[1] = (pixel >> 8) & 0xff;
        p[2] = pixel & 0xff;
      } else {
        p[0] = pixel & 0xff;
        p[1] = (pixel >> 8) & 0xff;
        p[2] = (pixel >> 16) & 0xff;
      }
      break;

    case 4:
      *(Uint32 *)p = pixel;
      break;
  }
}

// http://www.sdltutorials.com/sdl-scale-surface
SDL_Surface *ScaleSurface(SDL_Surface *Surface, Uint16 Width, Uint16 Height)
{
  if(!Surface || !Width || !Height)
    return 0;

  SDL_Surface *_ret = SDL_CreateRGBSurface(Surface->flags, Width, Height, Surface->format->BitsPerPixel,
                                           Surface->format->Rmask, Surface->format->Gmask, Surface->format->Bmask, Surface->format->Amask);

  double    _stretch_factor_x = (static_cast<double>(Width)  / static_cast<double>(Surface->w)),
      _stretch_factor_y = (static_cast<double>(Height) / static_cast<double>(Surface->h));

  for(Sint32 y = 0; y < Surface->h; y++)
    for(Sint32 x = 0; x < Surface->w; x++)
      for(Sint32 o_y = 0; o_y < _stretch_factor_y; ++o_y)
        for(Sint32 o_x = 0; o_x < _stretch_factor_x; ++o_x)
          putpixel(_ret, static_cast<Sint32>(_stretch_factor_x * x) + o_x,
                   static_cast<Sint32>(_stretch_factor_y * y) + o_y, getpixel(Surface, x, y));
  return _ret;
}

////////////////////////////////////////////////////////////////////////////////

class Texture {
public:
  Texture() { _sdltex = NULL; _sdlsurface = NULL; _width =  _height = 0; _resize_scale = 1; }
  ~Texture() { free(); }

  void free() {
    if(!_width)
      return;
    printf("Texture::free(%ix%i)\n", _width, _height);
    _width =  _height = 0;
    _resize_scale = 1;
    //Free texture if it exists
    if (_sdltex)
      SDL_DestroyTexture( _sdltex );
    if (_sdlsurface)
      SDL_FreeSurface( _sdlsurface );
    _sdltex = NULL;
  } // end free()

  //////////////////////////////////////////////////////////////////////////////

  inline int get_width() const { return _width;}
  inline int get_height() const { return _height;}
  inline double get_resize_scale() const { return _resize_scale;}
  inline Point2d center() const  { return Point2d(get_width()/2, get_height()/2); }

  //////////////////////////////////////////////////////////////////////////////

  bool from_file(SDL_Renderer* renderer, const std::string &str,
                 int goalwidth = -1, int goalheight = -1, double goalscale = -1) {
    printf("Texture::from_file('%s'), goal:(%i, %i, %g)\n", str.c_str(), goalwidth, goalheight, goalscale);
    free();
    // Load image as SDL_Surface
    _sdlsurface = IMG_Load( str.c_str() );
    if( _sdlsurface == NULL ) {
      printf( "Unable to load image %s! SDL Error: %s\n", str.c_str(), SDL_GetError() );
      return false;
    }

    if (goalwidth <= 0 && goalheight <= 0 && goalscale <= 0)
      _resize_scale = 1;
    else {
      double scalex =  (goalwidth > 0 ? 1. * goalwidth / _sdlsurface->w : 1E6);
      double scaley =  (goalheight > 0 ? 1. * goalheight / _sdlsurface->h : 1E6);
      double scalescale =  (goalscale > 0 ? goalscale : 1E6);
      _resize_scale = std::min(scalescale, std::min(scalex, scaley));
      SDL_Surface* surface_scaled = ScaleSurface(_sdlsurface, _resize_scale * _sdlsurface->w, _resize_scale * _sdlsurface->h);
      SDL_FreeSurface( _sdlsurface );
      _sdlsurface = surface_scaled;
    }

    // SDL_Surface is just the raw pixels
    // Convert it to a hardware-optimzed texture so we can render it
    _sdltex = SDL_CreateTextureFromSurface( renderer, _sdlsurface );
    if (_sdltex == NULL) {
      printf("Could not load texture '%s':'%s'\n", str.c_str(), SDL_GetError());
      return false;
    }
    //Get image dimensions
    _width = _sdlsurface->w;
    _height = _sdlsurface->h;
    return true;
  }// end load()

  //////////////////////////////////////////////////////////////////////////////

  bool render( SDL_Renderer* renderer, Point2d p, double scale = 1, SDL_Rect* clip = NULL,
               double angle_rad = 0, Point2d center = Point2d(-1, -1),
               SDL_RendererFlip flip = SDL_FLIP_NONE) {
    //Set rendering space and render to screen
    SDL_Rect renderQuad = { p.x, p.y, scale * _width, scale * _height };

    //Set clip rendering dimensions
    if( clip != NULL ) {
      renderQuad.w = scale * clip->w;
      renderQuad.h = scale * clip->h;
    }
    //Render to screen
    if (flip == SDL_FLIP_NONE && fabs(angle_rad) < 1E-2) {
      bool ok = (SDL_RenderCopy( renderer, _sdltex, clip, &renderQuad ) == 0);
      if (!ok)
        printf("SDL_RenderCopy() returned an error '%s'!\n", SDL_GetError());
      return ok;
    }

    SDL_Point psdl = p.to_sdl();
    SDL_Point* psdl_ptr = (center.x < 0 && center.y < 0 ? NULL : &psdl);
    bool ok = (SDL_RenderCopyEx( renderer, _sdltex, clip, &renderQuad, angle_rad * RAD2DEG, psdl_ptr, flip ) == 0);
    if (!ok)
      printf("SDL_RenderCopyEx() returned an error '%s'!\n", SDL_GetError());
    return ok;
  } // end render()

  inline bool render_center( SDL_Renderer* renderer, Point2d p, double scale = 1, SDL_Rect* clip = NULL,
                             double angle_rad = 0, Point2d center = Point2d(-1, -1),
                             SDL_RendererFlip flip = SDL_FLIP_NONE) {
    return render( renderer, p - scale*this->center(), scale, clip, angle_rad, center, flip);
  } // end render_center()

  //////////////////////////////////////////////////////////////////////////////

private:
  //The actual hardware texture
  SDL_Texture* _sdltex;
  SDL_Surface* _sdlsurface;
  //Image dimensions
  int _width, _height;
  double _resize_scale;
}; // end Texture

////////////////////////////////////////////////////////////////////////////////

void render_point(SDL_Renderer* renderer, Point2d p, int thickness,
                  Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255) {
  Uint8 r0, g0, b0, a0; // get original colors of the renderer
  SDL_GetRenderDrawColor( renderer, &r0, &g0, &b0, &a0 );
  SDL_SetRenderDrawColor( renderer, r, g, b, a );
  SDL_Rect fillRect = { p.x-thickness/2, p.y-thickness/2, thickness, thickness };
  SDL_RenderFillRect( renderer, &fillRect );
  SDL_SetRenderDrawColor( renderer, r0, g0, b0, a0 );
}

bool render_line(SDL_Renderer* renderer, Point2d pt1, Point2d pt2,
                 Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255, int thickness=1) {
  Uint8 r0, g0, b0, a0; // get original colors of the renderer
  SDL_GetRenderDrawColor( renderer, &r0, &g0, &b0, &a0 );
  // https://stackoverflow.com/questions/21560384/how-to-specify-width-or-point-size-in-sdl-2-0-draw-points-lines-or-rect
  // http://www.ferzkopp.net/Software/SDL2_gfx/Docs/html/_s_d_l2__gfx_primitives_8h.html#a247136a562abec2649718d38f5819b44
  if (thickLineRGBA(renderer, pt1.x, pt1.y, pt2.x, pt2.y, thickness, r, g, b, a) != 0) {
    printf("thickLineRGBA() returned an error!\n");
    return false;
  }
  SDL_SetRenderDrawColor( renderer, r0, g0, b0, a0 );
  return true;
}

bool render_polygon(SDL_Renderer* renderer, const std::vector<Point2d> & poly,
                 Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255, int thickness=1) {
  unsigned int npts = poly.size();
  bool ok = true;
  for (unsigned int i = 0; i < npts-1; ++i)
    ok = ok && render_line(renderer, poly[i], poly[i+1], r, g, b, a, thickness);
  ok = ok && render_line(renderer, poly[0], poly[npts-1], r, g, b, a, thickness);
  return ok;
}

bool render_rect(SDL_Renderer* renderer, const SDL_Rect & rect,
                 Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255, int thickness=1) {
  std::vector<Point2d> pts;
  pts.push_back(Point2d(rect.x, rect.y));
  pts.push_back(Point2d(rect.x, rect.y+rect.h));
  pts.push_back(Point2d(rect.x+rect.w, rect.y+rect.h));
  pts.push_back(Point2d(rect.x+rect.w, rect.y));
  return render_polygon(renderer, pts, r, g, b, a, thickness);
}

////////////////////////////////////////////////////////////////////////////////

inline bool render_arrow
(SDL_Renderer* renderer, Point2d pt1, Point2d pt2,
 Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255, int thickness=1) {
  // draw the body of the arrow
  if (!render_line(renderer, pt1, pt2, r, g, b, a, thickness))
    return false;
  // compute parameters of the arrow
  double side_strokes_length = hypot(pt2.y - pt1.y, pt2.x - pt1.x) / 3;
  double arrow_orien = atan2(pt2.y - pt1.y, pt2.x - pt1.x);
  Point2d pt_end;
  // draw first side stroke
  pt_end.x  = pt2.x + side_strokes_length * cos(arrow_orien + M_PI + M_PI / 6);
  pt_end.y  = pt2.y + side_strokes_length * sin(arrow_orien + M_PI + M_PI / 6);
  if (!render_line(renderer, pt2, pt_end, r, g, b, a, thickness))
    return false;
  // draw second side stroke
  pt_end.x  = pt2.x + side_strokes_length * cos(arrow_orien + M_PI - M_PI / 6);
  pt_end.y  = pt2.y + side_strokes_length * sin(arrow_orien + M_PI - M_PI / 6);
  if (!render_line(renderer, pt2, pt_end, r, g, b, a, thickness))
    return false;
} // end draw_arrow();

////////////////////////////////////////////////////////////////////////////////

class OrientedObject {
public:
  OrientedObject() {
    _radius =  _angle = 0;
    set_position(Point2d(0, 0));
  }

  double get_update_timer() const               { return  _update_timer.getTimeSeconds(); }
  double get_life_timer()   const               { return  _life_timer.getTimeSeconds(); }
  void set_accel(const Point2d & accel)         { _accel = accel; }
  void renorm_accel(const double & newnorm)     { _accel.renorm(newnorm); }
  Point2d get_accel() const                     { return  _accel; }
  void set_speed(const Point2d & speed)         { _speed = speed; }
  void renorm_speed(const double & newnorm)     { _speed.renorm(newnorm); }
  Point2d get_speed() const                     { return  _speed; }
  void set_tan_nor_speed(const Point2d & speed) { _speed = rotate(speed, _angle); }
  void set_position(const Point2d & position)   { _position = position; }
  Point2d get_position() const                  { return  _position; }
  void increase_angle(const double & dangle)    { _angle += dangle; }
  void advance(const double & dist) {
    set_position(_position + rotate(Point2d(dist, 0), _angle));
  }
  void update_pos_speed() {
    Timer::Time time = _update_timer.getTimeSeconds();
    _speed += time * _accel;
    _position += time * _speed;
    _update_timer.reset();
  }
  void set_radius(const double & radius)         { _radius = radius; }
  double get_radius() const                      { return  _radius; }

  bool is_visible(int winw, int winh) {
    return (_position.x >= -_radius
            && _position.x <= winw+_radius
            && _position.y >= -_radius
            && _position.y <= winh+_radius);
  }

protected:
  Timer _life_timer, _update_timer;
  Point2d _position, _accel, _speed;
  double _angle, _radius;
}; // end class OrientedObject

////////////////////////////////////////////////////////////////////////////////

class OrientedTexture : public OrientedObject {
public:
  bool from_file(SDL_Renderer* renderer,
                 const std::string & filename,
                 int goalwidth = -1, int goalheight = -1, double goalscale = -1) {
    // load data
    if (!_tex.from_file(renderer, filename, goalwidth, goalheight, goalscale))
      return false;
    _radius = hypot(_tex.get_width(), _tex.get_height()) / 2;
    _bbox_offset.resize(4);
    _bbox_offset[0] = Point2d(0, 0);
    _bbox_offset[1] = Point2d(0, _tex.get_height());
    _bbox_offset[2] = Point2d(_tex.get_width(), _tex.get_height()/2);
    _bbox_offset[3] = Point2d(_tex.get_width(), 0);
    return true;
  } // end from_file()

  bool render(SDL_Renderer* renderer) {
    if (!_tex.render_center(renderer, _position, 1, NULL, _angle))
      return false;
    //render_point(renderer, _position, 3, 255, 0, 0, 255);
    render_arrow(renderer, _position, _position + _speed, 255, 0, 0, 255, 3);
    render_arrow(renderer, _position, _position + _accel, 0, 255, 0, 255, 2);
    SDL_Rect rb;
    rough_bbox(rb);
    render_rect(renderer, rb, 255, 0, 0, 255, 5);
//    compute_tight_bbox(_bbox);
//    render_polygon(renderer, _bbox, 255, 0, 0, 255, 5);
    return true;
  }

  inline Point2d offset2world_pos(const Point2d & p) const {
    return _position + rotate(p - _tex.center(), _angle);
  }

  inline bool bbox_contains_point(const Point2d & p) {
    return (point_inside_polygon(p, _bbox_offset));
  }

  inline void rough_bbox(SDL_Rect & bbox) const {
    bbox.x = _position.x - _radius;
    bbox.y = _position.y - _radius;
    bbox.w = 2 * _radius;
    bbox.h = 2 * _radius;
  }
  inline void compute_tight_bbox(std::vector<Point2d> & tight_bbox) const {
    tight_bbox.resize(4);
    for (unsigned int i = 0; i < _bbox_offset.size(); ++i)
      tight_bbox[i] == offset2world_pos(_bbox_offset[i]);
  }

  inline bool collides_with(const OrientedTexture & b) const {
    // rough radius check
    if ((_position-b._position).norm() > _radius + b._radius)
      return false;
    // tight bbox check
    std::vector<Point2d> aP, bP;
    compute_tight_bbox(aP);
    b.compute_tight_bbox(bP);
    if (!IsPolygonsIntersecting(aP, bP))
      return false;
    // http://www.sdltutorials.com/sdl-per-pixel-collision
    return true;
  }

  //protected:
  Texture _tex;
  std::vector<Point2d> _bbox_offset, _bbox;
}; // end class OrientedTexture

#endif // SDL_UTILS_H

