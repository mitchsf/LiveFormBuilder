# LiveFormBuilder

AJAX-based live settings form library for ESP32. Generates a responsive web UI that updates device settings in real time — no reboot required. Press **Save** to persist to NVS or LittleFS via a user-provided callback.

## Features

- Real-time AJAX updates — each field change takes effect immediately
- Responsive design — works on desktop and mobile browsers
- Dropdown selects (standard and offset-indexed)
- Range sliders with live value display
- Color pickers (RGB stored as int)
- Conditional fields — show/hide based on runtime state
- Section subheadings and separators for visual grouping
- Save callback for NVS persistence
- onChange callback for side effects (brightness, mode switches, etc.)

## Requirements

- ESP32 (Arduino framework)
- `WiFi.h`, `WiFiServer.h`, `WiFiClient.h` (included with ESP32 Arduino core)

## Installation

Copy `LiveFormBuilder.h` and `LiveFormBuilder.cpp` into your project's `src/` or `lib/` directory.

## Quick Start

```cpp
#include "LiveFormBuilder.h"

WiFiServer formServer(81);
LiveFormBuilder liveForm;

int preset[10] = {0};

void saveSettings() {
  // write preset[] to NVS here
}

void onFieldChange(const String& field, int value) {
  if (field == "brightness") analogWrite(LED_PIN, value);
}

void setup() {
  // ... WiFi connect ...

  formServer.begin();

  liveForm.setTitle("My Clock v1.0");
  liveForm.setSubtitle("Live Settings");
  liveForm.setSaveCallback(saveSettings);
  liveForm.setOnChange(onFieldChange);
  liveForm.begin(&formServer);

  liveForm.addDropDown("Mode", "mode", "Auto,Manual,Demo", &preset[0]);
  liveForm.addRange("Brightness", "brightness", 0, 255, &preset[1], "br");
  liveForm.addColorPicker("Accent Color", "color", &preset[2]);
}

void loop() {
  liveForm.handleClient();
}
```

## API Reference

### Setup

| Method | Description |
|--------|-------------|
| `setTitle(title)` | First line of the page header |
| `setSubtitle(subtitle)` | Second line of the page header |
| `setSaveCallback(cb)` | Called when the Save button is pressed |
| `setOnChange(cb)` | Called after each field change with `(field, value)` |
| `begin(server)` | Attach to a `WiFiServer*` |
| `handleClient()` | Call from `loop()` to process incoming HTTP requests |

### Field Builders

| Method | Description |
|--------|-------------|
| `addDropDown(label, field, options, preset*)` | Dropdown select. Options are comma-separated. Value stored is the selected index (0-based). |
| `addDropDownOffset(label, field, options, preset*, offset)` | Dropdown where stored value = selected index + offset. Useful when option indices don't start at 0. |
| `addRange(label, field, min, max, preset*, id)` | Range slider. `id` is a short HTML id for the value display span. |
| `addColorPicker(label, field, preset*)` | HTML color picker. Stores RGB as a single int (e.g., `0xFF8800`). |
| `addConditionalDropDown(condition, label, field, options, preset*)` | Dropdown that only renders when `condition()` returns `true`. |
| `addSubheading(text)` | Section heading for visual grouping. |
| `addSeparator()` | Horizontal rule divider. |

### Compile-Time Limits

Override before including the header if needed:

```cpp
#define LF_MAX_FIELDS  40   // maximum number of form fields
#define LF_MAX_OPTIONS 20   // maximum options per dropdown
```

## How It Works

LiveFormBuilder runs a lightweight HTTP server on the ESP32. When a browser connects, it serves a single self-contained HTML page with inline CSS and JavaScript. Field changes fire AJAX `fetch()` calls back to the ESP32 (`/?field=name&value=N`), which updates the bound `preset` variable and invokes the onChange callback immediately. The Save button sends `/?save=1`, triggering the save callback for NVS persistence.

No external dependencies, no frameworks, no filesystem — the entire form is generated in code and served from RAM.

## License

MIT
