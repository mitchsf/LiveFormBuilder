/*----------------------------------------------------------------------*
   LiveFormBuilder.h - AJAX-based live settings form for ESP32
   
   Generates a responsive web form that updates settings immediately
   without reboot. Changes take effect on each field change.
   "Save" persists to NVS via user-provided callback.
   
   Usage:
     LiveFormBuilder liveForm;
     liveForm.setTitle("My Device v1.0");
     liveForm.setSaveCallback(mySaveFunc);
     liveForm.setOnChange(myChangeFunc);
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
  LF_DROPDOWN_OFFSET   // stored value = index + offset
};

struct LiveField {
  LiveFieldType type;
  String label;
  String fieldName;
  String options[LF_MAX_OPTIONS];
  int    optionCount;
  int    minVal;
  int    maxVal;
  int*   presetPtr;       // direct pointer to preset variable
  int    offset;          // for DROPDOWN_OFFSET: stored = selected + offset
  String rangeId;         // short ID for range value display span
  bool   (*condition)();  // nullptr = always show
};

/**
 * Callback: called after save button pressed and NVS write requested
 */
typedef void (*LiveSaveCallback)();

/**
 * Callback: called when any field changes
 * Return: field name and new (raw) value
 * Use for side effects (brightLevel, setPIR, etc.)
 */
typedef void (*LiveChangeCallback)(const String& field, int value);

class LiveFormBuilder {
public:
  LiveFormBuilder();

  /** Set the page title (first line of header) */
  void setTitle(const String& title);

  /** Set subtitle (second line of header, e.g. "Live Settings") */
  void setSubtitle(const String& subtitle);

  /** Register save callback - called when Save button pressed */
  void setSaveCallback(LiveSaveCallback cb);

  /** Register onChange callback - called after each field update */
  void setOnChange(LiveChangeCallback cb);

  /** Initialize with WiFiServer pointer */
  void begin(WiFiServer* server);

  /** Call from loop() when WiFi is connected */
  void handleClient();

  // -- Field builders --------------------------------------------------

  /** Add a dropdown with comma-separated options, bound to *preset */
  void addDropDown(const String& label, const String& field,
                   const String& options, int* preset);

  /** Add a dropdown where stored value = selected index + offset */
  void addDropDownOffset(const String& label, const String& field,
                         const String& options, int* preset, int offset);

  /** Add a range slider, bound to *preset */
  void addRange(const String& label, const String& field,
                int minVal, int maxVal, int* preset, const String& id);

  /** Add a visual section subheading */
  void addSubheading(const String& text);

  /** Add a color picker, bound to *preset (stores RGB as int) */
  void addColorPicker(const String& label, const String& field, int* preset);

  /** Add a horizontal separator line */
  void addSeparator();

  /** Add a conditional dropdown - only renders when condition() returns true */
  void addConditionalDropDown(bool (*condition)(),
                               const String& label, const String& field,
                               const String& options, int* preset);

private:
  WiFiServer*       _server;
  String            _title;
  String            _subtitle;
  LiveSaveCallback  _saveCb;
  LiveChangeCallback _changeCb;
  LiveField         _fields[LF_MAX_FIELDS];
  int               _fieldCount;

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
};

#endif // LIVEFORMBUILDER_H
