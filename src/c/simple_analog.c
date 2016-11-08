#include "simple_analog.h"

#include "pebble.h"

static Window *s_window;
static Layer *s_simple_bg_layer, *s_date_layer, *s_hands_layer;
static TextLayer *s_day_label, *s_num_label;

static GPath *s_tick_paths[NUM_CLOCK_TICKS];
static GPath *s_minute_arrow, *s_hour_arrow;
static char s_num_buffer[4], s_day_buffer[6];

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

  // 秒針の長さを算出。PebbleRound なら前者、違えば後者
  const int16_t second_hand_length = PBL_IF_ROUND_ELSE((bounds.size.w / 2) - 19, bounds.size.w / 2);

  // 現在時刻を取得
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  
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

  // 塗る色を白に、枠線を黒にする
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);

  // 長針を分の角度に回転する
  gpath_rotate_to(s_minute_arrow, TRIG_MAX_ANGLE * t->tm_min / 60);
  gpath_draw_filled(ctx, s_minute_arrow);
  gpath_draw_outline(ctx, s_minute_arrow);

  // 短針を時の角度に回転する
  gpath_rotate_to(s_hour_arrow, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, s_hour_arrow);
  gpath_draw_outline(ctx, s_hour_arrow);

  // 中心に黒点を打つ
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(bounds.size.w / 2 - 1, bounds.size.h / 2 - 1, 3, 3), 0, GCornerNone);

  // 時報
  if(t->tm_min == 0){
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

// 日付の更新
static void date_update_proc(Layer *layer, GContext *ctx) {
  // 現在時刻を取得
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  // 曜日フォーマットにして、曜日テキストレイヤーにセット
  strftime(s_day_buffer, sizeof(s_day_buffer), "%a", t);
  text_layer_set_text(s_day_label, s_day_buffer);
  // 日付フォーマットにして、日付テキストレイヤーにセット
  strftime(s_num_buffer, sizeof(s_num_buffer), "%d", t);
  text_layer_set_text(s_num_label, s_num_buffer);
}

// 秒タイマー
static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(s_window));
  // Layerを”dirty”にマークするものらしい。
  // dirtyにマークされたレイヤーは、システムが再描画（update_proc呼び出し）してくれるそうだ。
  // layer_mark_dirtyを呼んだ瞬間に再描画されるわけではなく、非同期で短時間後に再描画されるとのこと。
}


// ウインドウのロード時の処理
static void window_load(Window *window) {
  // ルートレイヤーを取得し、その矩形を得る
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // 背景レイヤーを作成
  s_simple_bg_layer = layer_create(bounds);
  // 背景レイヤーが更新されたときのコールバック関数に bg_update_proc を設定
  layer_set_update_proc(s_simple_bg_layer, bg_update_proc);
  // 背景レイヤーを追加
  layer_add_child(window_layer, s_simple_bg_layer);

  // 日付レイヤーを作成
  s_date_layer = layer_create(bounds);
  // 日付レイヤーが更新されたときのコールバック関数に date_update_proc を設定
  layer_set_update_proc(s_date_layer, date_update_proc);
  // 日付レイヤーを追加
  layer_add_child(window_layer, s_date_layer);

  // 曜日テキストレイヤーを作成
  s_day_label = text_layer_create(PBL_IF_ROUND_ELSE(
    GRect(63, 114, 27, 20),
    GRect(46, 114, 27, 20)));
  // 曜日をセット
  text_layer_set_text(s_day_label, s_day_buffer);
  text_layer_set_background_color(s_day_label, GColorBlack);
  text_layer_set_text_color(s_day_label, GColorWhite);
  text_layer_set_font(s_day_label, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(s_date_layer, text_layer_get_layer(s_day_label));

  // 日付テキストレイヤーを作成
  s_num_label = text_layer_create(PBL_IF_ROUND_ELSE(
    GRect(90, 114, 18, 20),
    GRect(73, 114, 18, 20)));
  // 日付をセット
  text_layer_set_text(s_num_label, s_num_buffer);
  text_layer_set_background_color(s_num_label, GColorBlack);
  text_layer_set_text_color(s_num_label, GColorWhite);
  text_layer_set_font(s_num_label, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(s_date_layer, text_layer_get_layer(s_num_label));

  // 鉢レイヤーを作成
  s_hands_layer = layer_create(bounds);
  // 針レイヤーが更新されたときのコールバック関数に hands_update_proc を設定
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  // 針レイヤーを追加
  layer_add_child(window_layer, s_hands_layer);
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
}

static void deinit() {
  gpath_destroy(s_minute_arrow);
  gpath_destroy(s_hour_arrow);

  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    gpath_destroy(s_tick_paths[i]);
  }

  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
