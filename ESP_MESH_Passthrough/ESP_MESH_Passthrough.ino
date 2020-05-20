#include <ESP8266WiFi.h>
#include <ESP8266WiFiMesh.h>
#include <TypeConversionFunctions.h>
#include <assert.h>

#define debugJumper 0

const char exampleMeshName[] PROGMEM = "MeshNode_";
const char exampleWiFiPassword[] PROGMEM = "ChangeThisWiFiPassword_TODO";

String manageRequest(const String &request, ESP8266WiFiMesh &meshInstance);
transmission_status_t manageResponse(const String &response, ESP8266WiFiMesh &meshInstance);
void networkFilter(int numberOfNetworks, ESP8266WiFiMesh &meshInstance);

ESP8266WiFiMesh meshNode = ESP8266WiFiMesh(manageRequest, manageResponse, networkFilter, FPSTR(exampleWiFiPassword), FPSTR(exampleMeshName), "", debugJumper); //turn off verbose with false

String manageRequest(const String &request, ESP8266WiFiMesh &meshInstance) {
  Serial.println(request);
  return ("request_" + meshInstance.getNodeID());
}

transmission_status_t manageResponse(const String &response, ESP8266WiFiMesh &meshInstance) {
  transmission_status_t statusCode = TS_TRANSMISSION_COMPLETE;
  meshInstance.setMessage("response" + meshInstance.getNodeID());
  return statusCode;
}

void networkFilter(int numberOfNetworks, ESP8266WiFiMesh &meshInstance) {
  for (int networkIndex = 0; networkIndex < numberOfNetworks; ++networkIndex) {
    String currentSSID = WiFi.SSID(networkIndex);
    int meshNameIndex = currentSSID.indexOf(meshInstance.getMeshName());
    if (meshNameIndex >= 0) {
      uint64_t targetNodeID = stringToUint64(currentSSID.substring(meshNameIndex + meshInstance.getMeshName().length()));
      ESP8266WiFiMesh::connectionQueue.push_back(NetworkInfo(networkIndex));
    }
  }
}

void setup() {
  WiFi.persistent(false);

  Serial.begin(9600);
  delay(50);

  WiFi.disconnect();

  if (debugJumper) {
    Serial.println(F("Setting up mesh node..."));
  }

  meshNode.begin();
  meshNode.activateAP();
  meshNode.setStaticIP(IPAddress(192, 168, 4, 21));
}

void loop() {
  if (Serial.available()) {
    String request = Serial.readString();
    meshNode.attemptTransmission(request, true); //cameron

    if (debugJumper) {
      if (ESP8266WiFiMesh::latestTransmissionSuccessful()) {
        Serial.println(F("Transmission successful."));
      }
      if (ESP8266WiFiMesh::latestTransmissionOutcomes.empty()) {
        Serial.println(F("No mesh AP found."));
      } else {
        for (TransmissionResult &transmissionResult : ESP8266WiFiMesh::latestTransmissionOutcomes) {
          if (transmissionResult.transmissionStatus == TS_TRANSMISSION_FAILED) {
            Serial.println(String(F("Transmission failed to mesh AP ")) + transmissionResult.SSID);
          } else if (transmissionResult.transmissionStatus == TS_CONNECTION_FAILED) {
            Serial.println(String(F("Connection failed to mesh AP ")) + transmissionResult.SSID);
          } else if (transmissionResult.transmissionStatus == TS_TRANSMISSION_COMPLETE) {
            // No need to do anything, transmission was successful.
          } else {
            Serial.println(String(F("Invalid transmission status for ")) + transmissionResult.SSID + String(F("!")));
            assert(F("Invalid transmission status returned from responseHandler!") && false);
          }
        }
      }
    }
  }
  /* Accept any incoming connections */
  meshNode.acceptRequest();
}
