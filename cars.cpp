#include <iostream>
#include "sdl_utils.h"

class Fish : public OrientedTexture {
public:
  void move_random_border(int winw, int winh) {
    double x = 0, y = 0, angle  = 0;
    int border = rand() % 4;
    if (border == 0) { // left
      x = -_radius+1;
      y = rand() % winh;
      angle = -M_PI_2 + drand48() * M_PI;
    }
    else if (border == 1) { // up
      x = rand() % winw;
      y = -_radius+1;
      angle = -M_PI + drand48() * M_PI;
    }
    else if (border == 2) { // right
      x = winw + _radius-1;
      y = rand() % winh;
      angle = M_PI_2 + drand48() * M_PI;
    }
    else if (border == 3) { // down
      x = rand() % winw;
      y = winh + _radius-1;
      angle = drand48() * M_PI;
    }
    set_position(Point2d(x, y));
    increase_angle(angle);
    set_tan_nor_speed(Point2d(20 + rand() % 150, 0));
  }

  //////////////////////////////////////////////////////////////////////////////

  void update(int winw, int winh) {
    if (!is_visible(winw, winh))
      move_random_border(winw, winh);
    OrientedTexture::update_pos_speed();
  }

protected:
}; // end class Fish

////////////////////////////////////////////////////////////////////////////////

class BubbleManager {
public:
  bool from_file(SDL_Renderer* renderer, const std::string &str) {
    return _bubble_tex.from_file(renderer, str, 100);
  }
  void create_bubble(const Point2d & pos, unsigned int radius_px) {
    _bubbles.push_back(OrientedObject());
    _bubbles.back().set_position(pos);
    _bubbles.back().set_radius(radius_px);
    _bubbles.back().set_speed(Point2d(0, -30 - rand()%100));
  }

  void update(int winw, int winh) {
    for (unsigned int i = 0; i < _bubbles.size(); ++i) {
      OrientedObject* b = &(_bubbles[i]);
      double speedx = 100*sin(3*b->get_life_timer()+b->get_radius());
      b->set_speed( Point2d( speedx, b->get_speed().y));// make bubble oscillate
      b->update_pos_speed();
      if (!b->is_visible(winw, winh)) {
        _bubbles.erase(_bubbles.begin() + i);
        --i;
      }
    }
  } // end update()

  bool render(SDL_Renderer* renderer) {
    DEBUG_PRINT("BubbleManager::render()\n");
    bool ok = true;
    for (unsigned int i = 0; i < _bubbles.size(); ++i) {
      ok = ok &&
          _bubble_tex.render_center(renderer,
                                    _bubbles[i].get_position(),
                                    1. * _bubbles[i].get_radius() / _bubble_tex.get_width());
    }
    return ok;
  }

  Texture _bubble_tex;
  std::vector<OrientedObject> _bubbles;
}; // end class BubbleManager

////////////////////////////////////////////////////////////////////////////////

class Car : public OrientedTexture {
public:
  bool from_file(SDL_Renderer* renderer,
            const std::string & car_filename,
            const Point2d & front_wheel_center_offset,
            const std::string & front_wheel_filename,
            const Point2d & back_wheel_center_offset,
            const std::string & back_wheel_filename,
            const Point2d & exhaust_pipe_offset,
            int goalwidth = -1, int goalheight = -1, double goalscale = -1) {
    // load data
    if (!OrientedTexture::from_file(renderer, car_filename, goalwidth, goalheight, goalscale)
        || !_front_wheel_img.from_file(renderer, front_wheel_filename, -1, -1, _tex.get_resize_scale())
        || !_back_wheel_img.from_file(renderer, back_wheel_filename, -1, -1, _tex.get_resize_scale())) {
      printf("Car::load() failed\n");
      return false;
    }
    // set default values
    _wheelangle = 0;
    _front_wheel_center_offset = _tex.get_resize_scale() * front_wheel_center_offset;
    _back_wheel_center_offset  = _tex.get_resize_scale() * back_wheel_center_offset;
    _exhaust_pipe_offset       = _tex.get_resize_scale() * exhaust_pipe_offset;
    set_position(Point2d(0, 0));
    return true;
  } // end load()

  //////////////////////////////////////////////////////////////////////////////

  void set_wheel_angspeed(const double & wheel_angspeed) {
    _wheel_angspeed = wheel_angspeed;
  }

  //////////////////////////////////////////////////////////////////////////////

  void update(int winw, int winh, BubbleManager* bubble_gen) {
    // turn wheels faster if car faster
    _wheel_angspeed = hypot(_speed.y, _speed.x) / 10;
    _wheelangle += _wheel_angspeed * _update_timer.getTimeSeconds();
    // orientate car in direction of speed
    if (_speed.norm() > 10)
      _angle = atan2(_speed.y, _speed.x);
    OrientedTexture::update_pos_speed();
    // stop if going out of the screen
    if (_position.x < _radius) { // left
      _accel.x = std::max(_accel.x + 10, 20.);
      if (_speed.norm() > 100) _speed.renorm(100);
    }
    else if (_position.x > winw - _radius) { // right
      _accel.x = std::min(_accel.x - 10, -20.);
      if (_speed.norm() > 100) _speed.renorm(100);
    }
    if (_position.y < _radius) { // up
      _accel.y = std::max(_accel.y + 10, 20.);
      if (_speed.norm() > 100) _speed.renorm(100);
    }
    else if (_position.y > winh - _radius) { // down
      _accel.y = std::min(_accel.y - 10, -20.);
      if (_speed.norm() > 100) _speed.renorm(100);
    }
    // create bubbles randomly or if accelerating
    if ((rand() % 2000 + _accel.norm()) > 1950) {
      Point2d ex = offset2world_pos(_exhaust_pipe_offset);
      bubble_gen->create_bubble(ex, 5 + rand() % 25 + _accel.norm() / 20); // bigger if accelerating
    }
  }

  //////////////////////////////////////////////////////////////////////////////

  bool render(SDL_Renderer* renderer) {
    DEBUG_PRINT("Car::render()\n");
    return
        OrientedTexture::render(renderer)
        && _front_wheel_img.render_center(renderer, offset2world_pos(_front_wheel_center_offset),  1, NULL, _wheelangle)
        && _back_wheel_img.render_center(renderer, offset2world_pos(_back_wheel_center_offset), 1, NULL, _wheelangle);
  }

protected:
  // wheel stuff
  double _wheelangle, _wheel_angspeed;
  Texture _front_wheel_img, _back_wheel_img;
  Point2d _front_wheel_center_offset, _back_wheel_center_offset, _exhaust_pipe_offset;
}; // end class Car


////////////////////////////////////////////////////////////////////////////////

class CandyManager {
public:
  bool add_file(SDL_Renderer* renderer, const std::string &str, int goalwidth = -1) {
    _candy_textures.resize(_candy_textures.size() + 1);
    _candy_textures.back().from_file(renderer, str, goalwidth);
    _candy_textures.back().set_position(Point2d(-1, -1));
    _curr_candy = &(_candy_textures.back());
    return true;
  }

  bool respawn(int winw, int winh){
    if (_candy_textures.empty()) {
      printf("Cannot render candy without texture!\n");
      return false;
    }
    _curr_idx = rand() % _candy_textures.size();
    _curr_candy = &(_candy_textures[_curr_idx]);
    _curr_candy->set_position(Point2d(rand()% winw, rand()%winh));
    return true;
  }

  bool update(int winw, int winh) {
    if (_curr_candy->get_position().x < 0)
      return respawn(winw, winh);
    return true;
  } // end update()

  bool render(SDL_Renderer* renderer) {
    DEBUG_PRINT("CandyManager::render()\n");
    if (_curr_idx >= _candy_textures.size())
      return false;
    return _curr_candy->render(renderer);
  }

  std::vector<OrientedTexture> _candy_textures;
  OrientedTexture* _curr_candy;
  unsigned int _curr_idx;
}; // end class CandyManager

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class Game {
public:
  bool init() {
    unsigned int nfishes = 15;
    unsigned int car_width = 200; // px
    unsigned int candy_width = 100; // px
    winw = 1200, winh  = 600; // pixels

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
    SDL_Rect windowRect = { 10, 10, winw, winh};
    window = SDL_CreateWindow( "Server", windowRect.x, windowRect.y, winw, winh, 0 );
    if ( window == NULL ) {
      std::cout << "Failed to create window : " << SDL_GetError();
      return false;
    }
    // create renderer
    renderer = SDL_CreateRenderer( window, -1, 0 );
    if ( renderer == NULL ) {
      std::cout << "Failed to create renderer : " << SDL_GetError();
      return false;
    }
    // Set size of renderer to the same as window
    SDL_RenderSetLogicalSize( renderer, winw, winh );
    // Set color of renderer to light blue
    SDL_SetRenderDrawColor( renderer, 150, 150, 255, 255 );
    //Check for joysticks
    gameControllers.resize(SDL_NumJoysticks());
    for (unsigned int i = 0; i < SDL_NumJoysticks(); ++i) {
      gameControllers[i] = SDL_JoystickOpen( i );
      if(gameControllers[i] == NULL )
        printf( "Warning: Unable to open game controller! SDL Error: %s\n", SDL_GetError() );
      else
        DEBUG_PRINT( "Joystick %i connected\n", i);
    }

    std::string base_path = SDL_GetBasePath(), model_path = base_path + "../models/";
    DEBUG_PRINT("base_path:'%s'\n", base_path.c_str());
    // init bubble manager
    _bubble_man.from_file(renderer, model_path + "bubble.png");
    for (unsigned int i = 0; i < 10; ++i)
      _bubble_man.create_bubble(Point2d(rand()% winw, rand() % winh), 10 + rand()%100);
    // init candy
    _candy_man.add_file(renderer, model_path + "fish/chuche1.png", candy_width);
    // init cars
    _cars.resize(2);
    if (!_cars[0].from_file(renderer, model_path + "arnaud.png",
                            Point2d(1165, 501), model_path + "arnaud_front_wheel.png",
                            Point2d(182, 494), model_path + "arnaud_back_wheel.png",
                            Point2d(9, 469),
                            car_width)
        || !_cars[1].from_file(renderer, model_path + "unai.png",
                               Point2d(1211, 502), model_path + "unai_front_wheel.png",
                               Point2d(182, 513), model_path + "unai_back_wheel.png",
                               Point2d(7, 484),
                               car_width))
      return false;
    _cars[0].set_position(Point2d(200, 200));
    _cars[1].set_position(Point2d(200, 400));
    // init fishes
    _fishes.resize(nfishes);
    for (unsigned int var = 0; var < nfishes; ++var) {
      Fish* fish = &(_fishes[var]);
      int fish_size = 100; // px
      std::ostringstream filename;
      filename << model_path + "fish/pez"<< 1+rand()%8 << ".png";
      fish->from_file(renderer, filename.str(), fish_size);
      fish->move_random_border(winw, winh);
    }
    return true;
  } // end init()

  //////////////////////////////////////////////////////////////////////////////

  bool clean() {
    DEBUG_PRINT("Game::clean()\n");
    SDL_DestroyRenderer( renderer);
    SDL_DestroyWindow( window );
    for (unsigned int i = 0; i < gameControllers.size(); ++i)
      SDL_JoystickClose( gameControllers[i] );
    SDL_Quit();
    return true;
  }
  //////////////////////////////////////////////////////////////////////////////

  bool update() {
    DEBUG_PRINT("Game::update()\n");
    SDL_Event event;
    while ( SDL_PollEvent( &event ) ) {
      if ( event.type == SDL_QUIT )
        return false;
      else if ( event.type == SDL_KEYDOWN ) {
        SDL_Keycode key = event.key.keysym.sym;
        if (key == SDLK_q)
          return false;
        else if (key == SDLK_UP)
          _cars.front().advance(10);
        else if (key == SDLK_DOWN)
          _cars.front().advance(-10);
        else if (key == SDLK_LEFT)
          _cars.front().increase_angle(.1);
        else if (key == SDLK_RIGHT)
          _cars.front().increase_angle(-.1);
      } // end SDL_KEYDOWN
      else if( event.type == SDL_JOYAXISMOTION ) {
        //Motion on controller 0
        if( event.jaxis.which <= _cars.size() ) {
          Car* car = &(_cars[event.jaxis.which]);
#if 1 // control car accelerations
          Point2d accel = car->get_accel();
          if( event.jaxis.axis == 0 ) // X axis motion
            car->set_accel(Point2d(event.jaxis.value / 50, accel.y));
          else if( event.jaxis.axis == 1)
            car->set_accel(Point2d(accel.x, event.jaxis.value / 50));
#else // control car speeds
          Point2d speed = car->get_speed();
          if( event.jaxis.axis == 0 ) // X axis motion
            car->set_speed(Point2d(event.jaxis.value / 50, speed.y));
          else if( event.jaxis.axis == 1)
            car->set_speed(Point2d(speed.x, event.jaxis.value / 50));
#endif
        }
      } // end SDL_JOYAXISMOTION
    } // end while ( SDL_PollEvent( &event ) )
    // update all subcomponents
    for (unsigned int i = 0; i < _cars.size(); ++i)
      _cars[i].update(winw, winh, &_bubble_man);
    for (unsigned int i = 0; i < _fishes.size(); ++i)
      _fishes[i].update(winw, winh);
    bool ok = _candy_man.update(winw, winh);
    _bubble_man.update(winw, winh);
    // check candy touched by car
    for (unsigned int i = 0; i < _cars.size(); ++i) {
      if (_cars[i].collides_with(*(_candy_man._curr_candy))) {
        printf("Collision car %i %g!\n", i, _candy_man._curr_candy->get_life_timer());
      }
    }
    return ok;
  }

  //////////////////////////////////////////////////////////////////////////////

  bool render() {
    SDL_RenderClear( renderer );
    DEBUG_PRINT("Game::render()\n");
    bool ok = _candy_man.render(renderer);
    for (unsigned int i = 0; i < _cars.size(); ++i)
      ok  = ok && _cars[i].render(renderer);
    for (unsigned int i = 0; i < _fishes.size(); ++i)
      ok  = ok && _fishes[i].render(renderer);
    ok  = ok && _bubble_man.render(renderer);
    DEBUG_PRINT("render finished()\n");
    SDL_RenderPresent( renderer);
    return ok;
  }

  //////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////

protected:
  SDL_Window* window;
  SDL_Renderer* renderer;
  std::vector<SDL_Joystick*> gameControllers;
  int winw, winh;
  CandyManager _candy_man;
  std::vector<Car> _cars;
  std::vector<Fish> _fishes;
  BubbleManager _bubble_man;
}; // end Game

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int main() {
  Game game;
  if (!game.init())
    return false;
  Rate rate(20);
  while (true) {
    if (!game.update())
      break;
    if (!game.render())
      break;
    rate.sleep();
  }
  return (game.clean() ? 0 : -1);
} // end main()
