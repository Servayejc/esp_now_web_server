#include <LittleFS.h>

enum JsonState { START, OBJECT, ARRAY, KEY, COLON, VALUE, STRING, NUMBER, LITERAL };

  int position = 0;
  int line = 1;

bool validateJsonRFC8259(Stream &input) {
  const int MAX_DEPTH = 32;  // Limit nesting
  char stack[MAX_DEPTH];
  int depth = 0;
  JsonState state = START;
  bool inString = false;
  bool escape = false;
  bool expectingKey = false;
  bool expectingValue = false;
  char literalBuffer[6] = {0};
  int literalIndex = 0;



  while (input.available()) {
    char c = input.read();
    position++;
    
    if(c == '\n'){
      line++;
      position = 0;
    }
 
    if (escape) {
      escape = false;
      continue;
    }

    if (c == '\\') {
      if (!inString) return false;
      escape = true;
      continue;
    }

    if (inString) {
      if (c == '"') {
        inString = false;
        if (state == KEY) {
          state = COLON;
          expectingKey = false;
        } else if (state == VALUE) {
          state = (depth > 0 && stack[depth - 1] == '{') ? KEY : VALUE;
        }
      }
      continue;
    }

    switch (c) {
      case '{':
        if (state == START || state == VALUE || state == ARRAY) {
          if (depth >= MAX_DEPTH) return false;
          stack[depth++] = '{';
          state = KEY;
          expectingKey = true;
        } else {
          return false;
        }
        break;

      case '}':
        if (depth == 0 || stack[depth - 1] != '{') return false;
        depth--;
        state = (depth > 0 && stack[depth - 1] == '{') ? KEY : VALUE;
        break;

      case '[':
        if (state == START || state == VALUE || state == ARRAY) {
          if (depth >= MAX_DEPTH) return false;
          stack[depth++] = '[';
          state = VALUE;
        } else {
          return false;
        }
        break;

      case ']':
        if (depth == 0 || stack[depth - 1] != '[') return false;
        depth--;
        state = (depth > 0 && stack[depth - 1] == '{') ? KEY : VALUE;
        break;

      case '"':
        if (state == KEY || state == VALUE || state == START) {
          inString = true;
        } else {
          return false;
        }
        break;

      case ':':
        if (state == COLON) {
          state = VALUE;
        } else {
          return false;
        }
        break;

      case ',':
        if (depth == 0) return false;
        if (stack[depth - 1] == '{') {
          state = KEY;
          expectingKey = true;
        } else if (stack[depth - 1] == '[') {
          state = VALUE;
        } else {
          return false;
        }
        break;

      case 't': case 'f': case 'n':

        if (state == VALUE) {
          literalIndex = 0;
          literalBuffer[literalIndex++] = c;
          while (input.available() && literalIndex < 5) {
            char lc = input.peek();
            if ((lc >= 'a' && lc <= 'z')) {
              literalBuffer[literalIndex++] = input.read();
            } else break;
          }
          literalBuffer[literalIndex] = '\0';
          if (strcmp(literalBuffer, "true") != 0 && strcmp(literalBuffer, "false") != 0 && strcmp(literalBuffer, "null") != 0) {
            return false;
          }
          state = (depth > 0 && stack[depth - 1] == '{') ? KEY : VALUE;
        } else {
          return false;
        }
        break;

      default:
        if ((c >= '0' && c <= '9') || c == '-') {
          if (state != VALUE) return false;
          // Basic number validation
          while (input.available()) {
            char nc = input.peek();
            if ((nc >= '0' && nc <= '9') || nc == '.' || nc == 'e' || nc == 'E' || nc == '+' || nc == '-') {
              input.read();
            } else break;
          }
          state = (depth > 0 && stack[depth - 1] == '{') ? KEY : VALUE;
        } else if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
          continue; // Ignore whitespace
        } else {
          return false;
        }
    }
  }

  return (depth == 0 && !inString && !escape && (state == VALUE || state == START));
}


bool validateJSON(){
  File file = LittleFS.open("/Bad.json","r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return false;
  }
  if(validateJsonRFC8259(file)){
    Serial.println("Valid JSON");
    file.close();
    return true;
  } else {
    Serial.print("Invalid JSON at line ");
    Serial.print(line);
    Serial.print(", position ");
    Serial.println(position);
    file.close();
    return false;
  }
}
 