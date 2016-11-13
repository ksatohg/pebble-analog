#include "simple_analog.h"

#include "pebble.h"

static Window *s_window;
static Layer *s_simple_bg_layer, *s_date_layer, *s_hands_layer, *s_digit_layer;
static TextLayer *s_hour_label, *s_minute_label;
static TextLayer *s_day_label, *s_num_label, *s_bt_label;

static GPath *s_tick_paths[NUM_CLOCK_TICKS];
static GPath *s_minute_arrow, *s_hour_arrow;
static char s_num_buffer[12], s_day_buffer[6];
static char s_bt_buffer[12];
static bool bt_cond = true;
static char s_digit_minute_buffer[10], s_digit_hour_buffer[6];

// 背景の更新
static void bg_update_proc(Layer *layer, GContext *ctx) {
  // 背景レイヤーを黒で塗りつぶし
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  // 背景レイヤーに文字盤を描画
  graphics_context_set_fill_color(ctx, GColorWhite);
  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    const int x_offset = PBL_IF_ROUND_ELSE(18, 0);
    const int y_offset = PBL_IF_ROUND_ELSE(6, 0);
    gpath_move_to(s_tick_paths[i], GPoint(x_offset, y_offset));
    gpath_draw_filled(ctx, s_tick_paths[i]);
  }
}

// 針の更新
static void hands_update_proc(Layer *layer, GContext *ctx) {
  // レイヤーの矩形と中心を取得
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);

  // 現在時刻を取得
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  
  //------ 短針　-------
  // 塗る色を白に、枠線を黒にする
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);

  // 短針を時の角度に回転する
  gpath_rotate_to(s_hour_arrow, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, s_hour_arrow);
  gpath_draw_outline(ctx, s_hour_arrow);

  //------ 長針　-------
  // 塗る色を白に、枠線を黒にする
  #ifdef PBL_COLOR
    graphics_context_set_fill_color(ctx, GColorRed);
  #else
    graphics_context_set_fill_color(ctx, GColorWhite);
  #endif
  graphics_context_set_stroke_color(ctx, GColorBlack);

  // 長針を分の角度に回転する
  gpath_rotate_to(s_minute_arrow, TRIG_MAX_ANGLE * t->tm_min / 60);
  gpath_draw_filled(ctx, s_minute_arrow);
  gpath_draw_outline(ctx, s_minute_arrow);

  //------ 秒針　-------
  // 秒針の長さを算出。PebbleRound なら前者、違えば後者
  const int16_t second_hand_length = PBL_IF_ROUND_ELSE((bounds.size.w / 2) - 19, bounds.size.w / 2);

  // 秒針の角度を算出 （TRIG_MAX_ANGLE は360度のこと）
  int32_t second_angle = TRIG_MAX_ANGLE * t->tm_sec / 60;
  
  // 秒針の先端位置を算出
  GPoint second_hand = {
    .x = (int16_t)(sin_lookup(second_angle) * (int32_t)second_hand_length / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(second_angle) * (int32_t)second_hand_length / TRIG_MAX_RATIO) + center.y,
  };

  // 秒針の描画
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_line(ctx, second_hand, center);

  // 中心に黒点を打つ
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(bounds.size.w / 2 - 1, bounds.size.h / 2 - 1, 3, 3), 0, GCornerNone);

  // 時報
  if(t->tm_min == 0 && t->tm_sec == 0){
    switch (t->tm_hour % 12) {
      case 1:
        vibes_enqueue_custom_pattern(pat_01);
        break;
      case 2:
        vibes_enqueue_custom_pattern(pat_02);
        break;
      case 3:
        vibes_enqueue_custom_pattern(pat_03);
        break;
      case 4:
        vibes_enqueue_custom_pattern(pat_04);
        break;
      case 5:
        vibes_enqueue_custom_pattern(pat_05);
        break;
      case 6:
        vibes_enqueue_custom_pattern(pat_06);
        break;
      case 7:
        vibes_enqueue_custom_pattern(pat_07);
        break;
      case 8:
        vibes_enqueue_custom_pattern(pat_08);
        break;
      case 9:
        vibes_enqueue_custom_pattern(pat_09);
        break;
      case 10:
        vibes_enqueue_custom_pattern(pat_10);
        break;
      case 11:
        vibes_enqueue_custom_pattern(pat_11);
        break;
      case 0:
        vibes_enqueue_custom_pattern(pat_12);
        break;
    }
  }
}


// 時刻文字表示の更新
static void digit_update_proc(Layer *layer, GContext *ctx) {

  #define DIGIT_X_OFFSET          10
  #define DIGIT_X_OFFSET_LONG     20
  #define DIGIT_Y_OFFSET          17
  #define TOP_LIMIT               0
  #define LEFT_LIMIT              0
  #define BOTTOM_LIMIT            PBL_IF_ROUND_ELSE(180-22,168-22)
  #define RIGHT_LIMIT             PBL_IF_ROUND_ELSE(180-22,144-22)
  #define RIGHT_LIMIT_LONG        PBL_IF_ROUND_ELSE(180-45,144-45)
  #define ANGLE_MERGE             TRIG_MAX_ANGLE * 18 / 360 
  
  // 前回のテキストレイヤーを削除
  text_layer_destroy(s_minute_label);
  text_layer_destroy(s_hour_label);
  
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "digit_update_proc: start");
  
  // レイヤーの矩形と中心を取得
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);

  // 現在時刻を取得
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  
  //------------ 分表示位置の算出 -------------
  // 中心からの距離を算出。PebbleRound なら前者、違えば後者
  const int16_t radius_minute = PBL_IF_ROUND_ELSE((bounds.size.w / 2) - 19, bounds.size.w / 2 - 5 );
  // 角度を算出 （TRIG_MAX_ANGLE は360度のこと）
  int32_t angle_minute = TRIG_MAX_ANGLE * t->tm_min / 60;
  //int32_t angle_minute = TRIG_MAX_ANGLE * t->tm_sec / 60;
  
  //------------ 時表示位置の算出 -------------
  // 中心からの距離を算出。PebbleRound なら前者、違えば後者
  const int16_t radius_hour = PBL_IF_ROUND_ELSE((bounds.size.w / 2) - 35, bounds.size.w / 2 - 20 );
  // 角度を算出 （TRIG_MAX_ANGLE は360度のこと）
  int32_t angle_hour = (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6);

  // APP_LOG(APP_LOG_LEVEL_DEBUG, "digit_update_proc: minute:%d  hour:%d  TRIG_MAX_ANGLE:%d", angle_minute, angle_hour, TRIG_MAX_ANGLE);
  
  // 時分の角度差を算出
  int32_t angle_diff = (angle_minute > angle_hour ? angle_minute - angle_hour : angle_hour - angle_minute);
  angle_diff = (angle_diff > TRIG_MAX_ANGLE * 180 / 360 ? TRIG_MAX_ANGLE - angle_diff : angle_diff);

    // 時分の角度が近づいていれば
  if ( angle_diff < ANGLE_MERGE ) 
  {
    //------------ 時・分を合体して表示する ------------
    // 分表示位置の算出
    GPoint digit_minute = {
      .x = (int16_t)(sin_lookup(angle_minute) * (int32_t)radius_minute / TRIG_MAX_RATIO) + center.x - DIGIT_X_OFFSET_LONG,
      .y = (int16_t)(-cos_lookup(angle_minute) * (int32_t)radius_minute / TRIG_MAX_RATIO) + center.y - DIGIT_Y_OFFSET,
    };
    
    // 画面外に出た場合の補正
    digit_minute.x = digit_minute.x < LEFT_LIMIT       ? LEFT_LIMIT       : digit_minute.x;
    digit_minute.y = digit_minute.y < TOP_LIMIT        ? TOP_LIMIT        : digit_minute.y;
    digit_minute.x = digit_minute.x > RIGHT_LIMIT_LONG ? RIGHT_LIMIT_LONG : digit_minute.x;
    digit_minute.y = digit_minute.y > BOTTOM_LIMIT     ? BOTTOM_LIMIT     : digit_minute.y;
    // 分表示文字列
    strftime(s_digit_minute_buffer, sizeof(s_digit_minute_buffer), "%H:%M", t);

    // 分テキストレイヤーを作成
    s_minute_label = text_layer_create(PBL_IF_ROUND_ELSE(
      GRect(digit_minute.x , digit_minute.y, 47, 24),
      GRect(digit_minute.x , digit_minute.y, 47, 24)));
    // 分テキストレイヤーの属性をセット
    text_layer_set_text(s_minute_label, s_digit_minute_buffer);
    text_layer_set_background_color(s_minute_label, GColorClear);
    text_layer_set_text_color(s_minute_label, GColorWhite);
    text_layer_set_font(s_minute_label, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    // 分テキストレイヤーを追加
    layer_add_child(s_digit_layer, text_layer_get_layer(s_minute_label));

    // 時テキストレイヤーを作成（double freeで落ちるのを防ぐためのダミー。表示はなし）
    s_hour_label = text_layer_create(PBL_IF_ROUND_ELSE(
      GRect(digit_minute.x , digit_minute.y, -100, -100),
      GRect(digit_minute.x , digit_minute.y, -100, -100)));
    // 時テキストレイヤーの属性をセット
    text_layer_set_text(s_hour_label, s_digit_minute_buffer);
    text_layer_set_background_color(s_hour_label, GColorClear);
    text_layer_set_text_color(s_hour_label, GColorClear);
    text_layer_set_font(s_hour_label, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    // 時テキストレイヤーを追加
    layer_add_child(s_digit_layer, text_layer_get_layer(s_hour_label));
    
  } else {  
    //------------ 時・分を別々に表示する ------------
    // 分表示位置の算出
    GPoint digit_minute = {
      .x = (int16_t)(sin_lookup(angle_minute) * (int32_t)radius_minute / TRIG_MAX_RATIO) + center.x - DIGIT_X_OFFSET,
      .y = (int16_t)(-cos_lookup(angle_minute) * (int32_t)radius_minute / TRIG_MAX_RATIO) + center.y - DIGIT_Y_OFFSET,
    };
    // 画面外に出た場合の補正
    digit_minute.x = digit_minute.x < LEFT_LIMIT   ? LEFT_LIMIT   : digit_minute.x;
    digit_minute.y = digit_minute.y < TOP_LIMIT    ? TOP_LIMIT    : digit_minute.y;
    digit_minute.x = digit_minute.x > RIGHT_LIMIT  ? RIGHT_LIMIT  : digit_minute.x;
    digit_minute.y = digit_minute.y > BOTTOM_LIMIT ? BOTTOM_LIMIT : digit_minute.y;
    // 分表示文字列
    strftime(s_digit_minute_buffer, sizeof(s_digit_minute_buffer), "%M", t);

    // 時表示位置の算出
    GPoint digit_hour = {
      .x = (int16_t)(sin_lookup(angle_hour) * (int32_t)radius_hour / TRIG_MAX_RATIO) + center.x - DIGIT_X_OFFSET,
      .y = (int16_t)(-cos_lookup(angle_hour) * (int32_t)radius_hour / TRIG_MAX_RATIO) + center.y - DIGIT_Y_OFFSET,
    };
    // 画面外に出た場合の補正
    digit_hour.x = digit_hour.x < LEFT_LIMIT   ? LEFT_LIMIT   : digit_hour.x;
    digit_hour.y = digit_hour.y < TOP_LIMIT    ? TOP_LIMIT    : digit_hour.y;
    digit_hour.x = digit_hour.x > RIGHT_LIMIT  ? RIGHT_LIMIT  : digit_hour.x;
    digit_hour.y = digit_hour.y > BOTTOM_LIMIT ? BOTTOM_LIMIT : digit_hour.y;
    // 時表示文字列
    strftime(s_digit_hour_buffer, sizeof(s_digit_hour_buffer), "%H", t);

    // 分テキストレイヤーを作成
    s_minute_label = text_layer_create(PBL_IF_ROUND_ELSE(
      GRect(digit_minute.x , digit_minute.y, 22, 24),
      GRect(digit_minute.x , digit_minute.y, 22, 24)));
    // 分テキストレイヤーの属性をセット
    text_layer_set_text(s_minute_label, s_digit_minute_buffer);
    text_layer_set_background_color(s_minute_label, GColorClear);
    text_layer_set_text_color(s_minute_label, GColorWhite);
    text_layer_set_font(s_minute_label, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    // 分テキストレイヤーを追加
    layer_add_child(s_digit_layer, text_layer_get_layer(s_minute_label));
  
    // 時テキストレイヤーを作成
    s_hour_label = text_layer_create(PBL_IF_ROUND_ELSE(
      GRect(digit_hour.x , digit_hour.y, 22, 24),
      GRect(digit_hour.x , digit_hour.y, 22, 24)));
    // 時テキストレイヤーの属性をセット
    text_layer_set_text(s_hour_label, s_digit_hour_buffer);
    text_layer_set_background_color(s_hour_label, GColorClear);
    text_layer_set_text_color(s_hour_label, GColorWhite);
    text_layer_set_font(s_hour_label, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    // 時テキストレイヤーを追加
    layer_add_child(s_digit_layer, text_layer_get_layer(s_hour_label));
    
  }
}



// 日付の更新
static void date_update_proc(Layer *layer, GContext *ctx) {
  // 現在時刻を取得
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  // 日付フォーマットにして、日付テキストレイヤーにセット
  //strftime(s_num_buffer, sizeof(s_num_buffer), "%m/%d %a", t);
  strftime(s_num_buffer, sizeof(s_num_buffer), "%d %a", t);
  text_layer_set_text(s_num_label, s_num_buffer);
  // 曜日フォーマットにして、曜日テキストレイヤーにセット
  //strftime(s_day_buffer, sizeof(s_day_buffer), "(%a)", t);
  //text_layer_set_text(s_day_label, s_day_buffer);
}

// 秒タイマー
static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(s_window));
  // Layerを”dirty”にマークするものらしい。
  // dirtyにマークされたレイヤーは、システムが再描画（update_proc呼び出し）してくれるそうだ。
  // layer_mark_dirtyを呼んだ瞬間に再描画されるわけではなく、非同期で短時間後に再描画されるとのこと。
}

// BT接続状況の更新
static void handle_bluetooth(bool connected) {
  text_layer_set_text(s_bt_label, connected ? "" : "BT LOST !!");
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "handle_bluetooth: connected is %s", connected ? "true" : "false");
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "handle_bluetooth: bt_cond is %s", bt_cond ? "true" : "false");
  if (connected != bt_cond) {
    vibes_enqueue_custom_pattern(pat_BT);
    bt_cond = connected;
  }
}

// ウインドウのロード時の処理
static void window_load(Window *window) {
  // ルートレイヤーを取得し、その矩形を得る
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  //-----------------------------------------------------------------------------------------------------
  // 背景レイヤー（文字盤）を作成
  s_simple_bg_layer = layer_create(bounds);
  // 背景レイヤーが更新されたときのコールバック関数に bg_update_proc を設定
  layer_set_update_proc(s_simple_bg_layer, bg_update_proc);
  // 背景レイヤーを追加
  layer_add_child(window_layer, s_simple_bg_layer);

  //-----------------------------------------------------------------------------------------------------
  // 針レイヤーを作成
  s_hands_layer = layer_create(bounds);
  // 針レイヤーが更新されたときのコールバック関数に hands_update_proc を設定
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  // 針レイヤーを追加
  layer_add_child(window_layer, s_hands_layer);

  //-----------------------------------------------------------------------------------------------------
  // デジタルレイヤーを作成
  s_digit_layer = layer_create(bounds);
  // デジタルレイヤーが更新されたときのコールバック関数に digit_update_proc を設定
  layer_set_update_proc(s_digit_layer, digit_update_proc);
  // デジタルレイヤーを追加
  layer_add_child(window_layer, s_digit_layer);

  //-----------------------------------------------------------------------------------------------------
  // 日付レイヤーを作成
  s_date_layer = layer_create(bounds);
  // 日付レイヤーが更新されたときのコールバック関数に date_update_proc を設定
  layer_set_update_proc(s_date_layer, date_update_proc);
  // 日付レイヤーを追加
  layer_add_child(window_layer, s_date_layer);

  // 日付テキストレイヤーを作成
  s_num_label = text_layer_create(PBL_IF_ROUND_ELSE(
    GRect(90-28, 90+15, 56, 26),
    GRect(72-28, 72+15, 56, 26)));
  // 日付をセット
  text_layer_set_text(s_num_label, s_num_buffer);
  text_layer_set_background_color(s_num_label, GColorClear);
  text_layer_set_text_color(s_num_label, GColorCyan);
  text_layer_set_font(s_num_label, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(s_date_layer, text_layer_get_layer(s_num_label));

  //// 曜日テキストレイヤーを作成
  //s_day_label = text_layer_create(PBL_IF_ROUND_ELSE(
  //  GRect(50, 114, 50, 20),
  //  GRect(50, 114, 50, 20)));
  //// 曜日をセット
  //text_layer_set_text(s_day_label, s_day_buffer);
  //text_layer_set_background_color(s_day_label, GColorClear);
  //text_layer_set_text_color(s_day_label, GColorCyan);
  //text_layer_set_font(s_day_label, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  //layer_add_child(s_date_layer, text_layer_get_layer(s_day_label));

  // BTテキストレイヤーを作成
  s_bt_label = text_layer_create(PBL_IF_ROUND_ELSE(
    GRect(55,  70, 100, 20),
    GRect(40,  50, 100, 20)));
  // 日付をセット
  text_layer_set_text(s_bt_label, s_bt_buffer);
  text_layer_set_background_color(s_bt_label, GColorClear);
  text_layer_set_text_color(s_bt_label, GColorRed);
  text_layer_set_font(s_bt_label, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(s_date_layer, text_layer_get_layer(s_bt_label));
  handle_bluetooth(connection_service_peek_pebble_app_connection());

}

static void window_unload(Window *window) {
  layer_destroy(s_simple_bg_layer);
  layer_destroy(s_date_layer);

  text_layer_destroy(s_day_label);
  text_layer_destroy(s_num_label);

  layer_destroy(s_hands_layer);
}

static void init() {
  // ウインドウの生成
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);

  s_day_buffer[0] = '\0';
  s_num_buffer[0] = '\0';

  // 長針短針の描画用データ
  s_minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
  s_hour_arrow = gpath_create(&HOUR_HAND_POINTS);

  // ルートレイヤーを取得し、その矩形を得る 
  Layer *window_layer = window_get_root_layer(s_window);
  GRect bounds = layer_get_bounds(window_layer);
  // ルートレイヤーの中心
  GPoint center = grect_center_point(&bounds);
  // 長針短針の描画起点をルートレイヤーの中心にする
  gpath_move_to(s_minute_arrow, center);
  gpath_move_to(s_hour_arrow, center);

  // 背景の文字盤の描画データ
  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    s_tick_paths[i] = gpath_create(&ANALOG_BG_POINTS[i]);
  }

  // 秒タイマーを起動
  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);

  // Bluetooth割り込みを有効にする
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = handle_bluetooth
  });

}

static void deinit() {
  gpath_destroy(s_minute_arrow);
  gpath_destroy(s_hour_arrow);

  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    gpath_destroy(s_tick_paths[i]);
  }

  tick_timer_service_unsubscribe();
  connection_service_unsubscribe();
  window_destroy(s_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
