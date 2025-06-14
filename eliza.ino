// Full ELIZA structure in Arduino C++ using SD card (based on doctor.txt)
// Requires: SD card module, doctor.txt stored in SD root

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <vector>

struct Decomp {
  std::vector<String> parts;
  bool save;
  std::vector<std::vector<String>> reasmbs;
  int next_reasmb_index;
};

struct Key {
  String word;
  int weight;
  std::vector<Decomp> decomps;
};

class Eliza {
public:
  std::vector<String> initials;
  std::vector<String> finals;
  std::vector<String> quits;
  std::vector<Key> keys;
  std::vector<std::vector<String>> memory;

  void setup() {
    Serial.begin(9600);
    while (!Serial);

    if (!SD.begin(10)) {
      Serial.println("SD card initialization failed!");
      while (true);
    }

    load("doctor.txt");
    Serial.println(initial());
  }

  void loop() {
    if (Serial.available()) {
      String input = Serial.readStringUntil('\n');
      input.trim();
      String reply = respond(input);
      if (reply.length() == 0) {
        Serial.println(final());
        while (true);
      } else {
        Serial.println(reply);
      }
    }
  }

  void load(String path) {
    File file = SD.open(path);
    if (!file) {
      Serial.println("Failed to open doctor.txt");
      return;
    }

    Key* currentKey = nullptr;
    Decomp* currentDecomp = nullptr;

    while (file.available()) {
      String line = file.readStringUntil('\n');
      line.trim();
      if (line.length() == 0) continue;

      int colonIndex = line.indexOf(':');
      if (colonIndex == -1) continue;
      String tag = line.substring(0, colonIndex);
      String content = line.substring(colonIndex + 1);
      content.trim();

      if (tag == "initial") initials.push_back(content);
      else if (tag == "final") finals.push_back(content);
      else if (tag == "quit") quits.push_back(content);
      else if (tag == "key") {
        currentKey = new Key();
        int spaceIdx = content.indexOf(' ');
        currentKey->word = (spaceIdx > 0) ? content.substring(0, spaceIdx) : content;
        currentKey->weight = (spaceIdx > 0) ? content.substring(spaceIdx + 1).toInt() : 1;
        keys.push_back(*currentKey);
      } else if (tag == "decomp" && currentKey != nullptr) {
        currentDecomp = new Decomp();
        currentDecomp->save = false;
        std::vector<String> parts;
        int start = 0;
        content.trim();
        if (content.startsWith("$")) {
          currentDecomp->save = true;
          content = content.substring(1);
          content.trim();
        }
        while (true) {
          int space = content.indexOf(' ', start);
          if (space == -1) {
            parts.push_back(content.substring(start));
            break;
          }
          parts.push_back(content.substring(start, space));
          start = space + 1;
        }
        currentDecomp->parts = parts;
        currentDecomp->next_reasmb_index = 0;
        keys.back().decomps.push_back(*currentDecomp);
      } else if (tag == "reasmb" && currentDecomp != nullptr) {
        std::vector<String> re;
        int start = 0;
        while (true) {
          int space = content.indexOf(' ', start);
          if (space == -1) {
            re.push_back(content.substring(start));
            break;
          }
          re.push_back(content.substring(start, space));
          start = space + 1;
        }
        keys.back().decomps.back().reasmbs.push_back(re);
      }
    }

    file.close();
  }

  String initial() {
    return initials[0];
  }

  String final() {
    return finals[0];
  }

  String respond(String input) {
    input.toLowerCase();
    for (String quit : quits) {
      if (input.indexOf(quit) >= 0) {
        return "";
      }
    }

    std::vector<String> words = split(input, ' ');

    for (Key &key : keys) {
      for (Decomp &decomp : key.decomps) {
        std::vector<std::vector<String>> results;
        if (_match_decomp(decomp.parts, words, results)) {
          std::vector<String> reasmb = _next_reasmb(decomp);
          return join(reasmb, ' ');
        }
      }
    }

    return "Please go on.";
  }

  bool _match_decomp(std::vector<String> parts, std::vector<String> words, std::vector<std::vector<String>> &results) {
    if (parts.size() == 1 && parts[0] == "*") {
      results.push_back(words);
      return true;
    }
    return false;
  }

  std::vector<String> _next_reasmb(Decomp &decomp) {
    int idx = decomp.next_reasmb_index % decomp.reasmbs.size();
    decomp.next_reasmb_index++;
    return decomp.reasmbs[idx];
  }

  std::vector<String> split(String s, char delim) {
    std::vector<String> result;
    String token = "";
    for (int i = 0; i < s.length(); i++) {
      char c = s.charAt(i);
      if (c == delim) {
        if (token.length() > 0) result.push_back(token);
        token = "";
      } else {
        token += c;
      }
    }
    if (token.length() > 0) result.push_back(token);
    return result;
  }

  String join(std::vector<String> parts, char delim) {
    String result = "";
    for (int i = 0; i < parts.size(); i++) {
      result += parts[i];
      if (i < parts.size() - 1) result += delim;
    }
    return result;
  }
};

Eliza eliza;

void setup() {
  eliza.setup();
}

void loop() {
  eliza.loop();
}
