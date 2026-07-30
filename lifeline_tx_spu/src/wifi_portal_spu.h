#ifndef WIFI_PORTAL_SPU_H
#define WIFI_PORTAL_SPU_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include "ConfigSPU.h"

struct WiFiNetworkSPU {
    String ssid;
    String password;
};

class WiFiPortalSPU {
public:
    WiFiPortalSPU();
    void begin();
    void update();
    bool connectToWiFi();
    bool connectToWiFiSilent();
    void startWiFiPortal();
    void stopWiFiPortal();
    void checkWiFiPortalButton();

    bool isWiFiConnected() const { return _wifiConnected; }
    bool isPortalActive() const { return _portalActive; }
    String getActiveSSID() const { return _activeSSID; }

private:
    WebServer _server;
    DNSServer _dnsServer;
    Preferences _preferences;

    bool _wifiConnected;
    bool _portalActive;
    unsigned long _portalStartTime;
    unsigned long _buttonPressStartTime;
    bool _buttonPressed;
    int _lastBeepSecond;

    WiFiNetworkSPU _storedNetworks[MAX_WIFI_NETWORKS];
    int _networkCount;
    String _activeSSID;

    void loadWiFiCredentials();
    void saveWiFiCredentialsList(const WiFiNetworkSPU nets[], int count);
    String getPortalHTML();
    void handlePortalRoot();
    void handlePortalSave();
};

extern WiFiPortalSPU wifiPortalSPU;

#endif // WIFI_PORTAL_SPU_H
