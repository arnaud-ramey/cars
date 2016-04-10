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
  Texture() { _sdltex = NULL; mWidth = 0; mHeight = 0; }
  ~Texture() { free(); }

  void free() {
    if( _sdltex == NULL )
      return;
    printf("Texture::free(%ix%i)\n", mWidth, mHeight);
    mWidth = 0;
    mHeight = 0;
    //Free texture if it exists
    SDL_DestroyTexture( _sdltex );
    _sdltex = NULL;
  } // end free()

  //////////////////////////////////////////////////////////////////////////////

  inline int getWidth() const { return mWidth;}
  inline int getHeight() const { return mHeight;}
  inline Point2d center() const  { return Point2d(getWidth()/2, getHeight()/2); }
  inline Point2i centeri() const  { return Point2i(getWidth()/2, getHeight()/2); }

  //////////////////////////////////////////////////////////////////////////////

  bool from_file(SDL_Renderer* renderer, const std::string &str,
                 double scale = 1) {
    free();
    // Load image as SDL_Surface
    SDL_Surface* surface = IMG_Load( str.c_str() );
    if( surface == NULL ) {
      printf( "Unable to load image %s! SDL Error: %s\n", str.c_str(), SDL_GetError() );
      return false;
    }

    if (fabs(scale - 1) > 1E-2)
      surface = ScaleSurface(surface, scale * surface->w, scale * surface->h);

    // SDL_Surface is just the raw pixels
    // Convert it to a hardware-optimzed texture so we can render it
    _sdltex = SDL_CreateTextureFromSurface( renderer, surface );
    if (_sdltex == NULL) {
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

  //////////////////////////////////////////////////////////////////////////////

  bool render( SDL_Renderer* renderer, Point2d p, double scale = 1, SDL_Rect* clip = NULL,
               double angle_rad = 0, Point2d center = Point2d(-1, -1),
               SDL_RendererFlip flip = SDL_FLIP_NONE) {
    //Set rendering space and render to screen
    SDL_Rect renderQuad = { p.x, p.y, scale * mWidth, scale * mHeight };

    //Set clip rendering dimensions
    if( clip != NULL ) {
      renderQuad.w = scale * clip->w;
      renderQuad.h = scale * clip->h;
    }
    //Render to screen
    if (flip == SDL_FLIP_NONE && fabs(angle_rad) < 1E-2)
      return(SDL_RenderCopy( renderer, _sdltex, clip, &renderQuad ) == 0);

    SDL_Point psdl = p.to_sdl();
    SDL_Point* psdl_ptr = (center.x < 0 && center.y < 0 ? NULL : &psdl);
    return (SDL_RenderCopyEx( renderer, _sdltex, clip, &renderQuad, angle_rad * RAD2DEG, psdl_ptr, flip ) == 0);
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
  //Image dimensions
  int mWidth, mHeight;
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
};

class OrientedTexture : public OrientedObject {
public:
  void load(SDL_Renderer* renderer,
            const std::string & filename,
            double scale = 1) {
    // load data
    _tex.from_file(renderer, filename, scale);
    _radius = hypot(_tex.getWidth(), _tex.getHeight()) / 2;
    // set default values
  } // end load()

  bool render(SDL_Renderer* renderer) {
    if (!_tex.render_center(renderer, _position, 1, NULL, _angle))
      return false;
    //render_point(renderer, _position, 3, 255, 0, 0, 255);
    render_arrow(renderer, _position, _position + _speed, 255, 0, 0, 255, 3);
    render_arrow(renderer, _position, _position + _accel, 0, 255, 0, 255, 2);
    return true;
  }

  inline Point2d offset2world_pos(const Point2d & p) const {
    return _position + rotate(p - _tex.center(), _angle);
  }

  //protected:
  Texture _tex;
}; // end class OrientedObject

#endif // SDL_UTILS_H

