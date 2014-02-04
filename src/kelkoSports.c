#include <pebble.h>

Window* window;
TextLayer* mode_text_layer;
TextLayer* timer_layer;
TextLayer* sportLength_text_layer;
InverterLayer* polish_layer;

typedef enum MODES { ModeIdle, ModePrepare, ModeSport, ModePause } mode;
mode currentMode = ModeIdle;

int prepareLength = 5;
int sportLength = 45;
int currentTimer = 0;

AppTimer* timerHandle;
void handle_tick(void* cookie);

void udpateSportLengthDisplay() {
	char* formattedLengthDisplay = "45 sec.";
	snprintf(formattedLengthDisplay, 8, "%02d sec.", sportLength);
	text_layer_set_text(sportLength_text_layer, formattedLengthDisplay);	
}

void updateClock() {
	char* formattedClockDisplay = "9999";
	snprintf(formattedClockDisplay, 4, "%03d ", currentTimer);
	text_layer_set_text(timer_layer, formattedClockDisplay);
}

void stopCurrentTimer() {
	app_timer_cancel(timerHandle);
}

void startTimer() {
	timerHandle = app_timer_register(1000, handle_tick, 0);
}

void switchToMode(mode newMode) {
	stopCurrentTimer();

	currentMode = newMode;
	switch(newMode) {
		case ModeIdle:
			text_layer_set_text(mode_text_layer, "Idle");
			currentTimer = 0;
		break;
		
		case ModePrepare:
			text_layer_set_text(mode_text_layer, "Prepare");
			currentTimer = prepareLength;
			startTimer();
		break;
		
		case ModeSport:
			text_layer_set_text(mode_text_layer, "Go!");
			currentTimer = sportLength;
			startTimer();
		break;
		
		case ModePause:
			text_layer_set_text(mode_text_layer, "Pause");
			currentTimer = sportLength + sportLength;
			startTimer();
		break;
	}
	
	updateClock();
}

void switchToNextMode(ClickRecognizerRef recognizer, void *context) {
	if (currentMode == ModeIdle || currentMode == ModePause) {
		switchToMode(ModePrepare);
		
	} else if (currentMode == ModePrepare || currentMode == ModeSport) {
		switchToMode(ModeIdle);
		
	}
}

void restartPrepare(ClickRecognizerRef recognizer, void *context) {
	if (currentMode == ModePrepare || currentMode == ModeSport) {
		switchToMode(ModePrepare);
		
	} else if (currentMode == ModePause) {
		currentTimer += sportLength;
		updateClock();
	}
}

void switchSportLength(ClickRecognizerRef recognizer, void *context) {
	if (currentMode == ModeIdle || currentMode == ModePause) {
		sportLength = (sportLength == 45) ? 30 : 45;
		udpateSportLengthDisplay();
	}
}


static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, switchToNextMode);
  window_single_click_subscribe(BUTTON_ID_UP, restartPrepare);
  window_single_click_subscribe(BUTTON_ID_DOWN, switchSportLength);
}

static void window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	
	mode_text_layer = text_layer_create( (GRect) { .origin = {8, 8}, .size = { 128, 30} });
	text_layer_set_text_alignment(mode_text_layer, GTextAlignmentLeft);
	text_layer_set_text(mode_text_layer, "---");
	text_layer_set_font(mode_text_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
	layer_add_child(window_layer, text_layer_get_layer(mode_text_layer));
	
	timer_layer = text_layer_create( (GRect) { .origin = {0, 44}, .size = { 140, 120} });
	text_layer_set_text_alignment(timer_layer, GTextAlignmentCenter);
	text_layer_set_text(timer_layer, "---");
	text_layer_set_font(timer_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
	layer_add_child(window_layer, text_layer_get_layer(timer_layer));
	
	sportLength_text_layer = text_layer_create( (GRect) { .origin = {0, 132}, .size = { 144, 30} });
	text_layer_set_text_alignment(sportLength_text_layer, GTextAlignmentRight);
	text_layer_set_text(sportLength_text_layer, "45 sec.");
	text_layer_set_font(sportLength_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	layer_add_child(window_layer, text_layer_get_layer(sportLength_text_layer));
	
	polish_layer = inverter_layer_create(GRect(0,0 , 144, 168));
	layer_add_child(window_layer, inverter_layer_get_layer(polish_layer));

	switchToMode(ModeIdle);	
}

static void window_unload(Window *window) {
  text_layer_destroy(mode_text_layer);
  text_layer_destroy(timer_layer);
  text_layer_destroy(sportLength_text_layer);
}

void handle_tick(void* cookie) {
	currentTimer--;
  
  	if (currentTimer == 15) {
  		vibes_double_pulse();
  		
  	} else if (currentTimer == 5) {
  		vibes_short_pulse();
  	}
  
	if (currentTimer < 5 || currentTimer % 5 == 0) {
		updateClock();
	}
	
	if (currentTimer > 0) {
		timerHandle = app_timer_register(1000, handle_tick, 0);

	} else {	
		vibes_long_pulse();		
		switchToMode((currentMode +1) % 4);
	}
}

static void init(void) {
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
