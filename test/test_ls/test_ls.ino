#define SDPin 9
#define myport Serial
#include <SD.h>

File root;
uint8_t cIndex = 2;
uint8_t nFiles;
// char * file_list[32];

void setup() {
  myport.begin(115200);
  initSD();
  // char* a = "hello";
  // for(int i=0;i<5;i++){
  //     Serial.println(*(a+i));
  // }
  /*convert const char*(e.g. String.c_str() SD.open(file).name()) to char* */
  // char *a[10];
  // String t = "filename";
  // for(uint8_t i=0; i<5; i++){
  //     a[i] = const_cast<char*>(t.c_str());
  // }
  // for(int i=0; i<10; i++){
  //     Serial.printf("%s ", a[i]);
  //     Serial.println(String(a[i]));
  // }
}

void loop() {
}

void initSD() {
    myport.print(F("Loading SD card: "));
    if (!SD.begin(SDPin)) {
        myport.println(F("failed, maybe not inserted."));
        return;
    }
    myport.println(F("done!"));
    root = SD.open("/");
    nFiles = listFile(root, 0, 0);
    myport.printf("ls finished, found %d cmd files.\n", nFiles);
    // myport.printf("current file: %s\n", file_list[cIndex]);
}

uint8_t listFile(File r, uint8_t count, uint8_t numTabs) {
    // please use '    ' instead of '\t' because in some cases length of '\t' is uncertainty
    if (!r) {
        myport.println(F("\nlistFile error: dir unreadable, terminated.\n"));
        return count;
    }
    delay(200);
    if (numTabs == 0) myport.println(r.name());
    while (true) {
        File file = r.openNextFile();
        if (!file) break;
        delay(200);
        for (uint8_t i = 0; i < numTabs; i++) myport.print("|   ");
        if (file.isDirectory()) {
            myport.printf("|---%s/\n", file.name());
            count = listFile(file, count, numTabs + 1);
        }
        else {
            String temp = file.name();
            temp.toLowerCase();
            if (temp.endsWith(".cmd")) {
                if (cIndex == count)
                    myport.printf("| > %2d ", count);
                else
                    myport.printf("|   %2d ", count);
                myport.println(temp + "  " + String(file.size()/1024.0) + "KB");
                // TODO debug this
                // file_list[count] = const_cast<char*>(temp.c_str());
                count++;
            }
            else myport.println("|   " + temp);
        }
        file.close();
    }
    return count;
}
