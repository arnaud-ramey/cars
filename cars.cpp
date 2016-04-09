#include <iostream>
#include <vector>
#include "sdl_utils.h"
#include "timer.h"

////////////////////////////////////////////////////////////////////////////////

class OrientedObject {
public:
  void load(SDL_Renderer* renderer,
            const std::string & filename,
            double scale = 1) {
    // load data
    _img.load(renderer, filename, scale);
    _angle = 0;
    // set default values
    move(Point2d(0, 0));
  } // end load()

  void move(const Point2d & center) { _center_worldpos = center; }
  void increase_angle(const double & dangle) { _angle += dangle; }
  void advance(const double & dist) {
    move(_center_worldpos + rotate(Point2d(dist, 0), _angle));
  }
  void render(SDL_Renderer* renderer) {
    _img.render2_center(renderer, _center_worldpos, NULL, _angle);
  }

protected:
  Timer _update_timer;
  Point2d _center_worldpos;
  double _angle;
  Texture _img;
}; // end class OrientedObject

class Fish : public OrientedObject {
public:
  Fish() {}

  void load(SDL_Renderer* renderer,
            const std::string & filename,
            double scale = 1) {
    OrientedObject::load(renderer, filename, scale);
    _radius = hypot(_img.getWidth(), _img.getHeight());
  }

  inline void set_speed(const double & speed) { _speed = speed; }

  //////////////////////////////////////////////////////////////////////////////

  void move_random_border(int w, int h) {
    double x = 0, y = 0, angle  = 0;
    int border = rand() % 4;
    if (border == 0) { // left
      x = -_radius+1;
      y = rand() % h;
      angle = -M_PI_2 + drand48() * M_PI;
    }
    else if (border == 1) { // up
      x = rand() % w;
      y = -_radius+1;
      angle = -M_PI + drand48() * M_PI;
    }
    else if (border == 2) { // right
      x = w + _radius-1;
      y = rand() % h;
      angle = M_PI_2 + drand48() * M_PI;
    }
    else if (border == 3) { // down
      x = rand() % w;
      y = h + _radius-1;
      angle = drand48() * M_PI;
    }
    move(Point2d(x, y));
    increase_angle(angle);
    set_speed(20 + rand() % 150);
  }

  //////////////////////////////////////////////////////////////////////////////

  void update(int w, int h) {
    advance(_speed * _update_timer.getTimeSeconds());
    move_if_gone(w, h);
    _update_timer.reset();
  }

protected:
  void move_if_gone(int w, int h) {
    if (_center_worldpos.x >= -_radius
        && _center_worldpos.x <= w+_radius
        && _center_worldpos.y >= -_radius
        && _center_worldpos.y <= h+_radius)
      return;
    //printf("move_if_gone():%s\n",_center_worldpos.to_string().c_str());
    move_random_border(w, h);
  }

  int _radius;
  double _speed;
}; // end class fish

////////////////////////////////////////////////////////////////////////////////

class Car : public OrientedObject {
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
    OrientedObject::load(renderer, car_filename, scale);
    _front_wheel_img.load(renderer, front_wheel_filename, scale);
    _back_wheel_img.load(renderer, back_wheel_filename, scale);
    // set default values
    _wheelangle = 0;
    _front_wheel_center_offset = scale * front_wheel_center;
    _back_wheel_center_offset = scale * back_wheel_center;
    move(Point2d(0, 0));
  } // end load()

  //////////////////////////////////////////////////////////////////////////////

  void advance(const double & dist) {
    OrientedObject::advance(dist);
    update_wheel_centers();
  }
  void move(const Point2d & p) {
    OrientedObject::move(p);
    update_wheel_centers();
  }
  void increase_angle(const double & dangle) {
    OrientedObject::increase_angle(dangle);
    update_wheel_centers();
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
    OrientedObject::render(renderer);
    _front_wheel_img.render2_center(renderer, _front_wheel_center_worldpos, NULL, _wheelangle);
    _back_wheel_img.render2_center(renderer, _back_wheel_center_worldpos, NULL, _wheelangle);
  }

protected:
  void update_wheel_centers() {
    _front_wheel_center_worldpos = _center_worldpos
        + rotate(_front_wheel_center_offset-_img.center(), _angle);
    _back_wheel_center_worldpos = _center_worldpos
        + rotate(_back_wheel_center_offset-_img.center(), _angle);
  }
  // wheel stuff
  double _wheelangle, _wheel_angspeed;
  Texture _front_wheel_img, _back_wheel_img;
  Point2d _front_wheel_center_offset, _back_wheel_center_offset;
  // in world coordinates
  Point2d _front_wheel_center_worldpos, _back_wheel_center_worldpos;
}; // end class Car

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class Game {
public:
  Game() {}

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
  if( !( IMG_Init( imgFlags ) & imgFlags ) ) {
    printf( "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError() );
  }
  // create window
  SDL_Rect windowRect = { 10, 10, 1200, 600 };
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
  // Set color of renderer to light blue
  SDL_SetRenderDrawColor( renderer, 150, 150, 255, 255 );
  //Game Controller 1 handler
  SDL_Joystick* _gameController = NULL;
  //Check for joysticks
  if( SDL_NumJoysticks() < 1 ) {
    printf( "Warning: No joysticks connected!\n" );
  } else {
    //Load joystick
    _gameController = SDL_JoystickOpen( 0 );
    if( _gameController == NULL )
      printf( "Warning: Unable to open game controller! SDL Error: %s\n", SDL_GetError() );
    else
      printf( "Joystick connected\n" );
  }
  //Analog joystick dead zone
  int JOYSTICK_DEAD_ZONE = 8000;

  Game game;
  game._cars.resize(2);
  Car* car1 = &(game._cars[0]);
  Car* car2 = &(game._cars[1]);
  double car_size = .12;
  car1->load(renderer, "../models/arnaud.png",
             Point2d(1165, 501), "../models/arnaud_front_wheel.png",
             Point2d(182, 494), "../models/arnaud_back_wheel.png",
             car_size);
  car2->load(renderer, "../models/unai.png",
             Point2d(1211, 502), "../models/unai_front_wheel.png",
             Point2d(182, 513), "../models/unai_back_wheel.png",
             car_size);
  car1->move(Point2d(200, 200));
  car2->move(Point2d(200, 400));
  unsigned int nfishes = 15;
  game._fishes.resize(nfishes);
  for (unsigned int var = 0; var < nfishes; ++var) {
    Fish* fish = &(game._fishes[var]);
    double size = .05 + drand48() * .3;
    std::ostringstream filename;
    filename << "../models/fish/pez"<< 1+rand()%8 << ".png";
    fish->load(renderer, filename.str(), size);
    fish->move_random_border(windowRect.w, windowRect.h);
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
      } // end SDL_KEYDOWN
      else if( event.type == SDL_JOYAXISMOTION ) {
        //Motion on controller 0
        if( event.jaxis.which == 0 ) {
          // X axis motion
          if( event.jaxis.axis == 0 ) {
            //Left of dead zone
            if( event.jaxis.value < -JOYSTICK_DEAD_ZONE ) {
              car1->advance(-10);
            }
            //Right of dead zone
            else if( event.jaxis.value > JOYSTICK_DEAD_ZONE ) {
              car1->advance(10);
            }
          }
        }
      } // end SDL_JOYAXISMOTION
    } // end while ( SDL_PollEvent( &event ) )
    game.update(windowRect.w, windowRect.h);

    SDL_RenderClear( renderer );
    game.render(renderer);
    SDL_RenderPresent( renderer);
    rate.sleep();
  } // end while (true)
  // clean
  SDL_DestroyRenderer( renderer);
  SDL_DestroyWindow( window );
  if (_gameController)
    SDL_JoystickClose( _gameController );
  SDL_Quit();
  return 0;
} // end main()
