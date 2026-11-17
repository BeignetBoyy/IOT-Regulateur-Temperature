#include <WiFi.h>
#include <WiFiMulti.h>

#define SaveDisconnectTime 1000
#define WiFiMaxTry 10
String translateEncryptionType(wifi_auth_mode_t encryptionType);
void wifi_printstatus(int);
void wifi_connect_basic(String hostname, String ssid, String passwd);
int wifi_search_neighbor();
void wifi_connect_multi(String hostname);
