import React, { useState } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  ScrollView,
  Switch,
  Alert,
} from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import { useLifeLine } from '../context/LifeLineContext';

export const SettingsScreen: React.FC = () => {
  const { theme, themeMode, toggleTheme, setThemeMode, connectionState, connectedDevice } = useLifeLine();

  // Settings local state
  const [loraFreq, setLoraFreq] = useState<'433' | '868' | '915'>('433');
  const [txPower, setTxPower] = useState<'14' | '17' | '20'>('20');
  const [spreadingFactor, setSpreadingFactor] = useState<'SF7' | 'SF9' | 'SF12'>('SF9');
  const [telemetryRate, setTelemetryRate] = useState<'1' | '3' | '5' | '10'>('3');
  const [autoReconnect, setAutoReconnect] = useState(true);
  const [audioAlerts, setAudioAlerts] = useState(true);
  const [haptics, setHaptics] = useState(true);
  const [resetSuccess, setResetSuccess] = useState(false);

  const handleResetDefaults = () => {
    setLoraFreq('433');
    setTxPower('20');
    setSpreadingFactor('SF9');
    setTelemetryRate('3');
    setAutoReconnect(true);
    setAudioAlerts(true);
    setHaptics(true);
    setThemeMode('dark');
    setResetSuccess(true);
    setTimeout(() => setResetSuccess(false), 2500);
  };

  return (
    <ScrollView
      style={[styles.container, { backgroundColor: theme.colors.bg0 }]}
      contentContainerStyle={styles.content}
      showsVerticalScrollIndicator={false}
    >
      {/* Page Header */}
      <View style={styles.headerBlock}>
        <Text style={[styles.title, { color: theme.colors.text0 }]}>SYSTEM CONFIGURATION</Text>
        <Text style={[styles.subtitle, { color: theme.colors.text2 }]}>
          HARDWARE PREFERENCES // RF PARAMETERS // UI THEME
        </Text>
      </View>

      {/* 1. Appearance / Theme */}
      <View style={[styles.card, { backgroundColor: theme.colors.bg2, borderColor: theme.colors.border }]}>
        <View style={styles.cardHeader}>
          <View style={styles.titleRow}>
            <Ionicons name="color-palette-outline" size={18} color={theme.colors.cyanStream} />
            <Text style={[styles.cardTitle, { color: theme.colors.text0 }]}>APPEARANCE & THEME</Text>
          </View>
          <Text style={[styles.badgeText, { color: theme.colors.cyanStream }]}>
            {themeMode.toUpperCase()} MODE
          </Text>
        </View>

        <Text style={[styles.cardDesc, { color: theme.colors.text2 }]}>
          Choose your tactical display mode. High-contrast monochromatic palettes optimized for field readability.
        </Text>

        <View style={styles.themeRow}>
          {/* Dark Mode Option */}
          <TouchableOpacity
            style={[
              styles.themeOption,
              {
                backgroundColor: '#0a0a0a',
                borderColor: themeMode === 'dark' ? theme.colors.cyanStream : theme.colors.borderStrong,
                borderWidth: themeMode === 'dark' ? 2 : 1,
              },
            ]}
            onPress={() => setThemeMode('dark')}
            activeOpacity={0.8}
          >
            <Ionicons
              name="moon"
              size={22}
              color={themeMode === 'dark' ? theme.colors.cyanStream : '#666666'}
            />
            <Text style={[styles.themeOptionTitle, { color: '#ffffff' }]}>DARK MODE</Text>
            <Text style={[styles.themeOptionDesc, { color: '#888888' }]}>OLED Pure Black</Text>
            {themeMode === 'dark' && (
              <View style={[styles.activeIndicator, { backgroundColor: theme.colors.cyanStream }]}>
                <Ionicons name="checkmark" size={12} color="#000000" />
              </View>
            )}
          </TouchableOpacity>

          {/* Light Mode Option */}
          <TouchableOpacity
            style={[
              styles.themeOption,
              {
                backgroundColor: '#f5f5f5',
                borderColor: themeMode === 'light' ? theme.colors.cyanStream : theme.colors.borderStrong,
                borderWidth: themeMode === 'light' ? 2 : 1,
              },
            ]}
            onPress={() => setThemeMode('light')}
            activeOpacity={0.8}
          >
            <Ionicons
              name="sunny"
              size={22}
              color={themeMode === 'light' ? '#0891b2' : '#999999'}
            />
            <Text style={[styles.themeOptionTitle, { color: '#000000' }]}>LIGHT MODE</Text>
            <Text style={[styles.themeOptionDesc, { color: '#666666' }]}>High-Sun Field</Text>
            {themeMode === 'light' && (
              <View style={[styles.activeIndicator, { backgroundColor: theme.colors.cyanStream }]}>
                <Ionicons name="checkmark" size={12} color="#000000" />
              </View>
            )}
          </TouchableOpacity>
        </View>
      </View>

      {/* 2. LoRa RF Configuration */}
      <View style={[styles.card, { backgroundColor: theme.colors.bg2, borderColor: theme.colors.border }]}>
        <View style={styles.cardHeader}>
          <View style={styles.titleRow}>
            <Ionicons name="radio-outline" size={18} color={theme.colors.cyanStream} />
            <Text style={[styles.cardTitle, { color: theme.colors.text0 }]}>LORA MESH RF PARAMETERS</Text>
          </View>
        </View>

        <Text style={[styles.cardDesc, { color: theme.colors.text2 }]}>
          Configure SX1278 spread-spectrum RF transceiver parameters for your geographical jurisdiction.
        </Text>

        {/* Frequency Bands */}
        <Text style={[styles.fieldLabel, { color: theme.colors.text1 }]}>CARRIER FREQUENCY BAND</Text>
        <View style={styles.pillGroup}>
          {[
            { id: '433', label: '433 MHz (Asia/EU)', note: 'LifeLine Default' },
            { id: '868', label: '868 MHz (Europe)', note: 'ISM Band' },
            { id: '915', label: '915 MHz (Americas)', note: 'US FCC' },
          ].map(item => {
            const isSel = loraFreq === item.id;
            return (
              <TouchableOpacity
                key={item.id}
                style={[
                  styles.optionPill,
                  {
                    backgroundColor: isSel ? theme.colors.buttonFill : theme.colors.bg1,
                    borderColor: isSel ? theme.colors.buttonFill : theme.colors.borderStrong,
                  },
                ]}
                onPress={() => setLoraFreq(item.id as any)}
                activeOpacity={0.8}
              >
                <Text
                  style={[
                    styles.optionPillText,
                    { color: isSel ? theme.colors.buttonText : theme.colors.text1 },
                  ]}
                >
                  {item.label}
                </Text>
              </TouchableOpacity>
            );
          })}
        </View>

        {/* Spreading Factor */}
        <Text style={[styles.fieldLabel, { color: theme.colors.text1, marginTop: 14 }]}>
          SPREADING FACTOR (RANGE VS BITRATE)
        </Text>
        <View style={styles.pillGroupRow}>
          {[
            { id: 'SF7', label: 'SF7 (Fast)' },
            { id: 'SF9', label: 'SF9 (Balanced)' },
            { id: 'SF12', label: 'SF12 (Max Range)' },
          ].map(item => {
            const isSel = spreadingFactor === item.id;
            return (
              <TouchableOpacity
                key={item.id}
                style={[
                  styles.segmentedBtn,
                  {
                    backgroundColor: isSel ? theme.colors.buttonFill : theme.colors.bg1,
                    borderColor: isSel ? theme.colors.buttonFill : theme.colors.borderStrong,
                  },
                ]}
                onPress={() => setSpreadingFactor(item.id as any)}
                activeOpacity={0.8}
              >
                <Text
                  style={[
                    styles.segmentedBtnText,
                    { color: isSel ? theme.colors.buttonText : theme.colors.text1 },
                  ]}
                >
                  {item.label}
                </Text>
              </TouchableOpacity>
            );
          })}
        </View>

        {/* Transmit Power */}
        <Text style={[styles.fieldLabel, { color: theme.colors.text1, marginTop: 14 }]}>
          RF TRANSMIT POWER BOOST
        </Text>
        <View style={styles.pillGroupRow}>
          {[
            { id: '14', label: '14 dBm (Eco)' },
            { id: '17', label: '17 dBm (Std)' },
            { id: '20', label: '20 dBm (Max Boost)' },
          ].map(item => {
            const isSel = txPower === item.id;
            return (
              <TouchableOpacity
                key={item.id}
                style={[
                  styles.segmentedBtn,
                  {
                    backgroundColor: isSel ? theme.colors.buttonFill : theme.colors.bg1,
                    borderColor: isSel ? theme.colors.buttonFill : theme.colors.borderStrong,
                  },
                ]}
                onPress={() => setTxPower(item.id as any)}
                activeOpacity={0.8}
              >
                <Text
                  style={[
                    styles.segmentedBtnText,
                    { color: isSel ? theme.colors.buttonText : theme.colors.text1 },
                  ]}
                >
                  {item.label}
                </Text>
              </TouchableOpacity>
            );
          })}
        </View>
      </View>

      {/* 3. Connectivity & Telemetry Polling */}
      <View style={[styles.card, { backgroundColor: theme.colors.bg2, borderColor: theme.colors.border }]}>
        <View style={styles.cardHeader}>
          <View style={styles.titleRow}>
            <Ionicons name="hardware-chip-outline" size={18} color={theme.colors.cyanStream} />
            <Text style={[styles.cardTitle, { color: theme.colors.text0 }]}>CONNECTIVITY & SYNC</Text>
          </View>
        </View>

        <View style={styles.switchRow}>
          <View style={styles.switchTextCol}>
            <Text style={[styles.switchTitle, { color: theme.colors.text0 }]}>Auto-Reconnect Last Node</Text>
            <Text style={[styles.switchSub, { color: theme.colors.text2 }]}>
              Automatically reconnect to paired ESP32 when in Bluetooth range.
            </Text>
          </View>
          <Switch
            value={autoReconnect}
            onValueChange={setAutoReconnect}
            thumbColor={autoReconnect ? theme.colors.buttonFill : '#888888'}
            trackColor={{ false: '#333333', true: theme.colors.cyanStream }}
          />
        </View>

        <View style={[styles.divider, { backgroundColor: theme.colors.border }]} />

        <View style={styles.switchRow}>
          <View style={styles.switchTextCol}>
            <Text style={[styles.switchTitle, { color: theme.colors.text0 }]}>Tactical Sound FX</Text>
            <Text style={[styles.switchSub, { color: theme.colors.text2 }]}>
              Audio chime on incoming base station messages and SOS ACK receipts.
            </Text>
          </View>
          <Switch
            value={audioAlerts}
            onValueChange={setAudioAlerts}
            thumbColor={audioAlerts ? theme.colors.buttonFill : '#888888'}
            trackColor={{ false: '#333333', true: theme.colors.cyanStream }}
          />
        </View>

        <View style={[styles.divider, { backgroundColor: theme.colors.border }]} />

        <View style={styles.switchRow}>
          <View style={styles.switchTextCol}>
            <Text style={[styles.switchTitle, { color: theme.colors.text0 }]}>Haptic Pulsing</Text>
            <Text style={[styles.switchSub, { color: theme.colors.text2 }]}>
              Vibration feedback during emergency broadcasts and hardware connection.
            </Text>
          </View>
          <Switch
            value={haptics}
            onValueChange={setHaptics}
            thumbColor={haptics ? theme.colors.buttonFill : '#888888'}
            trackColor={{ false: '#333333', true: theme.colors.cyanStream }}
          />
        </View>

        <View style={[styles.divider, { backgroundColor: theme.colors.border }]} />

        <Text style={[styles.fieldLabel, { color: theme.colors.text1, marginTop: 10 }]}>
          TELEMETRY REFRESH INTERVAL
        </Text>
        <View style={styles.pillGroupRow}>
          {['1', '3', '5', '10'].map(sec => {
            const isSel = telemetryRate === sec;
            return (
              <TouchableOpacity
                key={sec}
                style={[
                  styles.rateBtn,
                  {
                    backgroundColor: isSel ? theme.colors.buttonFill : theme.colors.bg1,
                    borderColor: isSel ? theme.colors.buttonFill : theme.colors.borderStrong,
                  },
                ]}
                onPress={() => setTelemetryRate(sec as any)}
                activeOpacity={0.8}
              >
                <Text
                  style={[
                    styles.rateBtnText,
                    { color: isSel ? theme.colors.buttonText : theme.colors.text1 },
                  ]}
                >
                  {sec}s
                </Text>
              </TouchableOpacity>
            );
          })}
        </View>
      </View>

      {/* 4. Diagnostics & Hardware Info */}
      <View style={[styles.card, { backgroundColor: theme.colors.bg2, borderColor: theme.colors.border }]}>
        <View style={styles.cardHeader}>
          <View style={styles.titleRow}>
            <Ionicons name="information-circle-outline" size={18} color={theme.colors.cyanStream} />
            <Text style={[styles.cardTitle, { color: theme.colors.text0 }]}>HARDWARE SPECIFICATIONS</Text>
          </View>
        </View>

        <View style={styles.specRow}>
          <Text style={[styles.specLabel, { color: theme.colors.text2 }]}>Active Link State:</Text>
          <Text style={[styles.specValue, { color: connectionState === 'CONNECTED' ? theme.colors.success : theme.colors.solarAmber }]}>
            {connectionState} {connectedDevice ? `(${connectedDevice.name})` : ''}
          </Text>
        </View>
        <View style={styles.specRow}>
          <Text style={[styles.specLabel, { color: theme.colors.text2 }]}>Firmware Build:</Text>
          <Text style={[styles.specValue, { color: theme.colors.text0 }]}>LifeLine v3.1.0 PRO</Text>
        </View>
        <View style={styles.specRow}>
          <Text style={[styles.specLabel, { color: theme.colors.text2 }]}>Microcontroller:</Text>
          <Text style={[styles.specValue, { color: theme.colors.text0 }]}>ESP32-S3 Dual-Core 240MHz</Text>
        </View>
        <View style={styles.specRow}>
          <Text style={[styles.specLabel, { color: theme.colors.text2 }]}>Transceiver Chip:</Text>
          <Text style={[styles.specValue, { color: theme.colors.text0 }]}>Semtech SX1278 LoRa</Text>
        </View>
        <View style={styles.specRow}>
          <Text style={[styles.specLabel, { color: theme.colors.text2 }]}>Bluetooth Profile:</Text>
          <Text style={[styles.specValue, { color: theme.colors.cyanStream }]}>Nordic UART (NUS)</Text>
        </View>

        {/* Reset Action */}
        <TouchableOpacity
          style={[styles.resetBtn, { borderColor: theme.colors.borderStrong }]}
          onPress={handleResetDefaults}
          activeOpacity={0.8}
        >
          <Ionicons name="refresh-outline" size={16} color={theme.colors.text2} />
          <Text style={[styles.resetBtnText, { color: theme.colors.text2 }]}>
            {resetSuccess ? 'CONFIGURATIONS RESTORED' : 'RESTORE FACTORY DEFAULTS'}
          </Text>
        </TouchableOpacity>
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
  },
  content: {
    padding: 16,
    paddingBottom: 40,
  },
  headerBlock: {
    marginBottom: 16,
    paddingHorizontal: 4,
  },
  title: {
    fontSize: 16,
    fontWeight: '900',
    letterSpacing: 2,
    marginBottom: 4,
  },
  subtitle: {
    fontSize: 10,
    fontFamily: 'monospace',
    letterSpacing: 1,
  },
  card: {
    borderRadius: 16,
    padding: 18,
    borderWidth: 1,
    marginBottom: 16,
  },
  cardHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 8,
  },
  titleRow: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
  },
  cardTitle: {
    fontSize: 12,
    fontWeight: '900',
    letterSpacing: 1.5,
  },
  badgeText: {
    fontSize: 9.5,
    fontFamily: 'monospace',
    fontWeight: '800',
    letterSpacing: 1,
  },
  cardDesc: {
    fontSize: 11,
    lineHeight: 16,
    marginBottom: 14,
  },
  themeRow: {
    flexDirection: 'row',
    gap: 12,
  },
  themeOption: {
    flex: 1,
    borderRadius: 14,
    padding: 16,
    alignItems: 'center',
    position: 'relative',
  },
  themeOptionTitle: {
    fontSize: 11,
    fontWeight: '900',
    letterSpacing: 1,
    marginTop: 8,
    marginBottom: 2,
  },
  themeOptionDesc: {
    fontSize: 9.5,
    fontFamily: 'monospace',
  },
  activeIndicator: {
    position: 'absolute',
    top: 10,
    right: 10,
    width: 20,
    height: 20,
    borderRadius: 10,
    justifyContent: 'center',
    alignItems: 'center',
  },
  fieldLabel: {
    fontSize: 10,
    fontWeight: '900',
    letterSpacing: 1,
    fontFamily: 'monospace',
    marginBottom: 8,
  },
  pillGroup: {
    gap: 8,
  },
  optionPill: {
    borderRadius: 10,
    paddingVertical: 10,
    paddingHorizontal: 14,
    borderWidth: 1,
  },
  optionPillText: {
    fontSize: 11,
    fontWeight: '800',
    letterSpacing: 0.5,
  },
  pillGroupRow: {
    flexDirection: 'row',
    gap: 8,
  },
  segmentedBtn: {
    flex: 1,
    borderRadius: 10,
    paddingVertical: 10,
    paddingHorizontal: 6,
    borderWidth: 1,
    alignItems: 'center',
  },
  segmentedBtnText: {
    fontSize: 10,
    fontWeight: '800',
    textAlign: 'center',
  },
  switchRow: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    paddingVertical: 6,
  },
  switchTextCol: {
    flex: 1,
    paddingRight: 12,
  },
  switchTitle: {
    fontSize: 12,
    fontWeight: '800',
    letterSpacing: 0.5,
    marginBottom: 2,
  },
  switchSub: {
    fontSize: 10,
    lineHeight: 14,
  },
  divider: {
    height: 1,
    marginVertical: 10,
  },
  rateBtn: {
    flex: 1,
    borderRadius: 10,
    paddingVertical: 10,
    alignItems: 'center',
    borderWidth: 1,
  },
  rateBtnText: {
    fontSize: 11,
    fontWeight: '800',
    fontFamily: 'monospace',
  },
  specRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 6,
    borderBottomWidth: 1,
    borderBottomColor: '#222222',
  },
  specLabel: {
    fontSize: 10.5,
    fontFamily: 'monospace',
  },
  specValue: {
    fontSize: 10.5,
    fontFamily: 'monospace',
    fontWeight: '800',
  },
  resetBtn: {
    flexDirection: 'row',
    justifyContent: 'center',
    alignItems: 'center',
    gap: 8,
    borderWidth: 1,
    borderRadius: 12,
    paddingVertical: 12,
    marginTop: 16,
  },
  resetBtnText: {
    fontSize: 10.5,
    fontWeight: '900',
    fontFamily: 'monospace',
    letterSpacing: 1,
  },
});
