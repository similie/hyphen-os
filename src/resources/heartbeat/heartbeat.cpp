#include "heartbeat.h"

HeartBeat::~HeartBeat()
{
}

HeartBeat::HeartBeat(String deviceID)
{
    this->deviceID = deviceID;
}

String HeartBeat::cellAccessTech(int rat)
{

    return String("ACCESS UNDEFINED");
}

void HeartBeat::setSystemDeets(JsonObject &writer)
{
    int freemem = (uint32_t)100;
    writer["freemem"] = freemem;
    int uptime = 100;
    writer["uptime"] = uptime;
    String version = "0.1";
    writer["version"] = version;
}
void HeartBeat::setPowerDeets(JsonObject &writer)
{
    float vCel = 100;
    writer["v_cel"] = vCel;
    float soc = 4.2;
    writer["SoC"] = soc;
    float bat = 100;
    writer["bat"] = bat;
}

void HeartBeat::setCellDeets(JsonObject &writer)
{
}

String HeartBeat::pump()
{
    // memset(buf, 0, sizeof(buf));
    JsonDocument doc;
    JsonObject network = doc["network"].to<JsonObject>();
    // BufferManager::WRITE_BUFFER,
    // JSONBufferWriter writer(BufferManager::WRITE_BUFFER, HEART_BUFFER_SIZE - 1);
    doc["device"] = this->deviceID;
    doc["date"] = Time.format(Time.now(), TIME_FORMAT_ISO8601_FULL);

    // writer.beginObject();
    JsonObject cellDetails = doc["cellular"].to<JsonObject>();
    setCellDeets(cellDetails);
    JsonObject pwrDetails = doc["power"].to<JsonObject>();
    setPowerDeets(pwrDetails);
    JsonObject sysDetails = doc["system"].to<JsonObject>();
    setSystemDeets(sysDetails);

    String pump;
    serializeJson(doc, pump);

    return pump;
}