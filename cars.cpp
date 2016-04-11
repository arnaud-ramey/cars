#include <iostream>
#include <algorithm>
#include "sdl_utils.h"


enum GameStatus {
  GAME_STATUS_WAITING   = 0, // waiting for robots or joypads
  GAME_STATUS_COUNTDOWN = 1, // countdown + lakitu
  GAME_STATUS_RACE      = 2, // race time
  GAME_STATUS_RACE_OVER = 3, // players cant move anymore
  NGAME_STATUES         = 4
};

class Fish : public Entity {
public:
  void move_random_border(int winw, int winh) {
    double x = 0, y = 0, angle  = 0;
    int border = rand() % 4;
    if (border == 0) { // left
      x = -_tex_radius+1;
      y = rand() % winh;
      angle = -M_PI_2 + drand48() * M_PI;
    }
    else if (border == 1) { // up
      x = rand() % winw;
      y = -_tex_radius+1;
      angle = -M_PI + drand48() * M_PI;
    }
    else if (border == 2) { // right
      x = winw + _tex_radius-1;
      y = rand() % winh;
      angle = M_PI_2 + drand48() * M_PI;
    }
    else if (border == 3) { // down
      x = rand() % winw;
      y = winh + _tex_radius-1;
      angle = drand48() * M_PI;
    }
    set_position(Point2d(x, y));
    increase_angle(angle);
    set_tan_nor_speed(Point2d(20 + rand() % 150, 0));
  }

  //////////////////////////////////////////////////////////////////////////////

  void update(int winw, int winh) {
    //double speedx = cos(3*get_life_timer()); set_angspeed(speedx);
    if (!is_visible(winw, winh))
      move_random_border(winw, winh);
    Entity::update_pos_speed();
  }

protected:
}; // end class Fish

////////////////////////////////////////////////////////////////////////////////

class BubbleManager {
public:
  void set_texture(Texture* tex) {
    _tex = tex;
  }
  void create_bubble(const Point2d & pos, const double & rendering_scale) {
    Entity b;
    b.set_position(pos);
    b.set_rendering_scale(rendering_scale);
    b.set_speed(Point2d(0, -30 - rand()%100));
    b.set_texture(_tex);
    _bubbles.push_back(b);
  }

  void update(int winw, int winh) {
    for (unsigned int i = 0; i < _bubbles.size(); ++i) {
      Entity* b = &(_bubbles[i]);
      double speedx = 100*cos(3*b->get_life_timer()+b->get_width());
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
      ok = ok && _bubbles[i].render(renderer);
    }
    return ok;
  }

  std::vector<Entity> _bubbles;
  Texture* _tex;
}; // end class BubbleManager

////////////////////////////////////////////////////////////////////////////////

class Car : public Entity {
public:
  bool set_textures(Texture* car_texture,
                    const Point2d & front_wheel_center_offset,
                    Texture* front_wheel_texture,
                    const Point2d & back_wheel_center_offset,
                    Texture* back_wheel_texture,
                    const Point2d & exhaust_pipe_offset) {
    // load data
    if (!set_texture(car_texture)) {
      printf("Car::load() failed\n");
      return false;
    }
    // set default values
    Entity front_wheel, back_wheel;
    front_wheel.set_texture(front_wheel_texture);
    add_child(car_texture->get_resize_scale() * front_wheel_center_offset, front_wheel);
    back_wheel.set_texture(back_wheel_texture);
    add_child(car_texture->get_resize_scale() * back_wheel_center_offset, back_wheel);
    _exhaust_pipe_offset       = car_texture->get_resize_scale() * exhaust_pipe_offset;
    set_position(Point2d(0, 0));
    return true;
  } // end load()

  //////////////////////////////////////////////////////////////////////////////

  void update(int winw, int winh, BubbleManager* bubble_gen) {
    // orientate car in direction of speed
    if (_speed.norm() > 10)
      _angle = atan2(_speed.y, _speed.x);
    // turn wheels faster if car faster
    double wheel_speed = hypot(_speed.y, _speed.x) / 10;
    for (int i = 0; i < _children.size(); ++i)
      _children[i].second.set_angspeed(wheel_speed);
    Entity::update_pos_speed();
    // stop if going out of the screen
    if (_position.x < _tex_radius) { // left
      _accel.x = std::max(_accel.x + 10, 20.);
      if (_speed.norm() > 100) _speed.renorm(100);
    }
    else if (_position.x > winw - _tex_radius) { // right
      _accel.x = std::min(_accel.x - 10, -20.);
      if (_speed.norm() > 100) _speed.renorm(100);
    }
    if (_position.y < _tex_radius) { // up
      _accel.y = std::max(_accel.y + 10, 20.);
      if (_speed.norm() > 100) _speed.renorm(100);
    }
    else if (_position.y > winh - _tex_radius) { // down
      _accel.y = std::min(_accel.y - 10, -20.);
      if (_speed.norm() > 100) _speed.renorm(100);
    }
    // create bubbles randomly or if accelerating
    if ((rand() % 2000 + _accel.norm()) > 1950) {
      Point2d ex = offset2world_pos(_exhaust_pipe_offset);
      bubble_gen->create_bubble(ex, .2 + .5 * drand48() + _accel.norm() / 2000.); // bigger if accelerating
    }
  }

  int rank;
protected:
  Point2d _exhaust_pipe_offset;
}; // end class Car


////////////////////////////////////////////////////////////////////////////////

class Candy : public Entity {
public:
  Candy() {
    _need_respawn = true;
  }

  bool set_textures(std::vector<Texture*> & candy_texture_ptrs) {
    _candy_textures = candy_texture_ptrs;
    set_texture(_candy_textures.back());
    _need_respawn = true;
    return true;
  }

  void move_far_away() { set_position(Point2d(-_entity_radius, -_entity_radius)); }

  bool respawn(int winw, int winh, std::vector<Car> & cars){
    if (_candy_textures.empty()) {
      printf("Cannot respawn candy without texture!\n");
      return false;
    }
    _need_respawn = false;
    _tex_idx = rand() % _candy_textures.size();
    set_texture(_candy_textures[_tex_idx]);
    Point2d old_pos = get_position();
    for (int tryidx = 0; tryidx < 100; ++tryidx) {
      bool ok = true;
      // new random position
      set_position(Point2d(.1 * winw + rand() % (int) (.8 * winw),
                           .1 * winh + rand() % (int) (.8 * winh)));
      // check far from previous
      if ((get_position() - old_pos).norm() < winw / 3)
        continue;
      // check far from players
      for (int i = 0; i < cars.size(); ++i) {
        if ((get_position() - cars[i].get_position()).norm() >= winw / 3)
          continue; // far enough from player i
        ok = false;
        break;
      } // end for i
      if (ok)
        break;
    } // end tryidx
    return true;
  } // end respawn()

  bool update(int winw, int winh, std::vector<Car> & cars) {
    if (get_position().x <= 0)
      return respawn(winw, winh, cars);
    return true;
  } // end update()

  std::vector<Texture*> _candy_textures;
  unsigned int _tex_idx;
  bool _need_respawn;
}; // end class Candy

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class Game {
public:
  static const double GAME_LENGTH = 15; // seconds
  static const double COUNTDOWN_LENGTH = 5; // seconds

  bool init(unsigned int winw, unsigned int winh) {
    _game_status = GAME_STATUS_WAITING;
    unsigned int nfishes = 15;
    _nplayers = 2;
    _winw = winw;
    _winh  = winh; // pixels

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
      return false;
    }
    //Initialize SDL_mixer
    if( Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 ) < 0 ) {
      printf( "SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError() );
      return false;
    }
    //Initialize SDL_ttf
    if( TTF_Init() == -1 ) {
      printf( "SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError() );
      return false;
    }
    // create window
    SDL_Rect windowRect = { 10, 10, _winw, _winh};
    window = SDL_CreateWindow( "cars", windowRect.x, windowRect.y, _winw, _winh, 0 );
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
    SDL_RenderSetLogicalSize( renderer, _winw, _winh );
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

    ///
    /// load data
    ///
    std::string base_path = SDL_GetBasePath(),
        data_path = base_path + "../data/",
        graphics_path = data_path + "graphics/";
    DEBUG_PRINT("base_path:'%s'\n", base_path.c_str());
    //Open the score font
    _score_font = TTF_OpenFont( (data_path + "fonts/LCD2U___.TTF").c_str(), 40 );
    if( _score_font == NULL ) {
      printf( "Failed to load font! SDL_ttf Error: %s\n", TTF_GetError() );
      return false;
    }
    // create scores
    _score_textures.resize(_nplayers);
    _scores.resize(_nplayers);
    //Open the time font
    _time_font = TTF_OpenFont( (data_path + "fonts/LCD2U___.TTF").c_str(), 80 );
    if( _time_font == NULL ) {
      printf( "Failed to load font! SDL_ttf Error: %s\n", TTF_GetError() );
      return false;
    }
    std::ostringstream GAME_LENGTH_str; GAME_LENGTH_str << GAME_LENGTH;
    _last_renderer_time = -1;
    // load music and sounds
    // WAVE, MOD, MIDI, OGG, MP3, FLAC
    // sox cocoa_river.ogg -r 22050 cocoa_river.wav
    _music = Mix_LoadMUS( (data_path + "music/cocoa_river.ogg").c_str() );
    if( _music == NULL ) {
      printf( "Failed to load music! SDL_mixer Error: %s\n", Mix_GetError() );
      return false;
    }
    if (! (_grab_collectable_sfx    = Mix_LoadWAV((data_path + "sounds/grab_collectable.ogg").c_str()))
        || ! (_last_lap_fanfare_sfx = Mix_LoadWAV((data_path + "sounds/last_lap_fanfare.ogg").c_str()))
        || ! (_pre_start_race_sfx   = Mix_LoadWAV((data_path + "sounds/pre_start_race.ogg").c_str()))
        || ! (_race_finish_sfx      = Mix_LoadWAV((data_path + "sounds/race_finish.ogg").c_str()))
        || ! (_start_race_sfx       = Mix_LoadWAV((data_path + "sounds/start_race.ogg").c_str()))
        || ! (_track_intro_sfx      = Mix_LoadWAV((data_path + "sounds/track_intro.ogg").c_str()))
        ) {
      printf( "Failed to load music! SDL_mixer Error: %s\n", Mix_GetError() );
      return false;
    }
    Mix_VolumeMusic(128);
    Mix_PlayChannel( -1, _track_intro_sfx, 0 );
    // init bubble manager
    _bubble_tex.from_file(renderer, graphics_path + "bubble.png", 50);
    _bubble_man.set_texture(&_bubble_tex);
    for (unsigned int i = 0; i < 10; ++i)
      _bubble_man.create_bubble(Point2d(rand()% _winw, rand() % _winh), .5);
    // init candy
    _candy_textures.resize(3);
    _candy_textures[0].from_file(renderer, graphics_path + "candy/chuche1.png", 100);
    _candy_textures[1].from_file(renderer, graphics_path + "candy/chuche2.png", 50);
    _candy_textures[2].from_file(renderer, graphics_path + "candy/huevo.png", 80);
    std::vector<Texture*> candy_texture_ptrs;
    for (int i = 0; i < _candy_textures.size(); ++i)
      candy_texture_ptrs.push_back(&_candy_textures[i]);
    _candy.set_textures(candy_texture_ptrs);
    // init cars
    unsigned int car_width = 200, cup_width = 64; // px
    _cup_textures.resize(3);
    _cup_textures[0].from_file(renderer, graphics_path + "cup_gold.png", cup_width);
    _cup_textures[1].from_file(renderer, graphics_path + "cup_silver.png", cup_width);
    _cup_textures[2].from_file(renderer, graphics_path + "cup_bronze.png", cup_width);
    _car_textures.resize(7);
    _car_textures[0].from_file(renderer, graphics_path + "cars/arnaud.png", car_width);
    _car_textures[1].from_file(renderer, graphics_path + "cars/arnaud_front_wheel.png",
                               -1, -1, _car_textures[0].get_resize_scale());
    _car_textures[2].from_file(renderer, graphics_path + "cars/arnaud_back_wheel.png",
                               -1, -1, _car_textures[0].get_resize_scale());
    _car_textures[3].from_file(renderer, graphics_path + "cars/unai.png", car_width);
    _car_textures[4].from_file(renderer, graphics_path + "cars/unai_front_wheel.png",
                               -1, -1, _car_textures[3].get_resize_scale());
    _car_textures[5].from_file(renderer, graphics_path + "cars/unai_back_wheel.png",
                               -1, -1, _car_textures[3].get_resize_scale());
    _car_textures[6].from_file(renderer, graphics_path + "cars/ainara.png", car_width);
    _cars.resize(_nplayers);
    for (int i = 0; i < _nplayers; ++i) {
      if (i == 0  && !_cars[i].set_textures(&_car_textures[0],
                                            Point2d(1165, 501), &_car_textures[1],
                                            Point2d(182, 494), &_car_textures[2],
                                            Point2d(9, 469)) )
        return false;
      else if (i == 1  && !_cars[i].set_textures(&_car_textures[3],
                                                 Point2d(1211, 502), &_car_textures[4],
                                                 Point2d(182, 513), &_car_textures[5],
                                                 Point2d(7, 484)))
        return false;
      // default car
      else if (i >= 2 && !_cars[i].set_textures(&_car_textures[6],
                                                Point2d(1165, 501), &_car_textures[1],
                                                Point2d(182, 494), &_car_textures[2],
                                                Point2d(9, 469)))
        return false;
      _cars[i].set_position(Point2d(200, (i+1) * winh / (_nplayers+1)));
    }
    // init fishes
    unsigned int nfish_textures = 8;
    _fish_textures.resize(nfish_textures);
    int fish_size = 100; // px
    for (unsigned int i = 0; i < nfish_textures; ++i) {
      std::ostringstream filename;
      filename << graphics_path + "fish/pez"<< i+1 << ".png";
      _fish_textures[i].from_file(renderer, filename.str(), fish_size);
    }
    _fishes.resize(nfishes);
    for (unsigned int var = 0; var < nfishes; ++var) {
      Fish* fish = &(_fishes[var]);
      fish->set_texture(&_fish_textures[rand()%nfish_textures]);
      fish->move_random_border(_winw, _winh);
    }
    return true;
  } // end init()

  //////////////////////////////////////////////////////////////////////////////

  bool clean() {
    DEBUG_PRINT("Game::clean()\n");
    SDL_DestroyRenderer( renderer);
    SDL_DestroyWindow( window );
    //Stop the music
    if (_music)
      Mix_FreeMusic( _music );
    _music = NULL;
    Mix_FreeChunk_safe(_grab_collectable_sfx);
    Mix_FreeChunk_safe(_last_lap_fanfare_sfx);
    Mix_FreeChunk_safe(_pre_start_race_sfx);
    Mix_FreeChunk_safe(_race_finish_sfx);
    Mix_FreeChunk_safe(_start_race_sfx);
    Mix_FreeChunk_safe(_track_intro_sfx);
    if (_score_font)
      TTF_CloseFont( _score_font );
    if (_time_font)
      TTF_CloseFont( _time_font );
    _score_font = _time_font = NULL;
    for (unsigned int i = 0; i < gameControllers.size(); ++i)
      SDL_JoystickClose( gameControllers[i] );
    //Quit SDL subsystems
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();
    return true;
  }
  //////////////////////////////////////////////////////////////////////////////

  bool update() {
    DEBUG_PRINT("Game::update()\n");
    // check game status changes
    if (_game_status == GAME_STATUS_WAITING) {
      DEBUG_PRINT("Game status: WAITING->COUNTDOWN()\n");
      _game_status = GAME_STATUS_COUNTDOWN;
      _game_timer.reset();
      _candy.move_far_away();
      Mix_HaltMusic();
      // reset ranks and scores
      for (int i = 0; i < _nplayers; ++i) {
        _cars[i].rank = -1;
        _scores[i] = 0;
        if (!_score_textures[i].loadFromRenderedText(renderer, _score_font, "0", 255, 0, 0))
          return false;
      }
      _time_texture.loadFromRenderedText(renderer, _time_font, "0", 255, 0, 0);
    }
    else if (_game_status == GAME_STATUS_COUNTDOWN) {
      if (_game_timer.getTimeSeconds() >= COUNTDOWN_LENGTH) {
        DEBUG_PRINT("Game status: COUNTODWNG->RACE()\n");
        _game_status = GAME_STATUS_RACE;
        _game_timer.reset();
        //Play the music
        Mix_PlayChannel( -1, _start_race_sfx, 0 );
        Mix_PlayMusic( _music, -1 );
      }
    }
    else if (_game_status == GAME_STATUS_RACE) {
      if (_game_timer.getTimeSeconds() >= GAME_LENGTH) {
        DEBUG_PRINT("Game status: RACE->RACE_OVER()\n");
        _game_status = GAME_STATUS_RACE_OVER;
        _candy.move_far_away();
        Mix_HaltMusic();
        Mix_PlayChannel( -1, _race_finish_sfx, 0 );
        podium();
      }
    }
    // update all subcomponents
    for (unsigned int i = 0; i < _nplayers; ++i)
      _cars[i].update(_winw, _winh, &_bubble_man);
    for (unsigned int i = 0; i < _fishes.size(); ++i)
      _fishes[i].update(_winw, _winh);
    if (_game_status ==  GAME_STATUS_RACE && !_candy.update(_winw, _winh, _cars))
      return false;
    _bubble_man.update(_winw, _winh);
    // check candy touched by car
    if (_game_status == GAME_STATUS_RACE) {
      for (unsigned int i = 0; i < _nplayers; ++i) {
        if (!_cars[i].collides_with(_candy, 80))
          continue;
        DEBUG_PRINT("Car %i got a candy at time %g!\n", i, _candy.get_life_timer());
        Mix_PlayChannel( -1, _grab_collectable_sfx, 0 );
        ++_scores[i];
        std::ostringstream score;
        score << _scores[i];
        if (!_score_textures[i].loadFromRenderedText(renderer, _score_font, score.str(), 255, 0, 0))
          return false;
        if (!_candy.respawn(_winw, _winh, _cars))
          return false;
      }
    }

    // update with events
    SDL_Event event;
    while ( SDL_PollEvent( &event ) ) {
      if ( event.type == SDL_QUIT )
        return false;
      else if ( event.type == SDL_KEYDOWN ) {
        SDL_Keycode key = event.key.keysym.sym;
        if (key == SDLK_q)
          return false;
        else if (key == SDLK_r)
          _game_status = GAME_STATUS_WAITING;
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
        if( event.jaxis.which <= _nplayers ) {
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
    return true;
  }

  //////////////////////////////////////////////////////////////////////////////

  bool render() {
    SDL_RenderClear( renderer );
    DEBUG_PRINT("Game::render()\n");
    bool ok = _candy.render(renderer);
    for (unsigned int i = 0; i < _nplayers; ++i) {
      ok  = ok && _cars[i].render(renderer);
      // rander rank cup if needed
      int rank = _cars[i].rank;
      if (rank < 0 || rank >= 3)
        continue;
      Point2d pos = _cars[i].get_position() + Point2d(0, -_cars[i].get_entity_radius());
      _cup_textures[rank].render_center(renderer, pos);
    }
    for (unsigned int i = 0; i < _fishes.size(); ++i)
      ok  = ok && _fishes[i].render(renderer);
    ok  = ok && _bubble_man.render(renderer);
    // render scores
    for (unsigned int i = 0; i < _nplayers; ++i) {
      int cell = _winw / (_nplayers+1), x = cell * (i+1);
      ok = ok && _cars[i].get_texture()->render_center(renderer, Point2d(x, 30), .5);
      ok = ok && _score_textures[i].render_center(renderer, Point2d(x + 70, 30));
      int rank = _cars[i].rank; // render rank cup if needed
      if (rank >= 0 && rank < 3)
        ok = ok && _cup_textures[rank].render_center(renderer, Point2d(x - 60, 30), .5);
    }
    // render time
    // refresh time if needed
    if (_game_status == GAME_STATUS_COUNTDOWN) {
      int time = COUNTDOWN_LENGTH + 1 -_game_timer.getTimeSeconds();
      if (time <= 3 && time != _last_renderer_time)
        Mix_PlayChannel( -1, _pre_start_race_sfx, 0 );
      ok = ok && render_time(time, 255, 0, 0)
           && _time_texture.render_center(renderer, Point2d(50, 50),1);
    } // end if GAME_STATUS_COUNTDOWN
    else if (_game_status == GAME_STATUS_RACE) {
      int time = GAME_LENGTH + 1 - _game_timer.getTimeSeconds();
      if (time == 9 && _last_renderer_time == 10) // 10 last seconds sfx
        Mix_PlayChannel( -1, _last_lap_fanfare_sfx, 0 ); // last seconds
      else if (time <= 5 && time != _last_renderer_time)
        Mix_PlayChannel( -1, _pre_start_race_sfx, 0 );
      ok = ok && render_time(time, 255, 255, 255)
           && _time_texture.render_center(renderer, Point2d(50, 50),1);
    } // end if GAME_STATUS_RACE
    DEBUG_PRINT("render finished()\n");
    SDL_RenderPresent( renderer);
    return ok;
  }

  //////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////

protected:
  void podium() { // set ranks for each player
    // https://stackoverflow.com/questions/9025084/sorting-a-vector-in-descending-order
    std::vector<int> scores_sorted = _scores;
    std::sort(scores_sorted.begin(), scores_sorted.end(), std::greater<int>());
    for (int rank = 2; rank >= 0; --rank) {
      for (int i = 0; i < _nplayers; ++i) {
        if (_scores[i] == scores_sorted[rank])
          _cars[i].rank = rank;
      }
    }
  } // end podium()

  //! \return true if render OK or already done
  bool render_time(const int time, int r, int g, int b) {
    if (_last_renderer_time == time)
      return true;
    std::ostringstream time_str; time_str << time;
    _last_renderer_time = time;
    return _time_texture.loadFromRenderedText(renderer, _time_font, time_str.str(),
                                              r, g, b);
  }

  SDL_Window* window;
  SDL_Renderer* renderer;
  int _winw, _winh, _nplayers;
  Timer _game_timer;
  GameStatus _game_status;
  // joystick stuff
  std::vector<SDL_Joystick*> gameControllers;
  // score stuff
  TTF_Font *_score_font;
  std::vector<Texture> _score_textures;
  std::vector<int> _scores;
  // time display stuff
  TTF_Font *_time_font;
  int _last_renderer_time;
  Texture _time_texture;
  // music stuff
  Mix_Music *_music;
  Mix_Chunk* _grab_collectable_sfx, *_last_lap_fanfare_sfx, *_pre_start_race_sfx,
  *_race_finish_sfx, *_start_race_sfx, *_track_intro_sfx;
  // candy stuff
  Candy _candy;
  std::vector<Texture> _candy_textures;
  // cars stuff
  std::vector<Car> _cars;
  std::vector<Texture> _car_textures;
  std::vector<Texture> _cup_textures;
  // fish stuff
  std::vector<Fish> _fishes;
  std::vector<Texture> _fish_textures;
  // bublle stuff
  BubbleManager _bubble_man;
  Texture _bubble_tex;
}; // end Game

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int main() {
  Game game;
  if (!game.init(800, 500)) {
    printf("game.init() failed!\n");
    return false;
  }
  Rate rate(20);
  while (true) {
    if (!game.update()){
      printf("game.update() failed!\n");
      break;
    }
    if (!game.render()){
      printf("game.render() failed!\n");
      break;
    }
    rate.sleep();
  }
  return (game.clean() ? 0 : -1);
} // end main()
