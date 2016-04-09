#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include "timer.h"

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

inline SDL_Point SDL_Point_ctor(int x, int y) { SDL_Point p; p.x = x; p.y = y; return p; }
inline SDL_Point operator + (const SDL_Point &a, const SDL_Point &b) {
  return SDL_Point_ctor(a.x+b.x,a.y+b.y);
}
inline SDL_Point operator - (const SDL_Point &a) {
  return SDL_Point_ctor(-a.x,-a.y);
}
inline SDL_Point operator - (const SDL_Point &a, const SDL_Point &b) {
  return SDL_Point_ctor(a.x-b.x,a.y-b.y);
}
inline SDL_Point operator * (const double &k, const SDL_Point &a) {
  return SDL_Point_ctor(k*a.x,k*a.y);
}
inline SDL_Point rotate(const SDL_Point &pt, double angle) {
  double cosa = cos(angle), sina = sin(angle);
  SDL_Point ans;
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

void render_point(SDL_Renderer* renderer, SDL_Point p, int w,
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
  Texture() { _tex = NULL; free(); }
  ~Texture() { free(); }

  void free() {
    mWidth = 0;
    mHeight = 0;
    //Free texture if it exists
    if( _tex == NULL )
      return;
    SDL_DestroyTexture( _tex );
    _tex = NULL;
  } // end free()

  //////////////////////////////////////////////////////////////////////////////

  inline int getWidth() const { return mWidth;}
  inline int getHeight() const { return mHeight;}
  inline SDL_Point center() const  { return SDL_Point_ctor(getWidth()/2, getHeight()/2); }

  //////////////////////////////////////////////////////////////////////////////

  bool load(SDL_Renderer* renderer, const std::string &str,
            double scale = 1) {
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

  bool render( SDL_Renderer* renderer, SDL_Point p, SDL_Rect* clip = NULL) {
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

  bool render_center( SDL_Renderer* renderer, SDL_Point p, SDL_Rect* clip = NULL) {
    return render( renderer, p-this->center(), clip);
  } // end render_center()

  //////////////////////////////////////////////////////////////////////////////

  bool render2( SDL_Renderer* renderer, SDL_Point p, SDL_Rect* clip,
                double angle, SDL_Point* center = NULL, SDL_RendererFlip flip = SDL_FLIP_NONE) {
    //Set rendering space and render to screen
    SDL_Rect renderQuad = { p.x, p.y, mWidth, mHeight };

    //Set clip rendering dimensions
    if( clip != NULL ) {
      renderQuad.w = clip->w;
      renderQuad.h = clip->h;
    }
    //Render to screen
    return (SDL_RenderCopyEx( renderer, _tex, clip, &renderQuad, angle * RAD2DEG, center, flip ) == 0);
  } // end render()

  bool render2_center( SDL_Renderer* renderer, SDL_Point p, SDL_Rect* clip,
                       double angle, SDL_Point* center = NULL, SDL_RendererFlip flip = SDL_FLIP_NONE) {
    return render2( renderer, p-this->center(), clip, angle, center, flip);
  } // end render2_center()

  //////////////////////////////////////////////////////////////////////////////

private:
  //The actual hardware texture
  SDL_Texture* _tex;
  //Image dimensions
  int mWidth, mHeight;
};

////////////////////////////////////////////////////////////////////////////////

class CarDrawer {
public:
  CarDrawer(SDL_Renderer* renderer,
            const std::string & car_filename,
            const SDL_Point & front_wheel_center,
            const std::string & front_wheel_filename,
            const SDL_Point & back_wheel_center,
            const std::string & back_wheel_filename,
            double scale = 1) {
    // load data
    _car_img.load(renderer, car_filename, scale);
    _front_wheel_img.load(renderer, front_wheel_filename, scale);
    _back_wheel_img.load(renderer, back_wheel_filename, scale);
    // set default values
    _wheelangle = 0;
    _carangle = 0;
    _front_wheel_center_offset = scale * front_wheel_center;
    _back_wheel_center_offset = scale * back_wheel_center;
    set_car_center(SDL_Point_ctor(0, 0));
  }

  //////////////////////////////////////////////////////////////////////////////

  void set_car_center(const SDL_Point & p) {
    _car_center_worldpos = p;
    update_wheel_centers();
  }

  //////////////////////////////////////////////////////////////////////////////

  void increase_angle(const double & dangle) {
    _carangle += dangle;
    update_wheel_centers();
  }
  void advance(const double & dist) {
    set_car_center(_car_center_worldpos + rotate(SDL_Point_ctor(dist, 0), _carangle));
  }
  void rotate_wheels(const double & angspeed) {
    _wheelangle += angspeed;
    update_wheel_centers();
  }
  //////////////////////////////////////////////////////////////////////////////

  void render(SDL_Renderer* renderer) {
    //_car_img.render_center(renderer, _car_center_worldpos);
    _car_img.render2_center(renderer, _car_center_worldpos, NULL, _carangle);
    _front_wheel_img.render2_center(renderer, _front_wheel_center_worldpos, NULL, _wheelangle);
    _back_wheel_img.render2_center(renderer, _back_wheel_center_worldpos, NULL, _wheelangle);
  }

protected:
  void update_wheel_centers() {
    _front_wheel_center_worldpos = _car_center_worldpos
        + rotate(_front_wheel_center_offset-_car_img.center(), _carangle);
    _back_wheel_center_worldpos = _car_center_worldpos
        + rotate(_back_wheel_center_offset-_car_img.center(), _carangle);
  }
  // car stuff
  SDL_Point _car_center_worldpos;
  double _carangle;
  // wheel stuff
  double _wheelangle;
  Texture _car_img, _front_wheel_img, _back_wheel_img;
  SDL_Point _front_wheel_center_offset, _back_wheel_center_offset;
  SDL_Point _front_wheel_center_worldpos, _back_wheel_center_worldpos;
  // in image coordinates
}; // end class CarDrawer

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class Game {
public:

  Game() {}
  //protected:
  struct Car {
    CarDrawer _drawer;
    double _speed;
    int _center_worldposition;
  };

  void draw_cars() {
    //_track._img.copyTo(_final_output);
  }

  std::vector<Car> _cars;
  //Track _track;
  Texture _final_output;
}; // end Game

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int main(int argc, char**argv) {
  if ( SDL_Init( SDL_INIT_EVERYTHING ) == -1 ) {
    std::cout << " Failed to initialize SDL : " << SDL_GetError() << std::endl;
    return false;
  }
  //Initialize PNG loading
  int imgFlags = IMG_INIT_PNG;
  if( !( IMG_Init( imgFlags ) & imgFlags ) )
  {
    printf( "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError() );
  }
  // create window
  SDL_Rect windowRect = { 10, 10, 800, 800 };
  SDL_Window* window = SDL_CreateWindow( "Server", windowRect.x, windowRect.y, windowRect.w, windowRect.h, 0 );
  if ( window == NULL ) {
    std::cout << "Failed to create window : " << SDL_GetError();
    return false;
  }
  // create renderer
  SDL_Renderer* renderer = SDL_CreateRenderer( window, -1, 0 );
  if ( renderer == NULL ) {
    std::cout << "Failed to create renderer : " << SDL_GetError();
    return false;
  }
  // Set size of renderer to the same as window
  SDL_RenderSetLogicalSize( renderer, windowRect.w, windowRect.h );
  // Set color of renderer to red
  SDL_SetRenderDrawColor( renderer, 50, 50, 50, 255 );

  CarDrawer car1(renderer, "../models/arnaud.png",
                 SDL_Point_ctor(1165, 501), "../models/arnaud_front_wheel.png",
                 SDL_Point_ctor(182, 494), "../models/arnaud_back_wheel.png",
                 .2);
  CarDrawer car2(renderer, "../models/unai.png",
                 SDL_Point_ctor(1211, 502), "../models/unai_front_wheel.png",
                 SDL_Point_ctor(182, 513), "../models/unai_back_wheel.png",
                 .2);
  car1.set_car_center(SDL_Point_ctor(200, 200));
  car2.set_car_center(SDL_Point_ctor(200, 400));

  double ang_speed = .3;
  bool loop = true;
  Rate rate(20);
  //Start counting frames per second
  while (loop) {
    SDL_Event event;
    while ( SDL_PollEvent( &event ) ) {
      if ( event.type == SDL_QUIT )
        loop = false;
      else if ( event.type == SDL_KEYDOWN ) {
        SDL_Keycode key = event.key.keysym.sym;
        if (key == SDLK_PLUS || key == SDLK_KP_PLUS)
          ang_speed = std::min(ang_speed + .05, 1.);
        else if (key == SDLK_MINUS || key == SDLK_KP_MINUS)
          ang_speed = std::max(ang_speed - .05, .01);
        else if (key == SDLK_UP)
          car1.advance(10);
        else if (key == SDLK_DOWN)
          car1.advance(-10);
        else if (key == SDLK_LEFT)
          car1.increase_angle(.1);
        else if (key == SDLK_RIGHT)
          car1.increase_angle(-.1);
      } // end while ( SDL_PollEvent( &event ) )
    } // end while(event)
    car1.rotate_wheels(ang_speed);
    car2.rotate_wheels(ang_speed);

    SDL_RenderClear( renderer );
    car1.render(renderer);
    car2.render(renderer);
    SDL_RenderPresent( renderer);
    // Add a 16msec delay to make our game run at ~60 fps
    //SDL_Delay( 50 );
    rate.sleep();
  } // end while (true)
  // clean
  SDL_DestroyRenderer( renderer);
  SDL_DestroyWindow( window );
  SDL_Quit();
  return 0;
} // end main()
