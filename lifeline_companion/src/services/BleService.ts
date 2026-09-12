import { NUS_SERVICE_UUID, NUS_RX_UUID, NUS_TX_UUID, LifeLineDevice } from '../constants/ble';
import { stringToBase64, base64ToString } from '../utils/base64';

// Dynamic import safeguard to prevent crashes in Expo Go or Web
let BleManager: any = null;
let bleManagerInstance: any = null;

try {
  const blePlx = require('react-native-ble-plx');
  BleManager = blePlx.BleManager;
  bleManagerInstance = new BleManager();
} catch (err) {
  console.warn('[BLE Service] Native react-native-ble-plx is not available in this environment.');
}

export class BleService {
  private activeDevice: any = null;
  private notificationSubscription: any = null;
  private packetBuffer: string = '';

  public isNativeBleSupported(): boolean {
    return bleManagerInstance !== null;
  }

  public async requestPermissions(): Promise<boolean> {
    // In React Native on Android 12+, PermissionsAndroid handles BLUETOOTH_SCAN and CONNECT
    return true;
  }

  public startScan(
    onDeviceFound: (device: LifeLineDevice) => void,
    onError: (error: any) => void
  ): void {
    if (!bleManagerInstance) {
      onError(new Error('Native Bluetooth LE is not available in standard Expo Go. Use Simulator mode or build an Expo Dev Client.'));
      return;
    }

    try {
      bleManagerInstance.startDeviceScan(
        null,
        { allowDuplicates: false },
        (error: any, scannedDevice: any) => {
          if (error) {
            onError(error);
            return;
          }

          if (scannedDevice && (scannedDevice.name || scannedDevice.localName)) {
            const name = scannedDevice.name || scannedDevice.localName || 'Unknown LifeLine';
            if (name.includes('LifeLine') || name.includes('TX') || name.includes('RX')) {
              onDeviceFound({
                id: scannedDevice.id,
                name: name,
                rssi: scannedDevice.rssi || -70,
                isSimulated: false,
              });
            }
          }
        }
      );
    } catch (e) {
      onError(e);
    }
  }

  public stopScan(): void {
    if (bleManagerInstance) {
      try {
        bleManagerInstance.stopDeviceScan();
      } catch (e) {
        console.warn('Error stopping BLE scan:', e);
      }
    }
  }

  public async connect(
    deviceId: string,
    onPacketReceived: (rawPacket: string) => void,
    onDisconnected: () => void
  ): Promise<LifeLineDevice> {
    if (!bleManagerInstance) {
      throw new Error('Native BLE not supported in this runtime.');
    }

    this.stopScan();

    const device = await bleManagerInstance.connectToDevice(deviceId);
    this.activeDevice = device;

    await device.discoverAllServicesAndCharacteristics();

    // Listen for disconnection
    device.onDisconnected(() => {
      this.cleanup();
      onDisconnected();
    });

    // Subscribe to Nordic UART TX Characteristic (Device -> Phone notifications)
    this.notificationSubscription = device.monitorCharacteristicForService(
      NUS_SERVICE_UUID,
      NUS_TX_UUID,
      (error: any, characteristic: any) => {
        if (error) {
          console.warn('[BLE] Monitor error:', error);
          return;
        }

        if (characteristic?.value) {
          try {
            const decoded = base64ToString(characteristic.value);
            this.handleIncomingChunk(decoded, onPacketReceived);
          } catch (err) {
            console.error('[BLE] Base64 decode error:', err);
          }
        }
      }
    );

    return {
      id: device.id,
      name: device.name || 'LifeLine Field Node',
      rssi: device.rssi || -65,
      isSimulated: false,
    };
  }

  private handleIncomingChunk(chunk: string, onPacketReceived: (packet: string) => void): void {
    this.packetBuffer += chunk;
    const lines = this.packetBuffer.split('\n');
    // All complete lines are parsed
    while (lines.length > 1) {
      const line = lines.shift()?.trim();
      if (line && line.length > 0) {
        onPacketReceived(line);
      }
    }
    this.packetBuffer = lines[0] || '';
  }

  public async sendCommand(command: string): Promise<boolean> {
    if (!this.activeDevice) {
      throw new Error('No LifeLine device currently connected.');
    }

    const payload = command.endsWith('\n') ? command : `${command}\n`;
    const base64Data = stringToBase64(payload);

    await this.activeDevice.writeCharacteristicWithResponseForService(
      NUS_SERVICE_UUID,
      NUS_RX_UUID,
      base64Data
    );
    return true;
  }

  public async disconnect(): Promise<void> {
    if (this.activeDevice) {
      try {
        await this.activeDevice.cancelConnection();
      } catch (e) {
        console.warn('[BLE] Disconnect error:', e);
      }
      this.cleanup();
    }
  }

  private cleanup(): void {
    if (this.notificationSubscription) {
      try {
        this.notificationSubscription.remove();
      } catch (e) {}
      this.notificationSubscription = null;
    }
    this.activeDevice = null;
    this.packetBuffer = '';
  }
}

export const bleService = new BleService();
