#include "BlueMagicCameraConnection.h"

BLERemoteCharacteristic *BlueMagicCameraConnection::_cameraStatus;
BLERemoteCharacteristic *BlueMagicCameraConnection::_deviceName;
BLERemoteCharacteristic *BlueMagicCameraConnection::_timecode;
BLERemoteCharacteristic *BlueMagicCameraConnection::_outgoingCameraControl;
BLERemoteCharacteristic *BlueMagicCameraConnection::_incomingCameraControl;

static BLEUUID OutgoingCameraControl("5DD3465F-1AEE-4299-8493-D2ECA2F8E1BB");
static BLEUUID IncomingCameraControl("B864E140-76A0-416A-BF30-5876504537D9");
static BLEUUID Timecode("6D8F2110-86F1-41BF-9AFB-451D87E976C8");
static BLEUUID CameraStatus("7FE8691D-95DC-4FC5-8ABD-CA74339B51B9");
static BLEUUID DeviceName("FFAC0C52-C9FB-41A0-B063-CC76282EB89C");

static BLEUUID ServiceId("00001800-0000-1000-8000-00805f9b34fb");
static BLEUUID BmdCameraService("291D567A-6D75-11E6-8B77-86F30CA893D3");

static void cameraStatusNotify(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
  BlueMagicState *blu = BlueMagicState::getInstance();
  blu->statusNotify(true, pData);
  blu->setCameraStatus(pData[0]);
}

static void timeCodeNotify(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
  BlueMagicState *blu = BlueMagicState::getInstance();
  blu->timecodeNotify(true, pData);
  // timecode
  uint8_t H, M, S, f;
  H = (pData[11] / 16 * 10) + (pData[11] % 16);
  M = (pData[10] / 16 * 10) + (pData[10] % 16);
  S = (pData[9] / 16 * 10) + (pData[9] % 16);
  f = (pData[8] / 16 * 10) + (pData[8] % 16);
  blu->setTimecode(H, M, S, f);
}

static void controlNotify(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
  BlueMagicState *blu = BlueMagicState::getInstance();
  blu->settingsNotify(true, pData);
  bool changed = false;

  // recording
  if (length == 13 && pData[0] == 255 && pData[1] == 9 && pData[4] == 10 && pData[5] == 1)
  {
    changed = true;
    int8_t transportMode = pData[8];
    blu->setTransportMode(transportMode);
  }

  //codec
  if (pData[0] == 255 && pData[4] == 10 && pData[5] == 0)
  {
    changed = true;
    int8_t codec = pData[8];
    int8_t quality = pData[9];
    blu->setCodec(codec);
    blu->setQuality(quality);
  }

  //resolution + framerate
  if (pData[0] == 255 && pData[4] == 1 && pData[5] == 9)
  {
    changed = true;
    int16_t frL = pData[8];
    int16_t frH = pData[9] << 8;
    int16_t frameRate = frL + frH;

    int16_t sfrL = pData[10];
    int16_t sfrH = pData[11] << 8;
    int16_t sensorRate = sfrL + sfrH;

    int16_t wL = pData[12];
    int16_t wH = pData[13] << 8;
    int16_t width = wL + wH;

    int16_t hL = pData[14];
    int16_t hH = pData[15] << 8;
    int16_t height = hL + hH;

    blu->setFrameRate(frameRate);
    blu->setSensorFrameRate(sensorRate);
    blu->setFrameWidth(width);
    blu->setFrameHeight(height);
  }

  // white balance
  if (pData[0] == 255 && pData[4] == 1 && pData[5] == 2)
  {
    changed = true;
    int16_t wbL = pData[8];
    int16_t wbH = pData[9] << 8;
    int16_t whiteBalance = wbL + wbH;

    int16_t tintL = pData[10];
    int16_t tintH = pData[11] << 8;
    int16_t tint = tintL + tintH;

    blu->setWhiteBalance(whiteBalance);
    blu->setTint(tint);
  }

  // zoom
  if (pData[0] == 255 && pData[4] == 0 && pData[5] == 7)
  {
    changed = true;
    int16_t zL = pData[8];
    int16_t zH = pData[9] << 8;
    int16_t zoom = zL + zH;
    blu->setZoom(zoom);
  }

  // aperture
  if (pData[0] == 255 && pData[4] == 0 && pData[5] == 2)
  {
    changed = true;
    uint16_t low = pData[8];
    uint16_t high = pData[9] << 8;
    float aperture = sqrt(pow(2, (float(low + high) / 2048.0)));
    blu->setAperture(aperture);
  }

  // iso
  if (pData[0] == 255 && pData[4] == 1 && pData[5] == 14)
  {
    changed = true;
    uint16_t low = pData[8];
    uint16_t high = pData[9] << 8;
    int32_t iso = low + high;
    blu->setIso(iso);
  }

  // shutter
  if (pData[0] == 255 && pData[4] == 1 && pData[5] == 11)
  {
    changed = true;
    uint16_t low = pData[8];
    uint16_t high = pData[9] << 8;
    int32_t shutter = low + high;
    blu->setShutter(shutter);
  }

  blu->setChanged(changed);
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{

  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    Serial.println("BLE Advertised Device found: ");

    if (advertisedDevice.haveServiceUUID() && advertisedDevice.getServiceUUID().equals(ServiceId))
    {
      advertisedDevice.getScan()->stop();
    }
    else
    {
      advertisedDevice.getScan()->erase(advertisedDevice.getAddress());
    }
  }
};

class MySecurity : public BLESecurityCallbacks
{
  uint32_t onPassKeyRequest()
  {
    // code snippet from jeppo7745 https://www.instructables.com/id/Magic-Button-4k-the-20USD-BMPCC4k-Remote/
    Serial.println("---> PLEASE ENTER 6 DIGIT PIN (end with ENTER) : ");
    int pinCode = 0;
    char ch;
    do
    {
      while (!Serial.available())
      {
        vTaskDelay(1);
      }
      ch = Serial.read();
      if (ch >= '0' && ch <= '9')
      {
        pinCode = pinCode * 10 + (ch - '0');
        Serial.print(ch);
      }
    } while ((ch != '\n'));
    return pinCode;
  }

  void onPassKeyNotify(uint32_t pass_key)
  {
    pass_key + 1;
  }

  bool onConfirmPIN(uint32_t pin)
  {
    return true;
  }

  bool onSecurityRequest()
  {
    return true;
  }

  void onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl)
  {
    int index = 1;
  }
};

BlueMagicCameraConnection::BlueMagicCameraConnection()
{
}

BlueMagicCameraConnection::~BlueMagicCameraConnection()
{
  delete _cameraStatus, _deviceName, _timecode, _outgoingCameraControl, _incomingCameraControl, _device;
  delete _client;
  delete _cameraControl;
  _init = false;
  _device.deinit(true);
}

void BlueMagicCameraConnection::begin(Preferences *pref)
{
  begin("BlueMagic32", pref);
}

void BlueMagicCameraConnection::begin(String name, Preferences *pref)
{

  if (_init)
  {
    return;
  }

  _name = name;
  setState(CAMERA_DISCONNECTED);

  if (!PREF_INCLUDED)
  {
    _pref = pref;
  }

  _pref->begin(_name.c_str(), false);

  setAuthentication(_pref->getBool("authenticated", false));
  String addr = _pref->getString("authenticated", "");
  if (addr.length() > 0)
  {
    setCameraAddress(BLEAddress(addr.c_str()));
  }

  _pref->end();

  _device.init(_name.c_str());
  _device.setPower(ESP_PWR_LVL_P9);
  _device.setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
  _device.setSecurityCallbacks(new MySecurity());

  BLESecurity *pSecurity = new BLESecurity();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
  pSecurity->setCapability(ESP_IO_CAP_IN);
  pSecurity->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

  _init = true;
}

bool BlueMagicCameraConnection::scan(bool active, int duration)
{
  if (getAuthentication() && getCameraAddress() != nullptr)
  {
    return false;
  }
  else
  {
    _bleScan = _device.getScan();
    _bleScan->clearResults();
    _bleScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    _bleScan->setActiveScan(active);
    _bleScan->start(duration);
  }
  return true;
}

int BlueMagicCameraConnection::connected()
{
  return _connected;
}

bool BlueMagicCameraConnection::available()
{
  return connected() && (_cameraControl != nullptr);
}

void BlueMagicCameraConnection::setState(CONNECTION_STATE state)
{
  _connected = state;
}

bool BlueMagicCameraConnection::connectToServer(BLEAddress address)
{
  _client = _device.createClient();
  setState(CAMERA_CONNECTING);
  _client->connect(address);

  BLERemoteService *pRemoteService = _client->getService(BmdCameraService);
  if (pRemoteService == nullptr)
  {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(BmdCameraService.toString().c_str());
    return false;
  }

  _deviceName = pRemoteService->getCharacteristic(DeviceName);
  if (_deviceName != nullptr)
  {
    _deviceName->writeValue(_name.c_str(), _name.length());
  }

  _outgoingCameraControl = pRemoteService->getCharacteristic(OutgoingCameraControl);
  if (_outgoingCameraControl == nullptr)
  {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(OutgoingCameraControl.toString().c_str());
    return false;
  }

  _incomingCameraControl = pRemoteService->getCharacteristic(IncomingCameraControl);
  if (_incomingCameraControl == nullptr)
  {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(IncomingCameraControl.toString().c_str());
    return false;
  }
  _incomingCameraControl->registerForNotify(controlNotify, false);

  _timecode = pRemoteService->getCharacteristic(Timecode);
  if (_timecode == nullptr)
  {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(Timecode.toString().c_str());
    return false;
  }
  _timecode->registerForNotify(timeCodeNotify);

  _cameraStatus = pRemoteService->getCharacteristic(CameraStatus);
  if (_cameraStatus == nullptr)
  {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(CameraStatus.toString().c_str());
    return false;
  }
  _cameraStatus->registerForNotify(cameraStatusNotify);

  setState(CAMERA_CONNECTED);
  setController();
  return true;
}

void BlueMagicCameraConnection::setController()
{
  _cameraControl = new BlueMagicCameraController(_outgoingCameraControl);
}

void BlueMagicCameraConnection::setAuthentication(bool authenticated)
{
  _authenticated = authenticated;
}

bool BlueMagicCameraConnection::getAuthentication()
{
  return _authenticated;
}

void BlueMagicCameraConnection::setCameraAddress(BLEAddress address)
{
  _cameraAddress = &address;
}

BLEAddress *BlueMagicCameraConnection::getCameraAddress()
{
  return _cameraAddress;
}

BlueMagicCameraController *BlueMagicCameraConnection::connect()
{
  return connect(0);
}

BlueMagicCameraController *BlueMagicCameraConnection::connect(uint8_t index)
{
  if (_cameraControl != nullptr)
  {
    return _cameraControl;
  }

  bool ok;
  bool scanned = scan(false, 5);

  BLEAddress address = BLEAddress("FF:FF:FF:FF:FF");

  if (scanned)
  {
    address = _bleScan->getResults().getDevice(index).getAddress();
    ok = connectToServer(address);
  }
  else
  {
    address = *getCameraAddress();
    ok = connectToServer(address);
  }

  if (ok)
  {
    setAuthentication(true);
    _pref->begin(_name.c_str(), false);
    _pref->putString("cameraAddress", address.toString().c_str());
    _pref->putBool("authenticated", getAuthentication());
    _pref->end();
    setCameraAddress(address);
    return _cameraControl;
  }

  return nullptr;
}

void BlueMagicCameraConnection::disconnect()
{
  _client->disconnect();
  delete _cameraControl;
  _cameraControl = nullptr;
  setState(CAMERA_DISCONNECTED);
}

void BlueMagicCameraConnection::clearPairing()
{
  if (connected() != CAMERA_DISCONNECTED)
  {
    disconnect();
  }
  if (*getCameraAddress()->getNative() != nullptr)
  {
    esp_ble_remove_bond_device(*getCameraAddress()->getNative());
  }
  esp_ble_remove_bond_device(*BLEAddress("90:fd:9f:c1:7b:4b").getNative());
  setAuthentication(false);
  // setCameraAddress(nullptr);
  _pref->begin(_name.c_str(), false);
  _pref->putString("cameraAddress", "");
  _pref->putBool("authenticated", getAuthentication());
  _pref->end();
}
