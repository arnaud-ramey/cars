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
  with OpenCV cv::Point*
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

  //! the + operator, that adds field by field
  Point2<_Type> operator + (const Point2<_Type>& B) const {
    return Point2<_Type>(x + B.x, y + B.y);
  }

  //! the - operator, that substracts field by field
  Point2<_Type> operator - (const Point2<_Type>& B) const {
    return Point2<_Type>(x - B.x, y - B.y);
  }

  //! the * operator, that multiplies field by field
  void operator *= (const double & alpha) const {
    x = alpha * x;
    y = alpha * y;
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


inline Point2d rotate(const Point2d &pt, double angle) {
  double cosa = cos(angle), sina = sin(angle);
  Point2d ans;
  ans.x = ROTATE_COSSIN_X(pt.x, pt.y, cosa, sina);
  ans.y = ROTATE_COSSIN_Y(pt.x, pt.y, cosa, sina);
  return ans;
}

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

void render_point(SDL_Renderer* renderer, Point2d p, int w,
                  Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255) {
  SDL_Rect fillRect = { p.x-w/2, p.y-w/2, w, w };
  SDL_SetRenderDrawColor( renderer, r, g, b, a );
  SDL_RenderFillRect( renderer, &fillRect );
  SDL_SetRenderDrawColor( renderer, 0, 0, 0, 255 );
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


class Texture {
public:
  Texture() { _tex = NULL; mWidth = 0; mHeight = 0; }
  ~Texture() { free(); }

  void free() {
    if( _tex == NULL )
      return;
    printf("Texture::free(%ix%i)\n", mWidth, mHeight);
    mWidth = 0;
    mHeight = 0;
    //Free texture if it exists
    SDL_DestroyTexture( _tex );
    _tex = NULL;
  } // end free()

  //////////////////////////////////////////////////////////////////////////////

  inline int getWidth() const { return mWidth;}
  inline int getHeight() const { return mHeight;}
  inline Point2d center() const  { return Point2d(getWidth()/2, getHeight()/2); }
  inline Point2i centeri() const  { return Point2i(getWidth()/2, getHeight()/2); }

  //////////////////////////////////////////////////////////////////////////////

  bool load(SDL_Renderer* renderer, const std::string &str,
            double scale = 1) {
    free();
    // Load image as SDL_Surface
    SDL_Surface* surface = IMG_Load( str.c_str() );

    if (fabs(scale - 1) > 1E-2)
      surface = ScaleSurface(surface, scale * surface->w, scale * surface->h);

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

  //////////////////////////////////////////////////////////////////////////////

  bool render( SDL_Renderer* renderer, Point2d p, SDL_Rect* clip = NULL) {
    //Set rendering space and render to screen
    SDL_Rect renderQuad = { p.x, p.y, mWidth, mHeight };

    //Set clip rendering dimensions
    if( clip != NULL ) {
      renderQuad.w = clip->w;
      renderQuad.h = clip->h;
    }

    //Render to screen
    return(SDL_RenderCopy( renderer, _tex, clip, &renderQuad ) == 0);
  } // end render()

  bool render_center( SDL_Renderer* renderer, Point2d p, SDL_Rect* clip = NULL) {
    return render( renderer, p-this->center(), clip);
  } // end render_center()

  //////////////////////////////////////////////////////////////////////////////

  bool render2( SDL_Renderer* renderer, Point2d p, SDL_Rect* clip,
                double angle_rad, Point2d center = Point2d(-1, -1), SDL_RendererFlip flip = SDL_FLIP_NONE) {
    //Set rendering space and render to screen
    SDL_Rect renderQuad = { p.x, p.y, mWidth, mHeight };

    //Set clip rendering dimensions
    if( clip != NULL ) {
      renderQuad.w = clip->w;
      renderQuad.h = clip->h;
    }
    //Render to screen
    SDL_Point psdl = p.to_sdl();
    SDL_Point* psdl_ptr = (center.x < 0 && center.y < 0 ? NULL : &psdl);
    return (SDL_RenderCopyEx( renderer, _tex, clip, &renderQuad, angle_rad * RAD2DEG, psdl_ptr, flip ) == 0);
  } // end render()

  bool render2_center( SDL_Renderer* renderer, Point2d p, SDL_Rect* clip,
                       double angle_rad, Point2d center = Point2d(-1, -1), SDL_RendererFlip flip = SDL_FLIP_NONE) {
    return render2( renderer, p-this->center(), clip, angle_rad, center, flip);
  } // end render2_center()

  //////////////////////////////////////////////////////////////////////////////

private:
  //The actual hardware texture
  SDL_Texture* _tex;
  //Image dimensions
  int mWidth, mHeight;
};

#endif // SDL_UTILS_H

