/*----------------------------------------------------------------------*
   LiveFormBuilder.cpp - AJAX-based live settings form for ESP32
  ----------------------------------------------------------------------*/

#include "LiveFormBuilder.h"

LiveFormBuilder::LiveFormBuilder()
  : _server(nullptr), _title("Live Settings"), _subtitle(""),
    _saveCb(nullptr), _changeCb(nullptr), _textCb(nullptr), _fieldCount(0)
{}

void LiveFormBuilder::setTitle(const String& title) { _title = title; }
void LiveFormBuilder::setSubtitle(const String& subtitle) { _subtitle = subtitle; }
void LiveFormBuilder::setSaveCallback(LiveSaveCallback cb) { _saveCb = cb; }
void LiveFormBuilder::setOnChange(LiveChangeCallback cb) { _changeCb = cb; }
void LiveFormBuilder::setOnTextChange(LiveTextCallback cb) { _textCb = cb; }

void LiveFormBuilder::begin(WiFiServer* server) {
  _server = server;
}

// -- URL decode ----------------------------------------------------------

String LiveFormBuilder::urlDecode(const String& input) {
  String decoded;
  decoded.reserve(input.length());
  for (int i = 0; i < (int)input.length(); i++) {
    if (input[i] == '+') {
      decoded += ' ';
    } else if (input[i] == '%' && i + 2 < (int)input.length()) {
      char hex[3] = { input[i + 1], input[i + 2], 0 };
      decoded += (char)strtol(hex, nullptr, 16);
      i += 2;
    } else {
      decoded += input[i];
    }
  }
  return decoded;
}

// -- Field builders ------------------------------------------------------

void LiveFormBuilder::parseOptions(const String& csv, String out[], int& count) {
  count = 0;
  int start = 0;
  for (int i = 0; i <= (int)csv.length(); i++) {
    if (i == (int)csv.length() || csv[i] == ',') {
      if (count < LF_MAX_OPTIONS) {
        out[count] = csv.substring(start, i);
        out[count].trim();
        count++;
      }
      start = i + 1;
    }
  }
}

void LiveFormBuilder::addDropDown(const String& label, const String& field,
                                   const String& options, int* preset) {
  if (_fieldCount >= LF_MAX_FIELDS) return;
  LiveField& f = _fields[_fieldCount++];
  f.type = LF_DROPDOWN;
  f.label = label;
  f.fieldName = field;
  f.presetPtr = preset;
  f.offset = 0;
  f.condition = nullptr;
  parseOptions(options, f.options, f.optionCount);
}

void LiveFormBuilder::addDropDownOffset(const String& label, const String& field,
                                         const String& options, int* preset, int offset) {
  if (_fieldCount >= LF_MAX_FIELDS) return;
  LiveField& f = _fields[_fieldCount++];
  f.type = LF_DROPDOWN_OFFSET;
  f.label = label;
  f.fieldName = field;
  f.presetPtr = preset;
  f.offset = offset;
  f.condition = nullptr;
  parseOptions(options, f.options, f.optionCount);
}

void LiveFormBuilder::addRange(const String& label, const String& field,
                                int minVal, int maxVal, int* preset,
                                const String& id) {
  if (_fieldCount >= LF_MAX_FIELDS) return;
  LiveField& f = _fields[_fieldCount++];
  f.type = LF_RANGE;
  f.label = label;
  f.fieldName = field;
  f.minVal = minVal;
  f.maxVal = maxVal;
  f.presetPtr = preset;
  f.rangeId = id;
  f.offset = 0;
  f.condition = nullptr;
}

void LiveFormBuilder::addSubheading(const String& text) {
  if (_fieldCount >= LF_MAX_FIELDS) return;
  LiveField& f = _fields[_fieldCount++];
  f.type = LF_SUBHEADING;
  f.label = text;
  f.presetPtr = nullptr;
  f.condition = nullptr;
}

void LiveFormBuilder::addConditionalSubheading(bool (*condition)(), const String& text) {
  if (_fieldCount >= LF_MAX_FIELDS) return;
  LiveField& f = _fields[_fieldCount++];
  f.type = LF_SUBHEADING;
  f.label = text;
  f.presetPtr = nullptr;
  f.condition = condition;
}

void LiveFormBuilder::addSeparator() {
  if (_fieldCount >= LF_MAX_FIELDS) return;
  LiveField& f = _fields[_fieldCount++];
  f.type = LF_SEPARATOR;
  f.presetPtr = nullptr;
  f.condition = nullptr;
}

void LiveFormBuilder::addColorPicker(const String& label, const String& field, int* preset) {
  if (_fieldCount >= LF_MAX_FIELDS) return;
  LiveField& f = _fields[_fieldCount++];
  f.type = LF_COLORPICKER;
  f.label = label;
  f.fieldName = field;
  f.presetPtr = preset;
  f.offset = 0;
  f.condition = nullptr;
}

void LiveFormBuilder::addConditionalDropDown(bool (*condition)(),
                                              const String& label, const String& field,
                                              const String& options, int* preset) {
  if (_fieldCount >= LF_MAX_FIELDS) return;
  LiveField& f = _fields[_fieldCount++];
  f.type = LF_DROPDOWN;
  f.label = label;
  f.fieldName = field;
  f.presetPtr = preset;
  f.offset = 0;
  f.condition = condition;
  parseOptions(options, f.options, f.optionCount);
}

void LiveFormBuilder::addTextInput(const String& label, const String& field,
                                    const String& placeholder, int maxLength,
                                    const String& buttonLabel) {
  if (_fieldCount >= LF_MAX_FIELDS) return;
  LiveField& f = _fields[_fieldCount++];
  f.type = LF_TEXTINPUT;
  f.label = label;
  f.fieldName = field;
  f.placeholder = placeholder;
  f.maxVal = maxLength;
  f.buttonLabel = buttonLabel;
  f.presetPtr = nullptr;
  f.condition = nullptr;
}

// -- Client handling -----------------------------------------------------

void LiveFormBuilder::handleClient() {
  if (!_server || !_server->hasClient()) return;

  WiFiClient client = _server->accept();
  if (!client) return;
  if (!client.available()) { client.stop(); return; }

  client.setTimeout(100);
  String req = client.readStringUntil('\r');
  client.readString();

  if (req.indexOf("/?save=1") >= 0) {
    handleSave(client);
    return;
  }

  if (req.indexOf("/?field=") >= 0) {
    handleAjax(client, req);
    return;
  }

  serveForm(client);
}

void LiveFormBuilder::sendOK(WiFiClient& client) {
  String r = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 2\r\nConnection: close\r\n\r\nOK";
  client.print(r);
  client.flush();
  client.stop();
}

void LiveFormBuilder::handleSave(WiFiClient& client) {
  if (_saveCb) _saveCb();
  sendOK(client);
}

void LiveFormBuilder::handleAjax(WiFiClient& client, const String& req) {
  int fi  = req.indexOf("field=")  + 6;
  int vi  = req.indexOf("value=");
  int fe  = req.indexOf('&', fi);
  int fe2 = req.indexOf(' ', fi);
  if (fe < 0 || fe2 < fe) fe = fe2;
  String field = req.substring(fi, fe);
  String value = "";
  if (vi >= 0) {
    int vs = vi + 6;
    int ve = req.indexOf(' ', vs);
    value  = req.substring(vs, ve);
  }

  // Check for text input fields first
  for (int i = 0; i < _fieldCount; i++) {
    if (_fields[i].fieldName == field && _fields[i].type == LF_TEXTINPUT) {
      if (_textCb) _textCb(field, urlDecode(value));
      sendOK(client);
      return;
    }
  }

  int v = value.toInt();

  // Find matching field and update preset
  for (int i = 0; i < _fieldCount; i++) {
    LiveField& f = _fields[i];
    if (f.fieldName == field && f.presetPtr != nullptr) {
      if (f.type == LF_RANGE) {
        *f.presetPtr = constrain(v, f.minVal, f.maxVal);
      }
      else if (f.type == LF_DROPDOWN_OFFSET) {
        *f.presetPtr = constrain(v, f.offset, f.optionCount - 1 + f.offset);
      }
      else if (f.type == LF_DROPDOWN) {
        *f.presetPtr = constrain(v, 0, f.optionCount - 1);
      }
      else if (f.type == LF_COLORPICKER) {
        *f.presetPtr = v;
      }
      break;
    }
  }

  if (_changeCb) _changeCb(field, v);

  sendOK(client);
}

// -- HTML generation -----------------------------------------------------

void LiveFormBuilder::genDropDown(String& h, int idx) {
  LiveField& f = _fields[idx];
  int sel = f.presetPtr ? *f.presetPtr : 0;

  h += "<div class=\"fg\"><label class=\"fl\">";
  h += f.label;
  h += "</label><select onchange=\"send('";
  h += f.fieldName;
  h += "',this.value)\">";

  for (int i = 0; i < f.optionCount; i++) {
    int val = i + f.offset;
    h += "<option value=\"";
    h += val;
    h += "\"";
    if (val == sel) h += " selected";
    h += ">";
    h += f.options[i];
    h += "</option>";
  }
  h += "</select></div>";
}

void LiveFormBuilder::genRange(String& h, int idx) {
  LiveField& f = _fields[idx];
  int val = f.presetPtr ? *f.presetPtr : f.minVal;

  h += "<div class=\"fg\"><label class=\"fl\">";
  h += f.label;
  h += "</label><div class=\"rc\">";
  h += "<input type=\"range\" min=\""; h += f.minVal;
  h += "\" max=\""; h += f.maxVal;
  h += "\" value=\""; h += val;
  h += "\" id=\""; h += f.rangeId;
  h += "\" oninput=\"document.getElementById('"; h += f.rangeId;
  h += "_v').textContent=this.value;send('";
  h += f.fieldName; h += "',this.value)\">";
  h += "<span class=\"rv\" id=\""; h += f.rangeId; h += "_v\">";
  h += val; h += "</span>";
  h += "</div></div>";
}

void LiveFormBuilder::genSubheading(String& h, int idx) {
  h += "<h2 class=\"sh\">";
  h += _fields[idx].label;
  h += "</h2>";
}

void LiveFormBuilder::genSeparator(String& h) {
  h += "<div class=\"sep\"></div>";
}

void LiveFormBuilder::genColorPicker(String& h, int idx) {
  LiveField& f = _fields[idx];
  int val = f.presetPtr ? *f.presetPtr : 0;
  char hex[8];
  snprintf(hex, sizeof(hex), "#%06X", val & 0xFFFFFF);

  h += "<div class=\"fg\"><label class=\"fl\">";
  h += f.label;
  h += "</label><input type=\"color\" value=\"";
  h += hex;
  h += "\" onchange=\"send('";
  h += f.fieldName;
  h += "',parseInt(this.value.substring(1),16))\"></div>";
}

void LiveFormBuilder::genTextInput(String& h, int idx) {
  LiveField& f = _fields[idx];
  h += "<div class=\"fg\"><label class=\"fl\">";
  h += f.label;
  h += "</label><div class=\"tr\">";
  h += "<input type=\"text\" id=\"";
  h += f.fieldName;
  h += "\" maxlength=\"";
  h += f.maxVal;
  h += "\" placeholder=\"";
  h += f.placeholder;
  h += "\" autocomplete=\"off\" autocorrect=\"off\" autocapitalize=\"off\" spellcheck=\"false\">";
  h += "<button type=\"button\" class=\"sb\" onclick=\"sendText('";
  h += f.fieldName;
  h += "')\">";
  h += f.buttonLabel;
  h += "</button></div></div>";
}

void LiveFormBuilder::serveForm(WiFiClient& client) {
  String h;
  h.reserve(8500);

  h += "<!DOCTYPE html><html lang=\"en\"><head>";
  h += "<meta charset=\"UTF-8\">";
  h += "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">";
  h += "<title>"; h += _title; h += "</title>";
  h += "<style>";
  h += ":root{--pc:#2563eb;--ph:#1d4ed8;--sc:#059669;--bg:#f8fafc;--cb:#ffffff;--tp:#1e293b;--br:#e2e8f0;--bf:#3b82f6;--sl:0 10px 15px -3px rgba(0,0,0,.1);}";
  h += "*{box-sizing:border-box;}";
  h += "body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:linear-gradient(135deg,var(--bg),#e2e8f0);margin:0;padding:20px;color:var(--tp);line-height:1.6;}";
  h += "#container{max-width:800px;margin:0 auto;background:var(--cb);border-radius:16px;box-shadow:var(--sl);overflow:hidden;}";
  h += "#header{background:linear-gradient(135deg,var(--pc),var(--ph));color:#fff;text-align:center;font-size:1.7rem;font-weight:700;margin:0;letter-spacing:-.5px;border-radius:16px 16px 0 0;padding:15px;display:flex;flex-direction:column;align-items:center;justify-content:center;}";
  h += ".st{font-size:0.55em;font-weight:400;opacity:0.9;}";
  h += "#inputs{padding:20px 40px;margin-top:0;}";
  h += ".fg{margin-bottom:24px;}";
  h += ".fl{display:block;font-size:1.1rem;font-weight:500;margin-bottom:8px;}";
  h += "select{width:100%;height:48px;padding:12px;font-size:1.1rem;border:2px solid var(--br);border-radius:8px;background:var(--cb);outline:none;transition:all 0.2s ease;}";
  h += "select:focus{border-color:var(--bf);}";
  h += ".rc{display:flex;align-items:center;gap:12px;}";
  h += "input[type=range]{flex:1;height:6px;appearance:none;background:var(--br);border-radius:3px;outline:none;}";
  h += "input[type=range]::-webkit-slider-thumb{appearance:none;width:20px;height:20px;background:var(--pc);border-radius:50%;cursor:pointer;}";
  h += "input[type=color]{width:100%;height:48px;padding:2px;border:2px solid var(--br);border-radius:8px;background:var(--cb);cursor:pointer;appearance:none;-webkit-appearance:none;}";
  h += "input[type=color]::-webkit-color-swatch-wrapper{padding:0;}";
  h += "input[type=color]::-webkit-color-swatch{border:none;border-radius:6px;}";
  h += ".rv{background:var(--pc);color:#fff;padding:3px 10px;border-radius:10px;font-weight:600;min-width:36px;text-align:center;}";
  h += ".sh{font-size:1.5rem;font-weight:600;color:var(--tp);margin:15px 0 20px 0;padding-bottom:10px;border-bottom:2px solid var(--br);}";
  h += ".sep{width:100%;height:1px;background:var(--br);margin:24px 0;}";
  h += ".tr{display:flex;gap:10px;align-items:center;}";
  h += ".tr input[type=text]{flex:1;height:48px;padding:12px;font-size:1.1rem;border:2px solid var(--br);border-radius:8px;background:var(--cb);outline:none;}";
  h += ".tr input[type=text]:focus{border-color:var(--bf);box-shadow:0 0 0 3px rgba(59,130,246,0.1);}";
  h += ".sb{height:48px;padding:0 20px;font-size:1.1rem;font-weight:600;color:#fff;background:var(--pc);border:none;border-radius:8px;cursor:pointer;white-space:nowrap;transition:background 0.2s;}";
  h += ".sb:active{background:var(--ph);}";
  h += ".save-btn{width:100%;padding:20px;font-size:1.2rem;font-weight:600;color:#fff;background:linear-gradient(135deg,var(--sc),#047857);border:none;border-radius:12px;cursor:pointer;margin-top:20px;box-shadow:0 4px 6px -1px rgba(0,0,0,.1);}";
  h += ".save-btn:active{opacity:.85;}";
  h += ".saved-overlay{position:fixed;top:50%;left:50%;transform:translate(-50%,-50%);background:rgba(5,150,105,0.95);color:white;padding:30px 50px;border-radius:16px;font-size:1.6rem;font-weight:700;box-shadow:0 20px 40px rgba(0,0,0,0.3);z-index:1000;transition:opacity 0.6s ease;pointer-events:none;white-space:nowrap;}";
  h += "@media(max-width:600px){body{padding:10px;}#inputs{padding:18px;}}";
  h += "</style></head><body>";
  h += "<div id=\"container\">";
  h += "<h1 id=\"header\">"; h += _title;
  if (_subtitle.length() > 0) {
    h += "<br><span class=\"st\">"; h += _subtitle; h += "</span>";
  }
  h += "</h1>";
  h += "<div id=\"inputs\">";

  // Generate all fields
  for (int i = 0; i < _fieldCount; i++) {
    LiveField& f = _fields[i];
    if (f.condition != nullptr && !f.condition()) continue;

    switch (f.type) {
      case LF_DROPDOWN:
      case LF_DROPDOWN_OFFSET:
        genDropDown(h, i);
        break;
      case LF_RANGE:
        genRange(h, i);
        break;
      case LF_SUBHEADING:
        genSubheading(h, i);
        break;
      case LF_SEPARATOR:
        genSeparator(h);
        break;
      case LF_COLORPICKER:
        genColorPicker(h, i);
        break;
      case LF_TEXTINPUT:
        genTextInput(h, i);
        break;
    }
  }

  h += "<div class=\"sep\"></div>";
  h += "<button type=\"button\" class=\"save-btn\" onclick=\"save()\">Save Settings</button>";
  h += "</div></div>";
  h += "<script>";
  h += "function send(f,v){fetch('/?field='+f+'&value='+v)}";
  h += "function sendText(id){var m=document.getElementById(id);if(m&&m.value.trim()!=''){fetch('/?field='+id+'&value='+encodeURIComponent(m.value.trim()));m.focus();}}";
  h += "function save(){fetch('/?save=1').then(()=>{";
  h += "var o=document.createElement('div');o.className='saved-overlay';";
  h += "o.textContent='✓ Settings Saved';";
  h += "document.body.appendChild(o);";
  h += "setTimeout(()=>{o.style.opacity='0';setTimeout(()=>o.remove(),600);},2400);})}";
  h += "</script></body></html>";

  String headers = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: ";
  headers += h.length();
  headers += "\r\nConnection: close\r\n\r\n";
  client.print(headers);
  client.print(h);
  client.flush();
  client.stop();
}
