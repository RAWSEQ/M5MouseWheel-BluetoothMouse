#include <M5Core2.h>
#include <BleMouse.h>

#define Faces_Encoder_I2C_ADDR     0X5E

#define MODE_X 1
#define MODE_Y 2
#define MODE_SCR 3
#define MODE_WX 4
#define MODE_WY 5
#define MODE_STEP 6

#define MODE_X_COLOR 0xff0000
#define MODE_Y_COLOR 0x3333ff
#define MODE_SCR_COLOR 0xff9900
#define MODE_WX_COLOR 0x33ff33
#define MODE_WY_COLOR 0x00cccc
#define MODE_STEP_COLOR 0xffffff

#define INTERVAL_TIMES_MIDDLE 500

bool ble_mode=false;
int st_mode=0;
int batt_percent=100;
int encoder_pos=0;
int m_step=1;
int st_b_l=0;
int st_b_r=0;
int st_b_m=0;
int iv_cnt_middle=0;

ButtonColors cl_on  = {0x7BEF, WHITE, WHITE};
ButtonColors cl_off = {BLACK, 0xC618, 0xC618};
ButtonColors cl_bl = {0x7BEF, 0xC618, 0xC618};
Button btn_x(110, 110, 100, 100, false ,"X", cl_off, cl_on);
Button btn_y(220, 110, 100, 100, false ,"Y", cl_off, cl_on);
Button btn_scr(0, 110, 100, 48, false ,"SCR", cl_off, cl_on);
Button btn_wx(0, 162, 48, 48, false ,"WX", cl_off, cl_on);
Button btn_wy(52, 162, 48, 48, false ,"WY", cl_off, cl_on);
Button btn_step(0, 6, 100, 100, false ,"STEP", cl_off, cl_on);
Button btn_l(170, 6, 65, 100, false ,"L", cl_off, cl_on);
Button btn_r(240, 6, 70, 48, false ,"R", cl_off, cl_on);
Button btn_m(240, 58, 70, 48, false ,"M", cl_off, cl_on);

BleMouse bleMouse("M5MouseWheel", "@RAWSEQ", batt_percent);

void setup() {
  // Setup M5
  M5.begin();
  M5.Lcd.fillScreen(BLACK);

  // Setup Buttons
  M5.Buttons.setFont(FSSB12);
  btn_x.setFont(FSSB24);
  btn_y.setFont(FSSB24);
  btn_x.addHandler(event_btn_x, E_RELEASE);
  btn_y.addHandler(event_btn_y, E_RELEASE);
  btn_scr.addHandler(event_btn_scr, E_RELEASE);
  btn_wx.addHandler(event_btn_wx, E_RELEASE);
  btn_wy.addHandler(event_btn_wy, E_RELEASE);
  btn_step.addHandler(event_btn_step, E_RELEASE);
  btn_l.addHandler(event_btn_l, E_RELEASE);
  btn_r.addHandler(event_btn_r, E_RELEASE);
  btn_m.addHandler(event_btn_m, E_RELEASE);
  M5.Buttons.draw();
  label_update_step();
  
  // Bluetooth Begin
  bleMouse.begin();
  label_change_status("Bluetooth Pairing...", RED);
}

void loop() {
  M5.update();

  // Interval Middle
  iv_cnt_middle++; if (iv_cnt_middle >= INTERVAL_TIMES_MIDDLE) { iv_cnt_middle = 0; }
  if (iv_cnt_middle == 0) {
    
    // Bluetooth
    bool diff_ble_mode = ble_mode;
    ble_mode = bleMouse.isConnected();
    if (diff_ble_mode != ble_mode) {
      if (ble_mode) {
        label_change_status("Bluetooth Connected.", BLUE);
      } else {
        label_change_status("Bluetooth Pairing...", RED);
      }
    }

    // Battery
    float batVoltage = M5.Axp.GetBatVoltage();
    int batPercent = ( batVoltage < 3.2 ) ? 0 : ( batVoltage - 3.2 ) * 100;
    if (batt_percent != batPercent) {
      bleMouse.setBatteryLevel(batPercent);
      batt_percent = batPercent;
    }
  }

  // Encoder Monitoring
  if (ble_mode) {
    Wire1.requestFrom(Faces_Encoder_I2C_ADDR, 3);
    if (Wire1.available()) {   
      int increment = Wire1.read();
      int diff_encoder_pos = encoder_pos;
      encoder_pos += ((increment > 127) ? (256 - increment) * -1 : increment);
      if (encoder_pos != diff_encoder_pos) {
        event_encoder_rotate(encoder_pos - diff_encoder_pos);
      }
    }
  }

  delay(10);
}

void label_change_status(char *msg, long color) {
  M5.Lcd.fillRect(0, 218, 320, 25, BLACK);
  M5.Lcd.setTextColor(color);
  M5.Lcd.drawString(msg, 0, 228, 1);
}

void label_update_step() {
  M5.Lcd.fillRect(105, 38, 50, 30, BLACK);
  char num_char[4];
  sprintf(num_char, "%d", m_step);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.drawString(num_char, 131, 53, 1);
}

void encoder_set_led(int i, long rgb) {
    delay(5);
    Wire1.beginTransmission(Faces_Encoder_I2C_ADDR);
    Wire1.write(i);
    Wire1.write((byte)(rgb>>16));
    Wire1.write((byte)(rgb>>8));
    Wire1.write((byte)(rgb));
    Wire1.endTransmission();
}

void encoder_set_led_all(long rgb) {
  for (int i = 0; i < 12; i++){
    encoder_set_led(i, rgb);
  }
}

void event_encoder_rotate(int incremental) {
  switch (st_mode) {
    case MODE_X:
      bleMouse.move(incremental*m_step,0);
      break;
    case MODE_Y:
      bleMouse.move(0,incremental*m_step);
      break;
    case MODE_SCR:
      bleMouse.move(0,0,incremental*m_step*-1);
      break;
    case MODE_WX:
      bleMouse.move(0,0,0,incremental*m_step);
      break;
    case MODE_WY:
      bleMouse.move(0,0,incremental*m_step);
      break;
    case MODE_STEP:
      m_step += incremental;
      label_update_step();
      break;
    default:
      break;
  }
}

void sub_set_mode(int mode, long color) {
  if (st_mode != mode) {
    btn_x.draw(cl_off);
    btn_y.draw(cl_off);
    btn_scr.draw(cl_off);
    btn_wx.draw(cl_off);
    btn_wy.draw(cl_off);
    btn_step.draw(cl_off);
    st_mode = mode;
    encoder_set_led_all(color);
  }
}

void event_btn_x(Event& e) {
  sub_set_mode(MODE_X, MODE_X_COLOR);
  btn_x.draw(cl_bl);
}
void event_btn_y(Event& e) {
  sub_set_mode(MODE_Y, MODE_Y_COLOR);
  btn_y.draw(cl_bl);
}
void event_btn_scr(Event& e) {
  sub_set_mode(MODE_SCR, MODE_SCR_COLOR);
  btn_scr.draw(cl_bl);
}
void event_btn_wx(Event& e) {
  sub_set_mode(MODE_WX, MODE_WX_COLOR);
  btn_wx.draw(cl_bl);
}
void event_btn_wy(Event& e) {
  sub_set_mode(MODE_WY, MODE_WY_COLOR);
  btn_wy.draw(cl_bl);
}
void event_btn_step(Event& e) {
  sub_set_mode(MODE_STEP, MODE_STEP_COLOR);
  btn_step.draw(cl_bl);
}
void event_btn_l(Event& e) {
  if (st_b_l) {
    bleMouse.release(MOUSE_LEFT);
    btn_l.draw(cl_off);
    st_b_l = 0;
  } else {
    bleMouse.press(MOUSE_LEFT);
    btn_l.draw(cl_bl);
    st_b_l = 1;
  }
}
void event_btn_r(Event& e) {
  if (st_b_r) {
    bleMouse.release(MOUSE_RIGHT);
    btn_r.draw(cl_off);
    st_b_r = 0;
  } else {
    bleMouse.press(MOUSE_RIGHT);
    btn_r.draw(cl_bl);
    st_b_r = 1;
  }
}
void event_btn_m(Event& e) {
  if (st_b_m) {
    bleMouse.release(MOUSE_MIDDLE);
    btn_m.draw(cl_off);
    st_b_m = 0;
  } else {
    bleMouse.press(MOUSE_MIDDLE);
    btn_m.draw(cl_bl);
    st_b_m = 1;
  }
}
