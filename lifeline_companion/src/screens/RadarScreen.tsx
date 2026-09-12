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
    theme,
  } = useLifeLine();

  const renderDevice = ({ item }: { item: LifeLineDevice }) => {
    const isConnected = connectedDevice?.id === item.id;
    const isBase = item.name.includes('RX') || item.name.includes('Base');

    return (
      <View style={[styles.deviceCard, { backgroundColor: theme.colors.bg2, borderColor: theme.colors.border }]}>
        <View style={[styles.deviceIconFrame, { backgroundColor: theme.colors.bg1, borderColor: theme.colors.borderStrong }]}>
          <Ionicons
            name={isBase ? 'business-outline' : 'walk-outline'}
            size={20}
            color={isBase ? theme.colors.cyanStream : theme.colors.text0}
          />
        </View>

        <View style={styles.deviceInfo}>
          <Text style={[styles.deviceName, { color: theme.colors.text0 }]}>{item.name.toUpperCase()}</Text>
          <View style={styles.deviceSubRow}>
            <Text style={[styles.deviceId, { color: theme.colors.text3 }]}>ID // {item.id}</Text>
            {item.rssi !== null && (
              <View style={styles.rssiBadge}>
                <Ionicons name="cellular-outline" size={12} color={theme.colors.text2} />
                <Text style={[styles.rssiText, { color: theme.colors.text2 }]}>{item.rssi} DBM</Text>
              </View>
            )}
          </View>
        </View>

        <TouchableOpacity
          style={[
            styles.connectPill,
            isConnected
              ? { backgroundColor: theme.colors.dangerBg, borderColor: theme.colors.danger, borderWidth: 1 }
              : { backgroundColor: theme.colors.buttonFill },
          ]}
          onPress={() => (isConnected ? disconnectDevice() : connectDevice(item))}
          disabled={connectionState === 'CONNECTING'}
          activeOpacity={0.8}
        >
          <Text
            style={[
              styles.connectPillText,
              { color: isConnected ? theme.colors.danger : theme.colors.buttonText },
            ]}
          >
            {isConnected ? 'DISCONNECT' : 'LINK NODE'}
          </Text>
        </TouchableOpacity>
      </View>
    );
  };

  return (
    <View style={[styles.container, { backgroundColor: theme.colors.bg0 }]}>
      {/* Radar Main Console Card */}
      <View style={[styles.consoleCard, { backgroundColor: theme.colors.bg2, borderColor: theme.colors.border }]}>
        <View
          style={[
            styles.radarVisualFrame,
            { backgroundColor: theme.colors.bg1, borderColor: theme.colors.borderStrong },
          ]}
        >
          <Ionicons
            name="radio-outline"
            size={36}
            color={connectionState === 'SCANNING' ? theme.colors.cyanStream : theme.colors.text2}
          />
          {connectionState === 'SCANNING' && (
            <ActivityIndicator
              size="small"
              color={theme.colors.cyanStream}
              style={styles.radarSpinner}
            />
          )}
        </View>

        <Text style={[styles.consoleTitle, { color: theme.colors.text0 }]}>
          {connectionState === 'SCANNING'
            ? 'RF SPECTRUM ACTIVE'
            : connectionState === 'CONNECTED'
            ? 'RF LINK SYNCHRONIZED'
            : 'DISCOVERY CONSOLE'}
        </Text>

        <Text style={[styles.statusSub, { color: theme.colors.text2 }]}>
          {statusMessage?.toUpperCase() || 'STANDBY // READY TO ENGAGE BEACON'}
        </Text>

        {isSimulator && (
          <View
            style={[
              styles.simNoticePill,
              { backgroundColor: theme.colors.warningBg, borderColor: theme.colors.warning },
            ]}
          >
            <Ionicons name="information-circle-outline" size={13} color={theme.colors.solarAmber} />
            <Text style={[styles.simNoticeText, { color: theme.colors.solarAmber }]}>
              HARDWARE SIMULATOR ACTIVE // DUAL-NODE SYNTHESIS
            </Text>
          </View>
        )}

        <View style={styles.actionRow}>
          {connectionState === 'SCANNING' ? (
            <TouchableOpacity
              style={[
                styles.stopPill,
                { backgroundColor: theme.colors.dangerBg, borderColor: theme.colors.danger },
              ]}
              onPress={stopScan}
              activeOpacity={0.8}
            >
              <Ionicons name="stop-circle-outline" size={16} color={theme.colors.danger} />
              <Text style={[styles.stopPillText, { color: theme.colors.danger }]}>ABORT SCAN</Text>
            </TouchableOpacity>
          ) : (
            <TouchableOpacity
              style={[styles.scanPill, { backgroundColor: theme.colors.buttonFill }]}
              onPress={startScan}
              disabled={connectionState === 'CONNECTING'}
              activeOpacity={0.85}
            >
              <Ionicons name="search-outline" size={16} color={theme.colors.buttonText} />
              <Text style={[styles.scanPillText, { color: theme.colors.buttonText }]}>
                DISCOVER FIELD NODES
              </Text>
            </TouchableOpacity>
          )}
        </View>
      </View>

      {/* Discovered Devices Header */}
      <View style={styles.sectionHeaderRow}>
        <Text style={[styles.sectionHeader, { color: theme.colors.text2 }]}>DETECTED HARDWARE NODES</Text>
        <Text style={[styles.counterText, { color: theme.colors.text3 }]}>[{availableDevices.length}]</Text>
      </View>

      {availableDevices.length === 0 ? (
        <View style={[styles.emptyState, { backgroundColor: theme.colors.bg1, borderColor: theme.colors.border }]}>
          <Ionicons name="bluetooth-outline" size={32} color={theme.colors.text3} />
          <Text style={[styles.emptyText, { color: theme.colors.text1 }]}>NO ACTIVE RF NODES DETECTED</Text>
          <Text style={[styles.emptySubText, { color: theme.colors.text3 }]}>
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
    padding: 16,
  },
  consoleCard: {
    borderRadius: 0,
    padding: 22,
    alignItems: 'center',
    borderWidth: 1,
    marginBottom: 20,
  },
  radarVisualFrame: {
    width: 68,
    height: 68,
    borderRadius: 0,
    borderWidth: 1,
    justifyContent: 'center',
    alignItems: 'center',
    marginBottom: 14,
    position: 'relative',
  },
  radarSpinner: {
    position: 'absolute',
  },
  consoleTitle: {
    fontSize: 14,
    fontWeight: '900',
    letterSpacing: 2,
    marginBottom: 4,
  },
  statusSub: {
    fontSize: 11,
    letterSpacing: 0.5,
    fontFamily: 'monospace',
    textAlign: 'center',
    marginBottom: 16,
  },
  simNoticePill: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 6,
    paddingHorizontal: 12,
    paddingVertical: 5,
    borderRadius: 9999,
    borderWidth: 1,
    marginBottom: 16,
  },
  simNoticeText: {
    fontSize: 10,
    fontWeight: '800',
    letterSpacing: 0.8,
    fontFamily: 'monospace',
  },
  actionRow: {
    width: '100%',
  },
  scanPill: {
    paddingVertical: 13,
    borderRadius: 9999,
    flexDirection: 'row',
    justifyContent: 'center',
    alignItems: 'center',
    gap: 8,
  },
  scanPillText: {
    fontWeight: '900',
    fontSize: 12,
    letterSpacing: 1.5,
  },
  stopPill: {
    borderWidth: 1,
    paddingVertical: 12,
    borderRadius: 9999,
    flexDirection: 'row',
    justifyContent: 'center',
    alignItems: 'center',
    gap: 8,
  },
  stopPillText: {
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
    fontSize: 11,
    fontWeight: '900',
    letterSpacing: 1.5,
  },
  counterText: {
    fontSize: 11,
    fontFamily: 'monospace',
    fontWeight: '700',
  },
  listContainer: {
    paddingBottom: 24,
  },
  deviceCard: {
    borderRadius: 0,
    padding: 14,
    flexDirection: 'row',
    alignItems: 'center',
    borderWidth: 1,
    marginBottom: 10,
  },
  deviceIconFrame: {
    width: 40,
    height: 40,
    borderRadius: 0,
    borderWidth: 1,
    justifyContent: 'center',
    alignItems: 'center',
    marginRight: 12,
  },
  deviceInfo: {
    flex: 1,
  },
  deviceName: {
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
    fontSize: 10,
    fontFamily: 'monospace',
  },
  rssiBadge: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 3,
  },
  rssiText: {
    fontSize: 10,
    fontFamily: 'monospace',
  },
  connectPill: {
    paddingHorizontal: 16,
    paddingVertical: 7,
    borderRadius: 9999,
  },
  connectPillText: {
    fontSize: 11,
    fontWeight: '900',
    letterSpacing: 0.8,
  },
  emptyState: {
    alignItems: 'center',
    justifyContent: 'center',
    paddingVertical: 48,
    borderWidth: 1,
    borderStyle: 'dashed',
  },
  emptyText: {
    fontSize: 12,
    fontWeight: '800',
    letterSpacing: 1.5,
    marginTop: 12,
  },
  emptySubText: {
    fontSize: 11,
    textAlign: 'center',
    marginTop: 4,
    paddingHorizontal: 24,
    lineHeight: 16,
  },
});
