#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"


#define MY_UUID { 0x21, 0x3F, 0x86, 0x5E, 0x48, 0x3F, 0x4C, 0xE7, 0x80, 0x08, 0x68, 0xEE, 0xF2, 0x18, 0xE1, 0x1F }
PBL_APP_INFO(MY_UUID,
             "kelko Sport", ":kelko:",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_STANDARD_APP);

AppContextRef appContext;

Window window;
TextLayer mode_text_layer;
TextLayer timer_layer;
InverterLayer polish_layer;

typedef enum MODES { ModeIdle, ModePrepare, ModeSport, ModePause } mode;
mode currentMode = ModeIdle;

int prepareLength = 5;
int sportLength = 45;
int pauseLength = 90;
int currentTimer = 0;

AppTimerHandle timerHandle;

void updateClock() {
	char* formattedClockDisplay = "9999";
	snprintf(formattedClockDisplay, 4, "%03d ", currentTimer);
	text_layer_set_text(&timer_layer, formattedClockDisplay);
}

void stopCurrentTimer() {
	app_timer_cancel_event(appContext, timerHandle);
}

void startTimer() {
	timerHandle = app_timer_send_event(appContext, 1000, 0);
}

void switchToMode(mode newMode) {
	stopCurrentTimer();

	currentMode = newMode;
	switch(newMode) {
		case ModeIdle:
			text_layer_set_text(&mode_text_layer, "Idle");
			currentTimer = 0;
		break;
		
		case ModePrepare:
			text_layer_set_text(&mode_text_layer, "Prepare");
			currentTimer = prepareLength;
			startTimer();
		break;
		
		case ModeSport:
			text_layer_set_text(&mode_text_layer, "Go!");
			currentTimer = sportLength;
			startTimer();
		break;
		
		case ModePause:
			text_layer_set_text(&mode_text_layer, "Pause");
			currentTimer = pauseLength;
			startTimer();
		break;
	}
	
	updateClock();
}

void switchToNextMode(ClickRecognizerRef recognizer, Window *window) {
	if (currentMode == ModeIdle || currentMode == ModePause) {
		switchToMode(ModePrepare);
		
	} else if (currentMode == ModePrepare || currentMode == ModeSport) {
		switchToMode(ModeIdle);
		
	}
}

void restartPrepare(ClickRecognizerRef recognizer, Window *window) {
	if (currentMode == ModePrepare || currentMode == ModeSport) {
		switchToMode(ModePrepare);
	}
}

void doublePause(ClickRecognizerRef recognizer, Window *window) {
	if (currentMode == ModePause) {
		currentTimer += pauseLength;
		updateClock();
	}
}

void config_provider(ClickConfig **config, Window *window) {
  config[BUTTON_ID_SELECT]->click.handler = (ClickHandler) switchToNextMode;  
  config[BUTTON_ID_UP]->click.handler = (ClickHandler) restartPrepare;  
  config[BUTTON_ID_DOWN]->click.handler = (ClickHandler) doublePause;
  
  (void)window;
}

void handle_init(AppContextRef ctx) {
	appContext = ctx;

	window_init(&window, "kelko Sport");
	window_stack_push(&window, true /* Animated */);
	
	text_layer_init(&mode_text_layer, GRect(8, 8, 128, 30));
	text_layer_set_text_alignment(&mode_text_layer, GTextAlignmentLeft);
	text_layer_set_text(&mode_text_layer, "---");
	text_layer_set_font(&mode_text_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
	layer_add_child(&window.layer, &mode_text_layer.layer);
	
	text_layer_init(&timer_layer, GRect(0, 44, 144, 120));
	text_layer_set_text_alignment(&timer_layer, GTextAlignmentCenter);
	text_layer_set_text(&timer_layer, "---");
	text_layer_set_font(&timer_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
	layer_add_child(&window.layer, &timer_layer.layer);
	
	inverter_layer_init(&polish_layer, GRect(0,0 , 144, 168));
	layer_add_child(&window.layer, &polish_layer.layer);
		
	window_set_click_config_provider(&window, (ClickConfigProvider) config_provider);
	
	switchToMode(ModeIdle);	
}


void handle_tick(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie) {
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
		timerHandle = app_timer_send_event(appContext, 1000, 0);

	} else {	
		vibes_long_pulse();		
		switchToMode((currentMode +1) % 4);
	}
}

void pbl_main(void *params) {
	PebbleAppHandlers handlers = {
		.init_handler = &handle_init,
		.timer_handler = &handle_tick
	};
	app_event_loop(params, &handlers);
}
