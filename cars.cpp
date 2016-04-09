#include <iostream>
#include <vector>
#include "sdl_utils.h"
#include "timer.h"

////////////////////////////////////////////////////////////////////////////////

class Fish {
public:
  Fish() {}

  void load(SDL_Renderer* renderer,
            const std::string & fish_filename,
            double scale = 1) {
    // load data
    _fish_img.load(renderer, fish_filename, scale);
    _angle = 0;
    // set default values
    move_fish(Point2d(0, 0));
  } // end load()

  //////////////////////////////////////////////////////////////////////////////

  void move_fish(const Point2d & center) {
    _center_worldpos = center;
  }
  void set_fish_speed(const double & speed) {
    _speed = speed;
  }
  void increase_angle(const double & dangle) {
    _angle += dangle;
  }
  void advance(const double & dist) {
    move_fish(_center_worldpos + rotate(Point2d(dist, 0), _angle));
  }
  void move_fish_random_border(int w, int h) {
    double x = 0, y = 0, angle  = 0;
    int border = rand() % 4;
    if (border == 0) { // left
      x = -_fish_img.getWidth()+1;
      y = rand() % h;
      angle = -M_PI_2 + drand48() * M_PI;
    }
    else if (border == 1) { // up
      x = rand() % w;
      y = -_fish_img.getHeight()+1;
      angle = -M_PI + drand48() * M_PI;
    }
    else if (border == 2) { // right
      x = w + _fish_img.getWidth()-1;
      y = rand() % h;
      angle = M_PI_2 + drand48() * M_PI;
    }
    else if (border == 3) { // down
      x = rand() % w;
      y = h + _fish_img.getHeight()-1;
      angle = drand48() * M_PI;
    }
    move_fish(Point2d(x, y));
    increase_angle(angle);
    set_fish_speed(100);
  }

  //////////////////////////////////////////////////////////////////////////////

  void update(int w, int h) {
    advance(_speed * _update_timer.getTimeSeconds());
    move_fish_if_gone(w, h);
    _update_timer.reset();
  }
  void render(SDL_Renderer* renderer) {
    _fish_img.render2_center(renderer, _center_worldpos, NULL, _angle);
  }

protected:
  void move_fish_if_gone(int w, int h) {
    if (_center_worldpos.x >= -_fish_img.getWidth()
        && _center_worldpos.x <= w+_fish_img.getWidth()
        && _center_worldpos.y >= -_fish_img.getHeight()
        && _center_worldpos.y <= h+_fish_img.getHeight())
      return;
    printf("move_fish_if_gone():%s\n",_center_worldpos.to_string().c_str());
    move_fish_random_border(w, h);
  }

  Timer _update_timer;
  Point2d _center_worldpos;
  double _angle;
  double _speed;
  Texture _fish_img;
}; // end class fish

////////////////////////////////////////////////////////////////////////////////

class Car {
public:
  Car() {}

  void load(SDL_Renderer* renderer,
            const std::string & car_filename,
            const Point2d & front_wheel_center,
            const std::string & front_wheel_filename,
            const Point2d & back_wheel_center,
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
    set_car_center(Point2d(0, 0));
  } // end load()

  //////////////////////////////////////////////////////////////////////////////

  void set_car_center(const Point2d & p) {
    _car_center_worldpos = p;
    update_wheel_centers();
  }

  //////////////////////////////////////////////////////////////////////////////

  void increase_angle(const double & dangle) {
    _carangle += dangle;
    update_wheel_centers();
  }
  void advance(const double & dist) {
    set_car_center(_car_center_worldpos + rotate(Point2d(dist, 0), _carangle));
  }
  void set_wheel_angspeed(const double & wheel_angspeed) {
    _wheel_angspeed = wheel_angspeed;
  }
  //////////////////////////////////////////////////////////////////////////////
  void update() {
    _wheelangle += _wheel_angspeed * _update_timer.getTimeSeconds();
    _update_timer.reset();
  }

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
  Timer _update_timer;
  Point2d _car_center_worldpos;
  double _carangle;
  // wheel stuff
  double _wheelangle, _wheel_angspeed;
  Texture _car_img, _front_wheel_img, _back_wheel_img;
  Point2d _front_wheel_center_offset, _back_wheel_center_offset;
  // in world coordinates
  Point2d _front_wheel_center_worldpos, _back_wheel_center_worldpos;
}; // end class Car

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class Game {
public:

  Game() {}
  //protected:

  void update(int w, int h) {
    for (unsigned int i = 0; i < _cars.size(); ++i)
      _cars[i].update();
    for (unsigned int i = 0; i < _fishes.size(); ++i)
      _fishes[i].update(w, h);
  }

  void render(SDL_Renderer* renderer) {
    for (unsigned int i = 0; i < _cars.size(); ++i)
      _cars[i].render(renderer);
    for (unsigned int i = 0; i < _fishes.size(); ++i)
      _fishes[i].render(renderer);
  }

  std::vector<Car> _cars;
  std::vector<Fish> _fishes;
  //Track _track;
  Texture _final_output;
}; // end Game

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int main(int argc, char**argv) {
  srand(time(NULL));
  srand48(time(NULL));
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
  // Set color of renderer to grey
  SDL_SetRenderDrawColor( renderer, 50, 50, 50, 255 );

  Game game;
  game._cars.resize(2);
  Car* car1 = &(game._cars[0]);
  Car* car2 = &(game._cars[1]);
  car1->load(renderer, "../models/arnaud.png",
             Point2d(1165, 501), "../models/arnaud_front_wheel.png",
             Point2d(182, 494), "../models/arnaud_back_wheel.png",
             .2);
  car2->load(renderer, "../models/unai.png",
             Point2d(1211, 502), "../models/unai_front_wheel.png",
             Point2d(182, 513), "../models/unai_back_wheel.png",
             .2);
  car1->set_car_center(Point2d(200, 200));
  car2->set_car_center(Point2d(200, 400));
  unsigned int nfishes = 30;
  game._fishes.resize(nfishes);
  for (unsigned int var = 0; var < nfishes; ++var) {
    Fish* fish = &(game._fishes[var]);
    double size = .05 + drand48() * .3;
    std::ostringstream filename;
    filename << "../models/fish/pez"<< 1+rand()%8 << ".png";
    fish->load(renderer, filename.str(), size);
    fish->move_fish_random_border(windowRect.w, windowRect.h);
  }

  double ang_speed = 2.;
  car1->set_wheel_angspeed(ang_speed);
  car2->set_wheel_angspeed(ang_speed);
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
        if (key == SDLK_PLUS || key == SDLK_KP_PLUS) {
          ang_speed = std::min(ang_speed + .2, 10.);
          car1->set_wheel_angspeed(ang_speed);
          car2->set_wheel_angspeed(ang_speed);
        }
        else if (key == SDLK_MINUS || key == SDLK_KP_MINUS) {
          ang_speed = std::max(ang_speed - .2, .01);
          car1->set_wheel_angspeed(ang_speed);
          car2->set_wheel_angspeed(ang_speed);
        }
        else if (key == SDLK_UP)
          car1->advance(10);
        else if (key == SDLK_DOWN)
          car1->advance(-10);
        else if (key == SDLK_LEFT)
          car1->increase_angle(.1);
        else if (key == SDLK_RIGHT)
          car1->increase_angle(-.1);
      } // end while ( SDL_PollEvent( &event ) )
    } // end while(event)
    game.update(windowRect.w, windowRect.h);

    SDL_RenderClear( renderer );
    game.render(renderer);
    SDL_RenderPresent( renderer);
    rate.sleep();
  } // end while (true)
  // clean
  SDL_DestroyRenderer( renderer);
  SDL_DestroyWindow( window );
  SDL_Quit();
  return 0;
} // end main()
