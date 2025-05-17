#include "heartbeat.h"

HeartBeat::~HeartBeat()
{
}

HeartBeat::HeartBeat(String deviceID)
{
    this->deviceID = deviceID;
}

String HeartBeat::printUptime()
{
    unsigned long ms = millis();
    unsigned long secs = ms / 1000;
    unsigned long mins = secs / 60;
    unsigned long hrs = mins / 60;

    secs %= 60;
    mins %= 60;

    return StringFormat("%02lu:%02lu:%02lu", hrs, mins, secs);
}

void HeartBeat::setSystemDeets(JsonObject &writer)
{
    writer["free"] = ESP.getFreeHeap();
    writer["mem"] = ESP.getHeapSize();
    String uptime = printUptime();
    writer["up"] = uptime;
    String version = String(BUILD_VERSION);
    writer["v"] = version;
}
void HeartBeat::setPowerDeets(JsonObject &writer)
{
    float vCel = FuelGauge.getVCell();
    writer["v_cel"] = vCel;
    float soc = FuelGauge.getCurrent_mA();
    writer["current"] = soc;
    float bat = FuelGauge.getNormalizedSoC();
    writer["bat"] = bat;
    float s_v = FuelGauge.getSolarVCell();
    writer["solar_v"] = s_v;
}

void HeartBeat::setCellDeets(JsonObject &writer)
{
    if (Hyphen.hyConnect().getConnection().getClass() != ConnectionClass::CELLULAR)
    {
        return;
    }
    Cellular &cellConn = (Cellular &)Hyphen.hyConnect().getConnection();
    String ip = cellConn.getLocalIP();
    ip.replace("+IPADDR: ", "");
    writer["IMEI"] = cellConn.getIMEI();
    writer["IMSI"] = cellConn.getIMSI();
    writer["l_ip"] = ip;
    writer["op"] = cellConn.getOperator();
    writer["prov"] = cellConn.getProvider();
    writer["n_mode"] = cellConn.getNetworkMode();
    writer["sig_q"] = cellConn.getSignalQuality();
    writer["ccid"] = cellConn.getSimCCID();
    writer["temp"] = cellConn.getTemperature();
    writer["sig"] = cellConn.getSignalQuality();
    writer["modem"] = cellConn.getModem().getModemModel();
    writer["net"] = cellConn.getNetworkMode();
}
void HeartBeat::setNetwordDeets(JsonObject &writer)
{
    if (Hyphen.hyConnect().getConnection().getClass() != ConnectionClass::WIFI)
    {
        return;
    }
    WiFiConnection &cellConn = (WiFiConnection &)Hyphen.hyConnect().getConnection();
    NetworkInfo info = cellConn.getNetworkInfo();
    writer["ssid"] = info.ssid;
    writer["bssid"] = info.bssid;
    writer["rssi"] = info.rssi;
    writer["channel"] = info.channel;
    writer["encryp"] = info.encryption;
    writer["l_ip"] = info.localIP.toString();
    writer["g_ip"] = info.gatewayIP.toString();
    writer["mask"] = info.subnetMask.toString();
    writer["dns1"] = info.dns1.toString();
    writer["dns2"] = info.dns2.toString();
}

String HeartBeat::pump()
{
    // memset(buf, 0, sizeof(buf));
    JsonDocument doc;
    JsonObject network = doc["network"].to<JsonObject>();
    setNetwordDeets(network);
    // BufferManager::WRITE_BUFFER,
    // JSONBufferWriter writer(BufferManager::WRITE_BUFFER, HEART_BUFFER_SIZE - 1);
    doc["device"] = this->deviceID;
    doc["date"] = Time.now(); // Time.format(Time.now(), TIME_FORMAT_ISO8601_FULL);

    // writer.beginObject();
    JsonObject cellDetails = doc["cell"].to<JsonObject>();
    setCellDeets(cellDetails);
    JsonObject pwrDetails = doc["pow"].to<JsonObject>();
    setPowerDeets(pwrDetails);
    JsonObject sysDetails = doc["sys"].to<JsonObject>();
    setSystemDeets(sysDetails);

    String pump;
    serializeJson(doc, pump);

    return pump;
}