import React, { createContext, useContext, useState, useEffect, ReactNode } from 'react';
import {
  ConnectionState,
  LifeLineDevice,
  LifeLineTelemetry,
  ChatMessage,
  SosStatus,
} from '../constants/ble';
import { bleService } from '../services/BleService';
import { mockBleService } from '../services/MockBleService';

interface LifeLineContextType {
  connectionState: ConnectionState;
  isSimulator: boolean;
  setIsSimulator: (val: boolean) => void;
  availableDevices: LifeLineDevice[];
  connectedDevice: LifeLineDevice | null;
  telemetry: LifeLineTelemetry | null;
  chatMessages: ChatMessage[];
  sosStatus: SosStatus;
  statusMessage: string | null;
  startScan: () => void;
  stopScan: () => void;
  connectDevice: (device: LifeLineDevice) => Promise<void>;
  disconnectDevice: () => Promise<void>;
  sendChatMessage: (text: string) => Promise<boolean>;
  triggerSos: (code: string, name: string) => Promise<boolean>;
  clearSos: () => void;
}

const LifeLineContext = createContext<LifeLineContextType | undefined>(undefined);

export const LifeLineProvider: React.FC<{ children: ReactNode }> = ({ children }) => {
  // Automatically start in simulator mode if native BLE isn't supported in current runtime (e.g. Expo Go)
  const [isSimulator, setIsSimulator] = useState<boolean>(!bleService.isNativeBleSupported());
  const [connectionState, setConnectionState] = useState<ConnectionState>('DISCONNECTED');
  const [availableDevices, setAvailableDevices] = useState<LifeLineDevice[]>([]);
  const [connectedDevice, setConnectedDevice] = useState<LifeLineDevice | null>(null);
  const [statusMessage, setStatusMessage] = useState<string | null>(null);

  const [telemetry, setTelemetry] = useState<LifeLineTelemetry | null>(null);
  const [chatMessages, setChatMessages] = useState<ChatMessage[]>([]);
  const [sosStatus, setSosStatus] = useState<SosStatus>({
    isActive: false,
    code: '',
    name: '',
    triggeredAt: null,
    ackStatus: 'AWAITING_BASE',
    ackNote: '',
    ackRssi: null,
    ackSnr: null,
  });

  const activeService = isSimulator ? mockBleService : bleService;

  // Packet parser for incoming Nordic UART ASCII streams
  const handleIncomingPacket = (packet: string) => {
    console.log('[LifeLine Packet]:', packet);
    const clean = packet.trim();

    // 1. STATUS packet (e.g., STATUS:DEV=003,BAT=92,LORA=OK,VER=v3.1.0 PRO)
    if (clean.startsWith('STATUS:')) {
      const dataStr = clean.substring(7);
      const parts = dataStr.split(',');
      const map: Record<string, string> = {};
      parts.forEach(p => {
        const [k, v] = p.split('=');
        if (k && v) map[k.trim()] = v.trim();
      });

      setTelemetry(prev => ({
        deviceId: map['DEV'] || prev?.deviceId || '001',
        batteryPct: parseInt(map['BAT'] || `${prev?.batteryPct || 90}`, 10),
        loraStatus: map['LORA'] || prev?.loraStatus || 'OK',
        version: map['VER'] || prev?.version || 'v3.1.0',
        lastUpdated: new Date(),
        latitude: prev?.latitude,
        longitude: prev?.longitude,
        altitude: prev?.altitude,
        temperature: prev?.temperature,
        humidity: prev?.humidity,
        gasPpm: prev?.gasPpm,
      }));
    }
    // 2. TELEMETRY packet (e.g., TELEMETRY:DEV=003,TEMP=21.4,HUM=88.2,LAT=27.717200,LON=85.324000,ALT=1350)
    else if (clean.startsWith('TELEMETRY:')) {
      const dataStr = clean.substring(10);
      const parts = dataStr.split(',');
      const map: Record<string, string> = {};
      parts.forEach(p => {
        const [k, v] = p.split('=');
        if (k && v) map[k.trim()] = v.trim();
      });

      setTelemetry(prev => ({
        deviceId: map['DEV'] || prev?.deviceId || '003',
        batteryPct: map['BAT'] ? parseInt(map['BAT'], 10) : (prev?.batteryPct || 85),
        loraStatus: prev?.loraStatus || 'OK',
        version: prev?.version || 'v3.1.0 PRO',
        latitude: map['LAT'] ? parseFloat(map['LAT']) : prev?.latitude,
        longitude: map['LON'] ? parseFloat(map['LON']) : prev?.longitude,
        altitude: map['ALT'] ? parseFloat(map['ALT']) : prev?.altitude,
        temperature: map['TEMP'] ? parseFloat(map['TEMP']) : prev?.temperature,
        humidity: map['HUM'] ? parseFloat(map['HUM']) : prev?.humidity,
        gasPpm: map['GAS'] ? parseInt(map['GAS'], 10) : prev?.gasPpm,
        lastUpdated: new Date(),
      }));
    }
    // 3. CLOSED-LOOP ACK (e.g., ACK_RECV:STATUS=DISPATCHED,BASE=BASE01,NOTE=Rescue en route,RSSI=-68,SNR=9)
    else if (clean.startsWith('ACK_RECV:')) {
      const dataStr = clean.substring(9);
      const parts = dataStr.split(',');
      const map: Record<string, string> = {};
      parts.forEach(p => {
        const [k, v] = p.split('=');
        if (k && v) map[k.trim()] = v.trim();
      });

      setSosStatus(prev => ({
        ...prev,
        ackStatus: map['STATUS'] || 'CONFIRMED',
        ackNote: map['NOTE'] || 'Base Station Received Alert',
        ackRssi: map['RSSI'] ? parseInt(map['RSSI'], 10) : null,
        ackSnr: map['SNR'] ? parseInt(map['SNR'], 10) : null,
      }));

      // Update any pending outgoing messages
      setChatMessages(prev =>
        prev.map(msg => (msg.status === 'PENDING' ? { ...msg, status: 'CONFIRMED' } : msg))
      );
    }
    // 4. INCOMING CHAT (e.g., CHAT:DEV=BASE01,TEXT=Landslide blocked road,RSSI=-62)
    else if (clean.startsWith('CHAT:')) {
      const dataStr = clean.substring(5);
      const parts = dataStr.split(',');
      let dev = 'Base Station';
      let text = '';
      let rssi: number | undefined;

      parts.forEach(p => {
        if (p.startsWith('DEV=')) dev = p.substring(4);
        else if (p.startsWith('TEXT=')) text = p.substring(5);
        else if (p.startsWith('RSSI=')) rssi = parseInt(p.substring(5), 10);
      });

      const newMsg: ChatMessage = {
        id: Date.now().toString(),
        sender: dev,
        text: text || clean,
        timestamp: new Date(),
        isOutgoing: false,
        status: 'CONFIRMED',
        rssi,
      };

      setChatMessages(prev => [...prev, newMsg]);
    }
  };

  const startScan = () => {
    setAvailableDevices([]);
    setConnectionState('SCANNING');
    setStatusMessage('Scanning for LifeLine emergency radios...');

    activeService.startScan(
      (device: LifeLineDevice) => {
        setAvailableDevices(prev => {
          if (prev.find(d => d.id === device.id)) return prev;
          return [...prev, device];
        });
      },
      (error: any) => {
        console.error('Scan error:', error);
        setConnectionState('DISCONNECTED');
        setStatusMessage(error.message || 'Scanning failed');
      }
    );
  };

  const stopScan = () => {
    activeService.stopScan();
    if (connectionState === 'SCANNING') {
      setConnectionState('DISCONNECTED');
    }
  };

  const connectDevice = async (device: LifeLineDevice) => {
    try {
      setConnectionState('CONNECTING');
      setStatusMessage(`Connecting to ${device.name}...`);

      const connected = await activeService.connect(
        device.id,
        handleIncomingPacket,
        () => {
          // Disconnected callback
          setConnectedDevice(null);
          setConnectionState('DISCONNECTED');
          setStatusMessage('Device disconnected.');
        }
      );

      setConnectedDevice(connected);
      setConnectionState('CONNECTED');
      setStatusMessage(`Connected to ${connected.name}`);
    } catch (e: any) {
      setConnectionState('DISCONNECTED');
      setStatusMessage(`Connection failed: ${e.message}`);
    }
  };

  const disconnectDevice = async () => {
    await activeService.disconnect();
    setConnectedDevice(null);
    setConnectionState('DISCONNECTED');
    setStatusMessage('Disconnected.');
  };

  const sendChatMessage = async (text: string): Promise<boolean> => {
    if (connectionState !== 'CONNECTED') return false;

    const newMsg: ChatMessage = {
      id: Date.now().toString(),
      sender: 'Me (Field)',
      text,
      timestamp: new Date(),
      isOutgoing: true,
      status: 'PENDING',
    };

    setChatMessages(prev => [...prev, newMsg]);

    try {
      await activeService.sendCommand(`MSG:${text}`);
      return true;
    } catch (e) {
      setChatMessages(prev =>
        prev.map(m => (m.id === newMsg.id ? { ...m, status: 'FAILED' } : m))
      );
      return false;
    }
  };

  const triggerSos = async (code: string, name: string): Promise<boolean> => {
    if (connectionState !== 'CONNECTED') return false;

    setSosStatus({
      isActive: true,
      code,
      name,
      triggeredAt: new Date(),
      ackStatus: 'TRANSMITTING_LORA',
      ackNote: 'Broadcasting over 433 MHz LoRa RF...',
      ackRssi: null,
      ackSnr: null,
    });

    try {
      await activeService.sendCommand(`ALERT:${code}`);
      return true;
    } catch (e) {
      setSosStatus(prev => ({
        ...prev,
        ackStatus: 'TRANSMIT_FAILED',
        ackNote: 'Failed to deliver to field unit via BLE',
      }));
      return false;
    }
  };

  const clearSos = () => {
    setSosStatus({
      isActive: false,
      code: '',
      name: '',
      triggeredAt: null,
      ackStatus: 'AWAITING_BASE',
      ackNote: '',
      ackRssi: null,
      ackSnr: null,
    });
  };

  return (
    <LifeLineContext.Provider
      value={{
        connectionState,
        isSimulator,
        setIsSimulator,
        availableDevices,
        connectedDevice,
        telemetry,
        chatMessages,
        sosStatus,
        statusMessage,
        startScan,
        stopScan,
        connectDevice,
        disconnectDevice,
        sendChatMessage,
        triggerSos,
        clearSos,
      }}
    >
      {children}
    </LifeLineContext.Provider>
  );
};

export const useLifeLine = () => {
  const context = useContext(LifeLineContext);
  if (!context) {
    throw new Error('useLifeLine must be used within a LifeLineProvider');
  }
  return context;
};
