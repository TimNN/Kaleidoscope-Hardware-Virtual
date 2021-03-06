#include "Keyboard.h"
#include "LEDs.h"
#include "Logging.h"
#include "virtual_io.h"
#include <iostream>
#include <sstream>
#include <assert.h>

using namespace kaleidoscope::logging;

static StandardKeyboardReportConsumer standardKeyboardReportConsumer;

Keyboard_::Keyboard_(void)
  :  _keyboardReportConsumer(&standardKeyboardReportConsumer),
     _systemLEDStates(0) {
}

void Keyboard_::begin(void) {
  releaseAll();
}
void Keyboard_::end(void) {
  releaseAll();
}

size_t Keyboard_::press(uint8_t k) {
  if (k == HID_KEYPAD_NUM_LOCK_AND_CLEAR ||
      k == HID_KEYBOARD_LOCKING_NUM_LOCK) {
    if (_numlockPressed) {
      // do nothing, just pressed again while already pressed
    } else {
      _numlockPressed = true;
      if (_systemLEDStates & LED_NUM_LOCK) {
        _systemLEDStates &= ~LED_NUM_LOCK;  // reset LED_NUM_LOCK
      } else {
        _systemLEDStates |= LED_NUM_LOCK;  // set LED_NUM_LOCK
      }
    }
  }

  // The remainder of this function is taken directly from KeyboardioHID's version

  // If the key is in the range of 'printable' keys
  if (k <= HID_LAST_KEY) {
    uint8_t bit = 1 << (uint8_t(k) % 8);
    _keyReport.keys[k / 8] |= bit;
    return 1;
  }

  // It's a modifier key
  else if (k >= HID_KEYBOARD_FIRST_MODIFIER && k <= HID_KEYBOARD_LAST_MODIFIER) {
    // Convert key into bitfield (0 - 7)
    k = k - HID_KEYBOARD_FIRST_MODIFIER;
    _keyReport.modifiers |= (1 << k);
    return 1;
  }

  // No empty/pressed key was found
  return 0;
}

size_t Keyboard_::release(uint8_t k) {
  if (k == HID_KEYPAD_NUM_LOCK_AND_CLEAR ||
      k == HID_KEYBOARD_LOCKING_NUM_LOCK) {
    _numlockPressed = false;
  }

  // The remainder of this function is taken directly from KeyboardioHID's version

  // If we're releasing a printable key
  if (k <= HID_LAST_KEY) {
    uint8_t bit = 1 << (k % 8);
    _keyReport.keys[k / 8] &= ~bit;
    return 1;
  }

  // It's a modifier key
  else if (k >= HID_KEYBOARD_FIRST_MODIFIER && k <= HID_KEYBOARD_LAST_MODIFIER) {
    // Convert key into bitfield (0 - 7)
    k = k - HID_KEYBOARD_FIRST_MODIFIER;
    _keyReport.modifiers &= ~(1 << k);
    return 1;
  }

  // No empty/pressed key was found
  return 0;
}

// releaseAll(), isModifierActive(), and wasModifierActive() are all
// taken directly from KeyboardioHID's versions
void Keyboard_::releaseAll(void) {
  memset(&_keyReport.allkeys, 0x00, sizeof(_keyReport.allkeys));
}
boolean Keyboard_::isModifierActive(uint8_t k) {
  if (k >= HID_KEYBOARD_FIRST_MODIFIER && k <= HID_KEYBOARD_LAST_MODIFIER) {
    k = k - HID_KEYBOARD_FIRST_MODIFIER;
    return !!(_keyReport.modifiers & (1 << k));
  }
  return false;
}
boolean Keyboard_::wasModifierActive(uint8_t k) {
  if (k >= HID_KEYBOARD_FIRST_MODIFIER && k <= HID_KEYBOARD_LAST_MODIFIER) {
    k = k - HID_KEYBOARD_FIRST_MODIFIER;
    return !!(_lastKeyReport.modifiers & (1 << k));
  }
  return false;
}

uint8_t Keyboard_::getLEDs() {
  return _systemLEDStates;
}

// For each bit set in 'bitfield', output the corresponding string to 'stream'
#define FOREACHBIT(bitfield, stream, str0, str1, str2, str3, str4, str5, str6, str7) \
  if((bitfield) & 1<<0) stream << str0; \
  if((bitfield) & 1<<1) stream << str1; \
  if((bitfield) & 1<<2) stream << str2; \
  if((bitfield) & 1<<3) stream << str3; \
  if((bitfield) & 1<<4) stream << str4; \
  if((bitfield) & 1<<5) stream << str5; \
  if((bitfield) & 1<<6) stream << str6; \
  if((bitfield) & 1<<7) stream << str7;

int Keyboard_::sendReport(void) {
  // Following KeyboardioHID, we only send report if it differs from previous report.
  if (!memcmp(_lastKeyReport.allkeys, _keyReport.allkeys, sizeof(_keyReport))) return -1;

  assert(_keyboardReportConsumer);
  _keyboardReportConsumer->processKeyboardReport(_keyReport);

  memcpy(_lastKeyReport.allkeys, _keyReport.allkeys, sizeof(_keyReport));

  return 0;  // actually not sure what the return value should be here; seems that
  // existing code doesn't check it
}

void Keyboard_::setKeyboardReportConsumer(
  KeyboardReportConsumer_ &keyboardReportConsumer) {
  _keyboardReportConsumer = &keyboardReportConsumer;
}

void StandardKeyboardReportConsumer::processKeyboardReport(
  const HID_KeyboardReport_Data_t &reportData) {
  std::stringstream keypresses;
  bool anything = false;
  if (reportData.modifiers) anything = true;
  else for (int i = 0; i < KEY_BYTES; i++) if (reportData.keys[i]) {
        anything = true;
        break;
      }
  if (!anything) {
    keypresses << "none";
  } else {
    FOREACHBIT(reportData.modifiers, keypresses,
               "lctrl ", "lshift ", "lalt ", "lgui ",
               "rctrl ", "rshift ", "ralt ", "rgui ")
    FOREACHBIT(reportData.keys[0], keypresses,
               "NO_EVENT ", "ERROR_ROLLOVER ", "POST_FAIL ", "ERROR_UNDEFINED ",
               "a ", "b ", "c ", "d ")
    FOREACHBIT(reportData.keys[1], keypresses,
               "e ", "f ", "g ", "h ", "i ", "j ", "k ", "l ")
    FOREACHBIT(reportData.keys[2], keypresses,
               "m ", "n ", "o ", "p ", "q ", "r ", "s ", "t ")
    FOREACHBIT(reportData.keys[3], keypresses,
               "u ", "v ", "w ", "x ", "y ", "z ", "1/! ", "2/@ ")
    FOREACHBIT(reportData.keys[4], keypresses,
               "3/# ", "4/$ ", "5/% ", "6/^ ", "7/& ", "8/* ", "9/( ", "0/) ")
    FOREACHBIT(reportData.keys[5], keypresses,
               "enter ", "esc ", "del/bksp ", "tab ",
               "space ", "-/_ ", "=/+ ", "[/{ ")
    FOREACHBIT(reportData.keys[6], keypresses,
               "]/} ", "\\/| ", "#/~ ", ";/: ", "'/\" ", "`/~ ", ",/< ", "./> ")
    FOREACHBIT(reportData.keys[7], keypresses,
               "//? ", "capslock ", "F1 ", "F2 ", "F3 ", "F4 ", "F5 ", "F6 ")
    FOREACHBIT(reportData.keys[8], keypresses,
               "F7 ", "F8 ", "F9 ", "F10 ", "F11 ", "F12 ", "prtscr ", "scrolllock ")
    FOREACHBIT(reportData.keys[9], keypresses,
               "pause ", "ins ", "home ", "pgup ", "del ", "end ", "pgdn ", "r_arrow ")
    FOREACHBIT(reportData.keys[10], keypresses,
               "l_arrow ", "d_arrow ", "u_arrow ", "numlock ",
               "num/ ", "num* ", "num- ", "num+ ")
    FOREACHBIT(reportData.keys[11], keypresses,
               "numenter ", "num1 ", "num2 ", "num3 ",
               "num4 ", "num5 ", "num6 ", "num7 ")
    FOREACHBIT(reportData.keys[12], keypresses,
               "num8 ", "num9 ", "num0 ", "num. ", "\\/| ", "app ", "power ", "num= ")
    FOREACHBIT(reportData.keys[13], keypresses,
               "F13 ", "F14 ", "F15 ", "F16 ", "F17 ", "F18 ", "F19 ", "F20 ")
    FOREACHBIT(reportData.keys[14], keypresses,
               "F21 ", "F22 ", "F23 ", "F24 ", "exec ", "help ", "menu ", "sel ")
    FOREACHBIT(reportData.keys[15], keypresses,
               "stop ", "again ", "undo ", "cut ", "copy ", "paste ", "find ", "mute ")
    FOREACHBIT(reportData.keys[16], keypresses,
               "volup ", "voldn ", "capslock_l ", "numlock_l ",
               "scrolllock_l ", "num, ", "num= ", "(other) ")
    for (int i = 17; i < KEY_BYTES; i++) {
      // A little imprecise, in two ways:
      //   (1) obviously, "(other)" refers to many distinct keys
      //   (2) this might undercount the number of "other" keys pressed
      // Therefore, if any keys are frequently used, they should be handled above and not via "other"
      if (reportData.keys[i]) keypresses << "(other) ";
    }
  }

  log_info("Sent virtual HID report. Pressed keys: %s\n", keypresses.str().c_str());
  logUSBEvent_keyboard("Keyboard HID report; pressed keys: " + keypresses.str());
}

Keyboard_ Keyboard;
