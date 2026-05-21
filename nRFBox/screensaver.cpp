#include "screensaver.h"
#include "screensaver_anim.h"
#include "setting.h" // For u8g2 and screensaverType

namespace ScreenSaver {

  int current_frame = 0;
  unsigned long last_frame_time = 0;
  
  // Matrix specific variables
  const int num_drops = 16;
  int drops_y[16];
  int drops_speed[16];
  char drops_chars[16][5];
  unsigned long last_char_switch_time = 0;

  void screensaverSetup() {
    u8g2.clearBuffer();
    u8g2.sendBuffer();

    if (screensaverType == 0) {
      current_frame = 0;
    } else {
      for(int i=0; i<num_drops; i++) {
        drops_y[i] = random(-64, 0);
        drops_speed[i] = random(1, 4);
        for(int j=0; j<5; j++) {
          drops_chars[i][j] = random(2) ? '1' : '0';
        }
      }
    }
  }

  void screensaverLoop() {
    if (screensaverType == 0) {
      // Cat animation
      if (millis() - last_frame_time > 100) { // ~10fps
        last_frame_time = millis();
        
        u8g2.clearBuffer();
        const unsigned char* frame_ptr = (const unsigned char*)pgm_read_ptr(&ss_frames[current_frame]);
        u8g2.drawXBMP(0, 0, 128, 64, frame_ptr);
        u8g2.sendBuffer();

        current_frame = (current_frame + 1) % ss_frames_count;
      }
    } else {
      // Matrix animation
      if (millis() - last_char_switch_time > 300) { // Switch some characters every 300ms
        last_char_switch_time = millis();
        for(int i=0; i<num_drops; i++) {
          for(int j=0; j<5; j++) {
            if (random(100) < 15) { // 15% chance to flip each character
              drops_chars[i][j] = random(2) ? '1' : '0';
            }
          }
        }
      }

      if (millis() - last_frame_time > 50) { // ~20fps
        last_frame_time = millis();
        
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_5x8_tr);
        for(int i=0; i<num_drops; i++) {
          for(int j=0; j<5; j++) {
             int py = drops_y[i] - j*8;
             if (py > 0 && py < 72) {
                 u8g2.setCursor(i * 8, py);
                 // Draw the stable character
                 char c[2] = { drops_chars[i][j], '\0' };
                 u8g2.print(c);
             }
          }
          drops_y[i] += drops_speed[i];
          if (drops_y[i] - 40 > 64) {
             drops_y[i] = random(-20, 0);
             drops_speed[i] = random(2, 5);
          }
        }
        u8g2.sendBuffer();
      }
    }
  }
}
