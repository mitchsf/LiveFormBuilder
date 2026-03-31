/*----------------------------------------------------------------------*
   LiveFormBuilder.h - AJAX-based live settings form for ESP32
   
   Generates a responsive web form that updates settings immediately
   without reboot. Changes take effect on each field change.
   "Save" persists to NVS via user-provided callback.
   
   Usage:
     LiveFormBuilder liveForm;
     liveForm.setTitle("My Device v1.0");
     liveForm.setSubtitle("Live Settings");
     liveForm.setSaveCallback(mySaveFunc);
     liveForm.setOnChange(myChangeFunc);
     liveForm.setOnTextChange(myTextFunc);
     liveForm.begin(&server);
     // in loop: liveForm.handleClient();
  ----------------------------------------------------------------------*/

#ifndef LIVEFORMBUILDER_H
#define LIVEFORMBUILDER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>

#ifndef LF_MAX_FIELDS
#define LF_MAX_FIELDS 40
#endif

#ifndef LF_MAX_OPTIONS
#define LF_MAX_OPTIONS 20
#endif

enum LiveFieldType {
  LF_DROPDOWN,
  LF_RANGE,
  LF_SUBHEADING,
  LF_SEPARATOR,
  LF_COLORPICKER,
  LF_DROPDOWN_OFFSET,  // stored value = index + offset
  LF_TEXTINPUT         // text input with send button
};

struct LiveField {
  LiveFieldType type;
  String label;
  String fieldName;
  String options[LF_MAX_OPTIONS];
  int    optionCount;
  int    minVal;
  int    maxVal;       // also used as maxLength for text inputs
  int*   presetPtr;    // direct pointer to preset variable
  int    offset;       // for DROPDOWN_OFFSET: stored = selected + offset
  String rangeId;      // short ID for range value display span
  String placeholder;  // text input placeholder
  String buttonLabel;  // text input button label
  bool   (*condition)();  // nullptr = always show
};

typedef void (*LiveSaveCallback)();
typedef void (*LiveChangeCallback)(const String& field, int value);
typedef void (*LiveTextCallback)(const String& field, const String& value);

class LiveFormBuilder {
public:
  LiveFormBuilder();

  void setTitle(const String& title);
  void setSubtitle(const String& subtitle);
  void setSaveCallback(LiveSaveCallback cb);
  void setOnChange(LiveChangeCallback cb);
  void setOnTextChange(LiveTextCallback cb);
  void begin(WiFiServer* server);
  void handleClient();

  void addDropDown(const String& label, const String& field,
                   const String& options, int* preset);
  void addDropDownOffset(const String& label, const String& field,
                         const String& options, int* preset, int offset);
  void addRange(const String& label, const String& field,
                int minVal, int maxVal, int* preset, const String& id);
  void addSubheading(const String& text);
  void addConditionalSubheading(bool (*condition)(), const String& text);
  void addColorPicker(const String& label, const String& field, int* preset);
  void addSeparator();
  void addConditionalDropDown(bool (*condition)(),
                               const String& label, const String& field,
                               const String& options, int* preset);
  void addTextInput(const String& label, const String& field,
                    const String& placeholder, int maxLength,
                    const String& buttonLabel = "Send");

private:
  WiFiServer*        _server;
  String             _title;
  String             _subtitle;
  LiveSaveCallback   _saveCb;
  LiveChangeCallback _changeCb;
  LiveTextCallback   _textCb;
  LiveField          _fields[LF_MAX_FIELDS];
  int                _fieldCount;

  void parseOptions(const String& csv, String out[], int& count);
  void serveForm(WiFiClient& client);
  void handleAjax(WiFiClient& client, const String& req);
  void handleSave(WiFiClient& client);
  void sendOK(WiFiClient& client);
  void genDropDown(String& h, int idx);
  void genRange(String& h, int idx);
  void genSubheading(String& h, int idx);
  void genColorPicker(String& h, int idx);
  void genSeparator(String& h);
  void genTextInput(String& h, int idx);
  static String urlDecode(const String& input);
};

#endif
