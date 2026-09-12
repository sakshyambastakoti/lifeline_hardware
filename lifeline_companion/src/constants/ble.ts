// Nordic UART Service (NUS) Specifications used in LifeLine TX Pro and RX Pro
export const NUS_SERVICE_UUID = '6E400001-B5A3-F393-E0A9-E50E24DCCA9E'.toLowerCase();
export const NUS_RX_UUID      = '6E400002-B5A3-F393-E0A9-E50E24DCCA9E'.toLowerCase(); // Phone -> ESP32 (Write)
export const NUS_TX_UUID      = '6E400003-B5A3-F393-E0A9-E50E24DCCA9E'.toLowerCase(); // ESP32 -> Phone (Notify)

export type ConnectionState = 'DISCONNECTED' | 'SCANNING' | 'CONNECTING' | 'CONNECTED';

export interface LifeLineDevice {
  id: string;
  name: string;
  rssi: number | null;
  isSimulated?: boolean;
}

export interface LifeLineTelemetry {
  deviceId: string;
  batteryPct: number;
  loraStatus: string;
  version: string;
  latitude?: number;
  longitude?: number;
  altitude?: number;
  temperature?: number;
  humidity?: number;
  gasPpm?: number;
  lastUpdated: Date;
}

export interface ChatMessage {
  id: string;
  sender: string;
  text: string;
  timestamp: Date;
  isOutgoing: boolean;
  status: 'PENDING' | 'SENT' | 'CONFIRMED' | 'FAILED';
  rssi?: number;
}

export interface SosStatus {
  isActive: boolean;
  code: string;
  name: string;
  triggeredAt: Date | null;
  ackStatus: string;
  ackNote: string;
  ackRssi: number | null;
  ackSnr: number | null;
}
