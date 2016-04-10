#include <iostream>
#include <vector>
#include "sdl_utils.h"

class Fish : public OrientedTexture {
public:
  Fish() {}

  //////////////////////////////////////////////////////////////////////////////

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
    return _bubble_tex.from_file(renderer, str, .1);
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
        //printf("Erasing bubble %i\n", i);
        _bubbles.erase(_bubbles.begin() + i);
        --i;
      }
    }
  } // end update()

  bool render(SDL_Renderer* renderer) {
    bool ok = true;
    for (unsigned int i = 0; i < _bubbles.size(); ++i) {
      ok = ok &&
          _bubble_tex.render_center(renderer,
                                    _bubbles[i].get_position(),
                                    1. * _bubbles[i].get_radius() / _bubble_tex.getWidth());
    }
    return ok;
  }

  Texture _bubble_tex;
  std::vector<OrientedObject> _bubbles;
}; // end class BubbleManager

////////////////////////////////////////////////////////////////////////////////

class Car : public OrientedTexture {
public:
  Car() {}

  void load(SDL_Renderer* renderer,
            const std::string & car_filename,
            const Point2d & front_wheel_center_offset,
            const std::string & front_wheel_filename,
            const Point2d & back_wheel_center_offset,
            const std::string & back_wheel_filename,
            const Point2d & exhaust_pipe_offset,
            double scale = 1) {
    // load data
    OrientedTexture::load(renderer, car_filename, scale);
    _front_wheel_img.from_file(renderer, front_wheel_filename, scale);
    _back_wheel_img.from_file(renderer, back_wheel_filename, scale);
    // set default values
    _wheelangle = 0;
    _front_wheel_center_offset = scale * front_wheel_center_offset;
    _back_wheel_center_offset = scale * back_wheel_center_offset;
    _exhaust_pipe_offset = scale * exhaust_pipe_offset;
    set_position(Point2d(0, 0));
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
    if ((rand() % 450 + _accel.norm()) > 400) {
      Point2d ex = offset2world_pos(_exhaust_pipe_offset);
      bubble_gen->create_bubble(ex, 5 + rand() % 25 + _accel.norm() / 20); // bigger if accelerating
    }
  }

  bool render(SDL_Renderer* renderer) {
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
  CandyManager() {
    _candy.set_position(Point2d(-1, -1));
  }

  bool add_file(SDL_Renderer* renderer, const std::string &str) {
    _candy_textures.resize(_candy_textures.size() + 1);
    _candy_textures.back().from_file(renderer, str, .1);
  }

  bool respawn(int winw, int winh){
    if (_candy_textures.empty()) {
      printf("Cannot render candy without texture!\n");
      return false;
    }
    _candy.set_position(Point2d(rand()% winw, rand()%winh));
    _tex_idx = rand() % _candy_textures.size();
    return true;
  }

  void update(int winw, int winh) {
    if (_candy.get_position().x < 0)
      respawn(winw, winh);
  } // end update()

  bool render(SDL_Renderer* renderer) {
    if (_tex_idx >= _candy_textures.size())
      return false;
    return _candy_textures[_tex_idx].render_center(renderer, _candy.get_position());
  }

  std::vector<Texture> _candy_textures;
  OrientedObject _candy;
  unsigned int _tex_idx;
}; // end class CandyManager

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class Game {
public:
  Game() {}

  void update(int winw, int winh) {
    _candy_man.update(winw, winh);
    for (unsigned int i = 0; i < _cars.size(); ++i)
      _cars[i].update(winw, winh, &_bubble_man);
    for (unsigned int i = 0; i < _fishes.size(); ++i)
      _fishes[i].update(winw, winh);
    _bubble_man.update(winw, winh);
  }

  bool render(SDL_Renderer* renderer) {
    bool ok = _candy_man.render(renderer);
    for (unsigned int i = 0; i < _cars.size(); ++i)
      ok  = ok && _cars[i].render(renderer);
    for (unsigned int i = 0; i < _fishes.size(); ++i)
      ok  = ok && _fishes[i].render(renderer);
    ok  = ok && _bubble_man.render(renderer);
    return ok;
  }

  CandyManager _candy_man;
  std::vector<Car> _cars;
  std::vector<Fish> _fishes;
  BubbleManager _bubble_man;
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
  unsigned int winw = 1200, winh  = 600; // pixels
  SDL_Rect windowRect = { 10, 10, winw, winh};
  SDL_Window* window = SDL_CreateWindow( "Server", windowRect.x, windowRect.y, winw, winh, 0 );
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
  SDL_RenderSetLogicalSize( renderer, winw, winh );
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

  std::string base_path = SDL_GetBasePath(), model_path = base_path + "../models/";
  printf("base_path:'%s'\n", base_path.c_str());
  Game game;
  game._bubble_man.from_file(renderer, model_path + "bubble.png");
  for (unsigned int i = 0; i < 10; ++i)
    game._bubble_man.create_bubble(Point2d(rand()% winw, rand() % winh), 10 + rand()%100);
  game._candy_man.add_file(renderer, model_path + "fish/chuche1.png");

  game._cars.resize(2);
  Car* car1 = &(game._cars[0]);
  Car* car2 = &(game._cars[1]);
  double car_size = .12;
  car1->load(renderer, model_path + "arnaud.png",
             Point2d(1165, 501), model_path + "arnaud_front_wheel.png",
             Point2d(182, 494), model_path + "arnaud_back_wheel.png",
             Point2d(9, 469),
             car_size);
  car2->load(renderer, model_path + "unai.png",
             Point2d(1211, 502), model_path + "unai_front_wheel.png",
             Point2d(182, 513), model_path + "unai_back_wheel.png",
             Point2d(7, 484),
             car_size);
  car1->set_position(Point2d(200, 200));
  car2->set_position(Point2d(200, 400));
  unsigned int nfishes = 15;
  game._fishes.resize(nfishes);
  for (unsigned int var = 0; var < nfishes; ++var) {
    Fish* fish = &(game._fishes[var]);
    double size = .05 + drand48() * .3;
    std::ostringstream filename;
    filename << model_path + "fish/pez"<< 1+rand()%8 << ".png";
    fish->load(renderer, filename.str(), size);
    fish->move_random_border(winw, winh);
  }

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
        if (key == SDLK_UP)
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
          Point2d accel = car1->get_accel();
          // X axis motion
          if( event.jaxis.axis == 0 )
            car1->set_accel(Point2d(event.jaxis.value / 100, accel.y));
          else if( event.jaxis.axis == 1)
            car1->set_accel(Point2d(accel.x, event.jaxis.value / 100));
        }
      } // end SDL_JOYAXISMOTION
    } // end while ( SDL_PollEvent( &event ) )
    game.update(winw, winh);

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
