import React from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  FlatList,
  ActivityIndicator,
} from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import { useLifeLine } from '../context/LifeLineContext';
import { LifeLineDevice } from '../constants/ble';
import { THEME } from '../constants/theme';

export const RadarScreen: React.FC = () => {
  const {
    connectionState,
    availableDevices,
    connectedDevice,
    statusMessage,
    startScan,
    stopScan,
    connectDevice,
    disconnectDevice,
    isSimulator,
  } = useLifeLine();

  const renderDevice = ({ item }: { item: LifeLineDevice }) => {
    const isConnected = connectedDevice?.id === item.id;
    const isBase = item.name.includes('RX') || item.name.includes('Base');

    return (
      <View style={styles.deviceCard}>
        <View style={styles.deviceIconFrame}>
          <Ionicons
            name={isBase ? 'business-outline' : 'walk-outline'}
            size={20}
            color={isBase ? THEME.colors.cyanStream : THEME.colors.text0}
          />
        </View>

        <View style={styles.deviceInfo}>
          <Text style={styles.deviceName}>{item.name.toUpperCase()}</Text>
          <View style={styles.deviceSubRow}>
            <Text style={styles.deviceId}>ID // {item.id}</Text>
            {item.rssi !== null && (
              <View style={styles.rssiBadge}>
                <Ionicons name="cellular-outline" size={12} color={THEME.colors.text2} />
                <Text style={styles.rssiText}>{item.rssi} DBM</Text>
              </View>
            )}
          </View>
        </View>

        <TouchableOpacity
          style={[
            styles.connectPill,
            isConnected ? styles.connectedPill : styles.actionPill,
          ]}
          onPress={() => (isConnected ? disconnectDevice() : connectDevice(item))}
          disabled={connectionState === 'CONNECTING'}
          activeOpacity={0.8}
        >
          <Text style={[styles.connectPillText, isConnected && styles.connectedPillText]}>
            {isConnected ? 'DISCONNECT' : 'LINK NODE'}
          </Text>
        </TouchableOpacity>
      </View>
    );
  };

  return (
    <View style={styles.container}>
      {/* Radar Main Console Card */}
      <View style={styles.consoleCard}>
        <View style={styles.radarVisualFrame}>
          <Ionicons
            name="radio-outline"
            size={36}
            color={connectionState === 'SCANNING' ? THEME.colors.cyanStream : THEME.colors.text2}
          />
          {connectionState === 'SCANNING' && (
            <ActivityIndicator
              size="small"
              color={THEME.colors.cyanStream}
              style={styles.radarSpinner}
            />
          )}
        </View>

        <Text style={styles.consoleTitle}>
          {connectionState === 'SCANNING'
            ? 'RF SPECTRUM ACTIVE'
            : connectionState === 'CONNECTED'
            ? 'RF LINK SYNCHRONIZED'
            : 'DISCOVERY CONSOLE'}
        </Text>

        <Text style={styles.statusSub}>
          {statusMessage?.toUpperCase() || 'STANDBY // READY TO ENGAGE BEACON'}
        </Text>

        {isSimulator && (
          <View style={styles.simNoticePill}>
            <Ionicons name="information-circle-outline" size={13} color={THEME.colors.solarAmber} />
            <Text style={styles.simNoticeText}>
              HARDWARE SIMULATOR ACTIVE // DUAL-NODE SYNTHESIS
            </Text>
          </View>
        )}

        <View style={styles.actionRow}>
          {connectionState === 'SCANNING' ? (
            <TouchableOpacity style={styles.stopPill} onPress={stopScan} activeOpacity={0.8}>
              <Ionicons name="stop-circle-outline" size={16} color={THEME.colors.danger} />
              <Text style={styles.stopPillText}>ABORT SCAN</Text>
            </TouchableOpacity>
          ) : (
            <TouchableOpacity
              style={styles.scanPill}
              onPress={startScan}
              disabled={connectionState === 'CONNECTING'}
              activeOpacity={0.85}
            >
              <Ionicons name="search-outline" size={16} color={THEME.colors.bg0} />
              <Text style={styles.scanPillText}>DISCOVER FIELD NODES</Text>
            </TouchableOpacity>
          )}
        </View>
      </View>

      {/* Discovered Devices Header */}
      <View style={styles.sectionHeaderRow}>
        <Text style={styles.sectionHeader}>DETECTED HARDWARE NODES</Text>
        <Text style={styles.counterText}>[{availableDevices.length}]</Text>
      </View>

      {availableDevices.length === 0 ? (
        <View style={styles.emptyState}>
          <Ionicons name="bluetooth-outline" size={32} color={THEME.colors.text3} />
          <Text style={styles.emptyText}>NO ACTIVE RF NODES DETECTED</Text>
          <Text style={styles.emptySubText}>
            Initiate scan to detect LifeLine TX Pro field transmitters or RX Pro base stations.
          </Text>
        </View>
      ) : (
        <FlatList
          data={availableDevices}
          renderItem={renderDevice}
          keyExtractor={item => item.id}
          contentContainerStyle={styles.listContainer}
        />
      )}
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: THEME.colors.bg0,
    padding: 16,
  },
  consoleCard: {
    backgroundColor: THEME.colors.bg2,
    borderRadius: THEME.geometry.sharp,
    padding: 22,
    alignItems: 'center',
    borderWidth: 1,
    borderColor: THEME.colors.border,
    marginBottom: 20,
  },
  radarVisualFrame: {
    width: 68,
    height: 68,
    borderRadius: THEME.geometry.sharp,
    backgroundColor: THEME.colors.bg1,
    borderWidth: 1,
    borderColor: THEME.colors.borderStrong,
    justifyContent: 'center',
    alignItems: 'center',
    marginBottom: 14,
    position: 'relative',
  },
  radarSpinner: {
    position: 'absolute',
  },
  consoleTitle: {
    color: THEME.colors.text0,
    fontSize: 14,
    fontWeight: '900',
    letterSpacing: 2,
    marginBottom: 4,
  },
  statusSub: {
    color: THEME.colors.text2,
    fontSize: 11,
    letterSpacing: 0.5,
    fontFamily: THEME.fonts.mono,
    textAlign: 'center',
    marginBottom: 16,
  },
  simNoticePill: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 6,
    backgroundColor: THEME.colors.warningBg,
    paddingHorizontal: 12,
    paddingVertical: 5,
    borderRadius: THEME.geometry.pill,
    borderWidth: 1,
    borderColor: THEME.colors.warning,
    marginBottom: 16,
  },
  simNoticeText: {
    color: THEME.colors.solarAmber,
    fontSize: 10,
    fontWeight: '800',
    letterSpacing: 0.8,
    fontFamily: THEME.fonts.mono,
  },
  actionRow: {
    width: '100%',
  },
  scanPill: {
    backgroundColor: THEME.colors.text0,
    paddingVertical: 13,
    borderRadius: THEME.geometry.pill,
    flexDirection: 'row',
    justifyContent: 'center',
    alignItems: 'center',
    gap: 8,
  },
  scanPillText: {
    color: THEME.colors.bg0,
    fontWeight: '900',
    fontSize: 12,
    letterSpacing: 1.5,
  },
  stopPill: {
    backgroundColor: THEME.colors.dangerBg,
    borderWidth: 1,
    borderColor: THEME.colors.danger,
    paddingVertical: 12,
    borderRadius: THEME.geometry.pill,
    flexDirection: 'row',
    justifyContent: 'center',
    alignItems: 'center',
    gap: 8,
  },
  stopPillText: {
    color: THEME.colors.danger,
    fontWeight: '900',
    fontSize: 12,
    letterSpacing: 1.5,
  },
  sectionHeaderRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 12,
    paddingHorizontal: 4,
  },
  sectionHeader: {
    color: THEME.colors.text2,
    fontSize: 11,
    fontWeight: '900',
    letterSpacing: 1.5,
  },
  counterText: {
    color: THEME.colors.text3,
    fontSize: 11,
    fontFamily: THEME.fonts.mono,
    fontWeight: '700',
  },
  listContainer: {
    paddingBottom: 24,
  },
  deviceCard: {
    backgroundColor: THEME.colors.bg2,
    borderRadius: THEME.geometry.sharp,
    padding: 14,
    flexDirection: 'row',
    alignItems: 'center',
    borderWidth: 1,
    borderColor: THEME.colors.border,
    marginBottom: 10,
  },
  deviceIconFrame: {
    width: 40,
    height: 40,
    borderRadius: THEME.geometry.sharp,
    backgroundColor: THEME.colors.bg1,
    borderWidth: 1,
    borderColor: THEME.colors.borderStrong,
    justifyContent: 'center',
    alignItems: 'center',
    marginRight: 12,
  },
  deviceInfo: {
    flex: 1,
  },
  deviceName: {
    color: THEME.colors.text0,
    fontSize: 13,
    fontWeight: '800',
    letterSpacing: 0.5,
    marginBottom: 3,
  },
  deviceSubRow: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 10,
  },
  deviceId: {
    color: THEME.colors.text3,
    fontSize: 10,
    fontFamily: THEME.fonts.mono,
  },
  rssiBadge: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 3,
  },
  rssiText: {
    color: THEME.colors.text2,
    fontSize: 10,
    fontFamily: THEME.fonts.mono,
  },
  connectPill: {
    paddingHorizontal: 16,
    paddingVertical: 7,
    borderRadius: THEME.geometry.pill,
  },
  actionPill: {
    backgroundColor: THEME.colors.text0,
  },
  connectedPill: {
    backgroundColor: THEME.colors.dangerBg,
    borderWidth: 1,
    borderColor: THEME.colors.danger,
  },
  connectPillText: {
    color: THEME.colors.bg0,
    fontSize: 11,
    fontWeight: '900',
    letterSpacing: 0.8,
  },
  connectedPillText: {
    color: THEME.colors.danger,
  },
  emptyState: {
    alignItems: 'center',
    justifyContent: 'center',
    paddingVertical: 48,
    borderWidth: 1,
    borderColor: THEME.colors.border,
    borderStyle: 'dashed',
    backgroundColor: THEME.colors.bg1,
  },
  emptyText: {
    color: THEME.colors.text1,
    fontSize: 12,
    fontWeight: '800',
    letterSpacing: 1.5,
    marginTop: 12,
  },
  emptySubText: {
    color: THEME.colors.text3,
    fontSize: 11,
    textAlign: 'center',
    marginTop: 4,
    paddingHorizontal: 24,
    lineHeight: 16,
  },
});
